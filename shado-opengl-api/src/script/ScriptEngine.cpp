#include "ScriptEngine.h"
#include "ScriptGlue.h"

#include "mono/jit/jit.h"
#include "mono/metadata/assembly.h"
#include "mono/metadata/object.h"
#include "mono/metadata/tabledefs.h"
#include "mono/metadata/mono-debug.h"
#include "mono/metadata/threads.h"

#include "FileWatch.h"

#include "Application.h"
#include "debug/Profile.h"
#include "../util/Buffer.h"
#include "../util/FileSystem.h"

#include "../Project/Project.h"
#include "scene/Components.h"

namespace Shado {

	static std::unordered_map<std::string, ScriptFieldType> s_ScriptFieldTypeMap =
	{
		{ "System.Single", ScriptFieldType::Float },
		{ "System.Double", ScriptFieldType::Double },
		{ "System.Boolean", ScriptFieldType::Bool },
		{ "System.Char", ScriptFieldType::Char },
		{ "System.Int16", ScriptFieldType::Short },
		{ "System.Int32", ScriptFieldType::Int },
		{ "System.Int64", ScriptFieldType::Long },
		{ "System.Byte", ScriptFieldType::Byte },
		{ "System.UInt16", ScriptFieldType::UShort },
		{ "System.UInt32", ScriptFieldType::UInt },
		{ "System.UInt64", ScriptFieldType::ULong },

		{ "Shado.Vector2", ScriptFieldType::Vector2 },
		{ "Shado.Vector3", ScriptFieldType::Vector3 },
		{ "Shado.Vector4", ScriptFieldType::Vector4 },
		{ "Shado.Colour", ScriptFieldType::Colour },

		{ "Shado.Entity", ScriptFieldType::Entity },
	};

	namespace Utils {

		static MonoAssembly* LoadMonoAssembly(const std::filesystem::path& assemblyPath, bool loadPDB = false)
		{
			ScopedBuffer fileData = FileSystem::ReadFileBinary(assemblyPath);

			// NOTE: We can't use this image for anything other than loading the assembly because this image doesn't have a reference to the assembly
			MonoImageOpenStatus status;
			MonoImage* image = mono_image_open_from_data_full(fileData.As<char>(), fileData.Size(), 1, &status, 0);

			if (status != MONO_IMAGE_OK)
			{
				const char* errorMessage = mono_image_strerror(status);
				// Log some error message using the errorMessage data
				return nullptr;
			}

			if (loadPDB)
			{
				std::filesystem::path pdbPath = assemblyPath;
				pdbPath.replace_extension(".pdb");

				if (std::filesystem::exists(pdbPath))
				{
					ScopedBuffer pdbFileData = FileSystem::ReadFileBinary(pdbPath);
					mono_debug_open_image_from_memory(image, pdbFileData.As<const mono_byte>(), pdbFileData.Size());
					SHADO_CORE_INFO("Loaded PDB {}", pdbPath);
				}
			}

			std::string pathString = assemblyPath.string();
			MonoAssembly* assembly = mono_assembly_load_from_full(image, pathString.c_str(), &status, 0);
			mono_image_close(image);

			return assembly;
		}

		void PrintAssemblyTypes(MonoAssembly* assembly)
		{
			MonoImage* image = mono_assembly_get_image(assembly);
			const MonoTableInfo* typeDefinitionsTable = mono_image_get_table_info(image, MONO_TABLE_TYPEDEF);
			int32_t numTypes = mono_table_info_get_rows(typeDefinitionsTable);

			for (int32_t i = 0; i < numTypes; i++)
			{
				uint32_t cols[MONO_TYPEDEF_SIZE];
				mono_metadata_decode_row(typeDefinitionsTable, i, cols, MONO_TYPEDEF_SIZE);

				const char* nameSpace = mono_metadata_string_heap(image, cols[MONO_TYPEDEF_NAMESPACE]);
				const char* name = mono_metadata_string_heap(image, cols[MONO_TYPEDEF_NAME]);
				SHADO_CORE_TRACE("{}.{}", nameSpace, name);
			}
		}

		ScriptFieldType MonoTypeToScriptFieldType(MonoType* monoType)
		{
			std::string typeName = mono_type_get_name(monoType);

			auto it = s_ScriptFieldTypeMap.find(typeName);
			if (it == s_ScriptFieldTypeMap.end())
			{
				SHADO_CORE_ERROR("Unknown type: {}", typeName);
				return ScriptFieldType::None;
			}

			return it->second;
		}
	}

	struct ScriptEngineData
	{
		MonoDomain* RootDomain = nullptr;
		MonoDomain* AppDomain = nullptr;

		MonoAssembly* CoreAssembly = nullptr;
		MonoImage* CoreAssemblyImage = nullptr;

		MonoAssembly* AppAssembly = nullptr;
		MonoImage* AppAssemblyImage = nullptr;

		std::filesystem::path CoreAssemblyFilepath;
		std::filesystem::path AppAssemblyFilepath;

		ScriptClass EntityClass;
		ScriptClass EditorClass;

		std::unordered_map<std::string, Ref<ScriptClass>> EntityClasses;
		std::unordered_map<UUID, Ref<ScriptInstance>> EntityInstances;
		std::unordered_map<UUID, ScriptFieldMap> EntityScriptFields;

		std::unordered_map<std::string, Ref<ScriptClass>> EditorClasses;
		std::unordered_map<std::string, Ref<ScriptInstance>> EditorInstances;

		Ref<filewatch::FileWatch<std::string>> AppAssemblyFileWatcher;
		bool AssemblyReloadPending = false;

#ifdef SHADO_DEBUG
		bool EnableDebugging = true;
#else
		bool EnableDebugging = false;
#endif
		// Runtime

		Scene* SceneContext = nullptr;
	};

	static ScriptEngineData* s_Data = nullptr;

	static void OnAppAssemblyFileSystemEvent(const std::string& path, const filewatch::Event change_type)
	{
		if (!s_Data->AssemblyReloadPending && change_type == filewatch::Event::modified)
		{
			s_Data->AssemblyReloadPending = true;

			Application::get().SubmitToMainThread([]()
				{
					s_Data->AppAssemblyFileWatcher.reset();
					ScriptEngine::ReloadAssembly();
				});
		}
	}

	void ScriptEngine::Init()
	{
		s_Data = new ScriptEngineData();

		InitMono();
		ScriptGlue::RegisterFunctions();

		bool status = LoadAssembly("resources/Scripts/Shado-script-core.dll");
		if (!status)
		{
			SHADO_CORE_ERROR("[ScriptEngine] Could not load Shado-ScriptCore assembly.");
			return;
		}

		auto scriptModulePath = Project::GetAssetDirectory() / Project::GetActive()->GetConfig().ScriptModulePath;
		status = LoadAppAssembly(scriptModulePath);
		if (!status)
		{
			SHADO_CORE_ERROR("[ScriptEngine] Could not load app assembly.");
			return;
		}

		LoadAssemblyClasses();

		ScriptGlue::RegisterComponents();

		// Retrieve and instantiate class
		s_Data->EntityClass = ScriptClass("Shado", "Entity", true);
		s_Data->EditorClass = ScriptClass("Shado.Editor", "Editor", true);
	}

	void ScriptEngine::Shutdown()
	{
		ShutdownMono();
		delete s_Data;
	}

	void ScriptEngine::InitMono()
	{
		mono_set_assemblies_path("../mono/lib");

		if (s_Data->EnableDebugging)
		{
			const char* argv[2] = {
				"--debugger-agent=transport=dt_socket,address=127.0.0.1:2550,server=y,suspend=n,loglevel=3,logfile=MonoDebugger.log",
				"--soft-breakpoints"
			};

			mono_jit_parse_options(2, (char**)argv);
			mono_debug_init(MONO_DEBUG_FORMAT_MONO);
		}

		MonoDomain* rootDomain = mono_jit_init("HazelJITRuntime");
		SHADO_CORE_ASSERT(rootDomain, "");

		// Store the root domain pointer
		s_Data->RootDomain = rootDomain;

		if (s_Data->EnableDebugging)
			mono_debug_domain_create(s_Data->RootDomain);

		mono_thread_set_main(mono_thread_current());
	}

	void ScriptEngine::ShutdownMono()
	{
		mono_domain_set(mono_get_root_domain(), false);

		mono_domain_unload(s_Data->AppDomain);
		s_Data->AppDomain = nullptr;

		mono_jit_cleanup(s_Data->RootDomain);
		s_Data->RootDomain = nullptr;
	}

	bool ScriptEngine::LoadAssembly(const std::filesystem::path& filepath)
	{
		// Create an App Domain
		s_Data->AppDomain = mono_domain_create_appdomain("HazelScriptRuntime", nullptr);
		mono_domain_set(s_Data->AppDomain, true);

		s_Data->CoreAssemblyFilepath = filepath;
		s_Data->CoreAssembly = Utils::LoadMonoAssembly(filepath, s_Data->EnableDebugging);
		if (s_Data->CoreAssembly == nullptr)
			return false;

		s_Data->CoreAssemblyImage = mono_assembly_get_image(s_Data->CoreAssembly);
		return true;
	}

	bool ScriptEngine::LoadAppAssembly(const std::filesystem::path& filepath)
	{
		s_Data->AppAssemblyFilepath = filepath;
		s_Data->AppAssembly = Utils::LoadMonoAssembly(filepath, s_Data->EnableDebugging);
		if (s_Data->AppAssembly == nullptr)
			return false;

		s_Data->AppAssemblyImage = mono_assembly_get_image(s_Data->AppAssembly);

		s_Data->AppAssemblyFileWatcher = CreateRef<filewatch::FileWatch<std::string>>(filepath.string(), OnAppAssemblyFileSystemEvent);
		s_Data->AssemblyReloadPending = false;
		return true;
	}

	void ScriptEngine::ReloadAssembly()
	{
		mono_domain_set(mono_get_root_domain(), false);

		mono_domain_unload(s_Data->AppDomain);

		LoadAssembly(s_Data->CoreAssemblyFilepath);
		LoadAppAssembly(s_Data->AppAssemblyFilepath);
		LoadAssemblyClasses();

		ScriptGlue::RegisterComponents();

		// Retrieve and instantiate class
		s_Data->EntityClass = ScriptClass("Shado", "Entity", true);
		s_Data->EditorClass = ScriptClass("Shado.Editor", "Editor", true);
	}

	void ScriptEngine::OnRuntimeStart(Scene* scene)
	{
		s_Data->SceneContext = scene;
	}

	bool ScriptEngine::EntityClassExists(const std::string& fullClassName)
	{
		return s_Data->EntityClasses.find(fullClassName) != s_Data->EntityClasses.end();
	}

	void ScriptEngine::OnCreateEntity(Entity entity)
	{
		const auto& sc = entity.getComponent<ScriptComponent>();
		if (ScriptEngine::EntityClassExists(sc.ClassName))
		{
			UUID entityID = entity.getUUID();

			Ref<ScriptInstance> instance = CreateRef<ScriptInstance>(s_Data->EntityClasses[sc.ClassName], entity);
			s_Data->EntityInstances[entityID] = instance;

			// Copy field values
			if (s_Data->EntityScriptFields.find(entityID) != s_Data->EntityScriptFields.end())
			{
				const ScriptFieldMap& fieldMap = s_Data->EntityScriptFields.at(entityID);
				for (const auto& [name, fieldInstance] : fieldMap)
					instance->SetFieldValueInternal(name, fieldInstance.m_Buffer);
			}

			instance->InvokeOnCreate();
		}
	}

	void ScriptEngine::OnUpdateEntity(Entity entity, TimeStep ts)
	{
		UUID entityUUID = entity.getUUID();
		if (s_Data->EntityInstances.find(entityUUID) != s_Data->EntityInstances.end())
		{
			Ref<ScriptInstance> instance = s_Data->EntityInstances[entityUUID];
			instance->InvokeOnUpdate((float)ts);
		}
		else
		{
			SHADO_CORE_ERROR("Could not find ScriptInstance for entity {}", entityUUID);
		}
	}

	// Called with the scene.OnDrawRuntime
	void ScriptEngine::OnDrawEntity(Entity entity)
	{
		UUID entityUUID = entity.getUUID();
		if (s_Data->EntityInstances.find(entityUUID) != s_Data->EntityInstances.end())
		{
			Ref<ScriptInstance> instance = s_Data->EntityInstances[entityUUID];
			instance->InvokeOnDraw();
		} else
		{
			SHADO_CORE_ERROR("Could not find ScriptInstance for entity {}", entityUUID);
		}
	}

	Scene* ScriptEngine::GetSceneContext()
	{
		return s_Data->SceneContext;
	}

	Ref<ScriptInstance> ScriptEngine::GetEntityScriptInstance(UUID entityID)
	{
		auto it = s_Data->EntityInstances.find(entityID);
		if (it == s_Data->EntityInstances.end())
			return nullptr;

		return it->second;
	}


	Ref<ScriptClass> ScriptEngine::GetEntityClass(const std::string& name)
	{
		if (s_Data->EntityClasses.find(name) == s_Data->EntityClasses.end())
			return nullptr;

		return s_Data->EntityClasses.at(name);
	}

	void ScriptEngine::OnRuntimeStop()
	{
		s_Data->SceneContext = nullptr;

		s_Data->EntityInstances.clear();
	}

	std::unordered_map<std::string, Ref<ScriptClass>> ScriptEngine::GetEntityClasses()
	{
		return s_Data->EntityClasses;
	}

	ScriptFieldMap& ScriptEngine::GetScriptFieldMap(Entity entity)
	{
		SHADO_CORE_ASSERT(entity, "");

		UUID entityID = entity.getUUID();
		return s_Data->EntityScriptFields[entityID];
	}

	
	void ScriptEngine::Helper_ProcessFields(MonoClass* monoClass, const char* className, Ref<ScriptClass> scriptClass) {
		int fieldCount = mono_class_num_fields(monoClass);

		SHADO_CORE_WARN("{} has {} fields:", className, fieldCount);
		void* iterator = nullptr;
		while (MonoClassField* field = mono_class_get_fields(monoClass, &iterator))
		{
			const char* fieldName = mono_field_get_name(field);
			uint32_t flags = mono_field_get_flags(field);
			if (flags & FIELD_ATTRIBUTE_PUBLIC)
			{
				MonoType* type = mono_field_get_type(field);
				ScriptFieldType fieldType = Utils::MonoTypeToScriptFieldType(type);
				SHADO_CORE_WARN("  {} ({})", fieldName, Utils::ScriptFieldTypeToString(fieldType));

				scriptClass->m_Fields[fieldName] = { fieldType, fieldName, field };
			}
		}
	}

	void ScriptEngine::Helper_ProcessClass(const MonoTableInfo* typeDefinitionsTable, int32_t i, MonoClass* entityClass, MonoClass* editorClass, MonoImage* image) {

		uint32_t cols[MONO_TYPEDEF_SIZE];
		mono_metadata_decode_row(typeDefinitionsTable, i, cols, MONO_TYPEDEF_SIZE);

		const char* nameSpace = mono_metadata_string_heap(image, cols[MONO_TYPEDEF_NAMESPACE]);
		const char* className = mono_metadata_string_heap(image, cols[MONO_TYPEDEF_NAME]);
		std::string fullName;
		if (strlen(nameSpace) != 0)
			fullName = fmt::format("{}.{}", nameSpace, className);
		else
			fullName = className;

		MonoClass* monoClass = mono_class_from_name(image, nameSpace, className);

		if (monoClass == nullptr)
			return;

		if (monoClass == entityClass)
			return;

		if (monoClass == editorClass)
			return;

		bool isEntity = mono_class_is_subclass_of(monoClass, entityClass, false);
		if (isEntity) {

			Ref<ScriptClass> scriptClass = CreateRef<ScriptClass>(nameSpace, className);
			s_Data->EntityClasses[fullName] = scriptClass;


			// This routine is an iterator routine for retrieving the fields in a class.
			// You must pass a gpointer that points to zero and is treated as an opaque handle
			// to iterate over all of the elements. When no more values are available, the return value is NULL.
			Helper_ProcessFields(monoClass, className, scriptClass);
		}

		bool isEditor = mono_class_is_subclass_of(monoClass, editorClass, false);
		if (isEditor) {
			bool isCore = image == s_Data->CoreAssemblyImage;
			SHADO_CORE_INFO("{0}, {1}", fullName, isCore);

			Ref<ScriptClass> scriptClass = CreateRef<ScriptClass>(nameSpace, className, isCore);

			Helper_ProcessFields(monoClass, className, scriptClass);

			// Get an instance of the class
			Ref<ScriptInstance> scriptInstance = CreateRef<ScriptInstance>(scriptClass);

			// Call GetTargetType to get the type for
			MonoString* typeFor = (MonoString*)scriptClass->InvokeMethod(
				scriptInstance->GetManagedObject(),
				//scriptClass->GetMethod("GetTargetType", 0)
				mono_class_get_method_from_name(editorClass, "GetTargetType", 0)
			);

			if (typeFor != nullptr) {
				std::string typeForStr = mono_string_to_utf8(typeFor);
				s_Data->EditorClasses[typeForStr] = scriptClass;
				s_Data->EditorInstances[typeForStr] = scriptInstance;
			}
		}
	}

	void ScriptEngine::LoadAssemblyClasses()
	{
		// Process AppAssembly
		s_Data->EntityClasses.clear();

		MonoClass* entityClass = mono_class_from_name(s_Data->CoreAssemblyImage, "Shado", "Entity");
		MonoClass* editorClass = mono_class_from_name(s_Data->CoreAssemblyImage, "Shado.Editor", "Editor");
		
		{
			const MonoTableInfo* typeDefinitionsTable = mono_image_get_table_info(s_Data->AppAssemblyImage, MONO_TABLE_TYPEDEF);
			int32_t numTypes = mono_table_info_get_rows(typeDefinitionsTable);
			for (int32_t i = 0; i < numTypes; i++)
			{
				Helper_ProcessClass(typeDefinitionsTable, i, entityClass, editorClass, s_Data->AppAssemblyImage);
			}
		}

		// Process Core Assembly
		{
			const MonoTableInfo* typeDefinitionsTable = mono_image_get_table_info(s_Data->CoreAssemblyImage, MONO_TABLE_TYPEDEF);
			int32_t numTypes = mono_table_info_get_rows(typeDefinitionsTable);
			for (int32_t i = 0; i < numTypes; i++)
			{
				Helper_ProcessClass(typeDefinitionsTable, i, entityClass, editorClass, s_Data->CoreAssemblyImage);
			}
		}

		//auto& entityClasses = s_Data->EntityClasses;
		//mono_field_get_value()
	}

	MonoString* ScriptEngine::NewString(const char* str)
	{
		return mono_string_new(s_Data->AppDomain, str);
	}

	MonoImage* ScriptEngine::GetCoreAssemblyImage()
	{
		return s_Data->CoreAssemblyImage;
	}


	void ScriptEngine::DrawCustomEditorForFieldRunning(const ScriptField& field, Ref<ScriptInstance> scriptInstance, const std::string& name)
	{
		for (auto [typeFor, editorInstance] : s_Data->EditorInstances) {
			// If we find custom editor, then invoke the OnEditorDraw func
			std::string temp = mono_type_get_name(mono_field_get_type(field.ClassField));
			if (typeFor == temp) {

				// Set the target
				MonoObject* target = scriptInstance->GetFieldValue<MonoObject*>(name);
				mono_field_set_value(editorInstance->GetManagedObject(),
					mono_class_get_field_from_name(s_Data->EditorClass.m_MonoClass, "target"),
					target
				);

				// Set the fieldName
				mono_field_set_value(editorInstance->GetManagedObject(),
					mono_class_get_field_from_name(s_Data->EditorClass.m_MonoClass, "fieldName"),
					ScriptEngine::NewString(name.c_str())
				);

				if (target == nullptr)
					continue;

				editorInstance->GetScriptClass()->InvokeMethod(
					editorInstance->GetManagedObject(),
					editorInstance->GetScriptClass()->GetMethod("OnEditorDraw", 0)
				);
			}
		}
	}


	MonoObject* ScriptEngine::GetManagedInstance(UUID uuid)
	{
		SHADO_CORE_ASSERT(s_Data->EntityInstances.find(uuid) != s_Data->EntityInstances.end(), "");
		return s_Data->EntityInstances.at(uuid)->GetManagedObject();
	}

	MonoObject* ScriptEngine::InstantiateClass(MonoClass* monoClass, MonoMethod* ctor, void** args)
	{
		MonoObject* instance = mono_object_new(s_Data->AppDomain, monoClass);

		if (ctor == nullptr && args == nullptr)
			mono_runtime_object_init(instance);
		else
			mono_runtime_invoke(ctor, instance, args, nullptr);
		return instance;
	}

	ScriptClass::ScriptClass(const std::string& classNamespace, const std::string& className, bool isCore)
		: m_ClassNamespace(classNamespace), m_ClassName(className)
	{
		m_MonoClass = mono_class_from_name(isCore ? s_Data->CoreAssemblyImage : s_Data->AppAssemblyImage,
			m_ClassNamespace.c_str(),
			m_ClassName.c_str());
	}

	ScriptClass::ScriptClass(MonoClass* klass)
		: m_MonoClass(klass)
	{
	}
		
	MonoObject* ScriptClass::Instantiate(MonoMethod* ctor, void** args)
	{
		return ScriptEngine::InstantiateClass(m_MonoClass, ctor, args);
	}

	MonoMethod* ScriptClass::GetMethod(const std::string& name, int parameterCount)
	{
		return mono_class_get_method_from_name(m_MonoClass, name.c_str(), parameterCount);
	}

	MonoObject* ScriptClass::InvokeMethod(MonoObject* instance, MonoMethod* method, void** params)
	{
		MonoObject* exception = nullptr;
		return mono_runtime_invoke(method, instance, params, &exception);
	}

	ScriptInstance::ScriptInstance(Ref<ScriptClass> scriptClass, Entity entity)
		: m_ScriptClass(scriptClass)
	{
		m_Instance = scriptClass->Instantiate();

		m_Constructor = s_Data->EntityClass.GetMethod(".ctor", 1);
		m_OnCreateMethod = scriptClass->GetMethod("OnCreate", 0);
		m_OnUpdateMethod = scriptClass->GetMethod("OnUpdate", 1);
		m_OnDrawMethod = scriptClass->GetMethod("OnDraw", 0);
		m_OnDestroyedMethod = scriptClass->GetMethod("OnDestroy", 0);

		// Call Entity constructor
		{
			UUID entityID = entity.getUUID();
			void* param = &entityID;
			m_ScriptClass->InvokeMethod(m_Instance, m_Constructor, &param);
		}
	}

	ScriptInstance::ScriptInstance(Ref<ScriptClass> scriptClass)
		: m_ScriptClass(scriptClass)
	{
		m_Instance = scriptClass->Instantiate();
	}

	void ScriptInstance::InvokeOnCreate()
	{
		if (m_OnCreateMethod)
			m_ScriptClass->InvokeMethod(m_Instance, m_OnCreateMethod);
	}

	void ScriptInstance::InvokeOnUpdate(float ts)
	{
		if (m_OnUpdateMethod)
		{
			void* param = &ts;
			m_ScriptClass->InvokeMethod(m_Instance, m_OnUpdateMethod, &param);
		}
	}

	void ScriptInstance::InvokeOnDraw() {
		if (m_OnDrawMethod)
		{
			m_ScriptClass->InvokeMethod(m_Instance, m_OnDrawMethod);
		}
	}

	bool ScriptInstance::GetFieldValueInternal(const std::string& name, void* buffer)
	{
		const auto& fields = m_ScriptClass->GetFields();
		auto it = fields.find(name);
		if (it == fields.end())
			return false;

		const ScriptField& field = it->second;
		mono_field_get_value(m_Instance, field.ClassField, buffer);
		return true;
	}

	bool ScriptInstance::SetFieldValueInternal(const std::string& name, const void* value)
	{
		const auto& fields = m_ScriptClass->GetFields();
		auto it = fields.find(name);
		if (it == fields.end())
			return false;

		const ScriptField& field = it->second;
		mono_field_set_value(m_Instance, field.ClassField, (void*)value);
		return true;
	}

}
