#include "SceneHierarchyPanel.h"
#include "scene/Components.h"
#include <glm/gtc/type_ptr.hpp>
#include "imgui_internal.h"
#include "scene/utils/SceneUtils.h"
#include "debug/Profile.h"
#include <fstream>

namespace Shado {
	extern const std::filesystem::path g_AssetsPath;

	SceneHierarchyPanel::SceneHierarchyPanel(const Ref<Scene>& scene) {
		setContext(scene);
	}

	void SceneHierarchyPanel::setSelected(Entity entity) {
		m_Selected = entity;
	}

	void SceneHierarchyPanel::setContext(const Ref<Scene>& scene) {
		m_Context = scene;
		m_Selected = {};
	}

	void SceneHierarchyPanel::onImGuiRender() {
		SHADO_PROFILE_FUNCTION();

		ImGui::Begin("Scene Hierachy");

		if (m_Context) {

			m_Context->m_Registry.each([this](auto entityID) {
				Entity entity = { entityID, m_Context.get() };
				drawEntityNode(entity);
			});

			if (ImGui::IsMouseDown(0) && ImGui::IsWindowHovered()) {
				m_Selected = {};
			}

			// Context menu to create 
			if (ImGui::BeginPopupContextWindow(0, 1, false)) {
				if (ImGui::MenuItem("Create empty Entity"))
					m_Context->createEntity("Empty Entity");

				ImGui::EndPopup();
			}
		}
		ImGui::End();

		ImGui::Begin("Properties");
		if (m_Selected) {
			drawComponents(m_Selected);
		}
		
		ImGui::End();
	}

	void SceneHierarchyPanel::resetSelection() {
		m_Selected = {};
	}

	void SceneHierarchyPanel::drawEntityNode(Entity entity) {
		SHADO_PROFILE_FUNCTION();

		TagComponent& tc = entity.getComponent<TagComponent>();
		
		auto flags = ImGuiTreeNodeFlags_OpenOnArrow | (m_Selected == entity ? ImGuiTreeNodeFlags_Selected : 0);
		flags |= ImGuiTreeNodeFlags_SpanAvailWidth;

		bool opened = ImGui::TreeNodeEx((void*)(uint64_t)(uint32_t)entity, flags, tc.tag.c_str());
		if (ImGui::IsItemClicked()) {
			m_Selected = entity;
		}

		// Context menu item
		bool deleteEntity = false;
		if (ImGui::BeginPopupContextItem()) {
			if (ImGui::MenuItem("Duplicate Entity"))
				m_Context->duplicateEntity(m_Selected);

			if (ImGui::MenuItem("Delete Entity"))
				deleteEntity = true;			

			ImGui::EndPopup();
		}

		if (opened) {
			ImGui::TreePop();
		}

		// Defer the entity deletion to avoid bugs
		if (deleteEntity) {
			m_Context->destroyEntity(entity);
			if (m_Selected == entity)
				m_Selected = {};
		}
	}

	// Helpers
	template<typename T>
	static void drawComponent(const char* label, Entity entity, std::function<void(T&)> ui, bool allowDeletion = true);
	static void drawVec3Control(const std::string& label, glm::vec3& values, float resetValue = 0.0f, float columnWidth = 100.0f);
	static void addComponentContextMenu(Entity m_Selected, uint32_t vpWidth, uint32_t vpHeight);
	static void drawTextureControl(void* spriteData, const std::string& type = "Quad");
	static void generateShaderFile(const std::string& path, const std::string& type);

	void SceneHierarchyPanel::drawComponents(Entity entity) {
		SHADO_PROFILE_FUNCTION();

		drawComponent<TagComponent>("Tag", entity, [](TagComponent& tc) {
			char buffer[512];
			memset(buffer, 0, sizeof(buffer));
			strcpy_s(buffer, sizeof(buffer), tc.tag.c_str());
			if (ImGui::InputText("##Tag", buffer, sizeof(buffer))) {
				tc.tag = std::string(buffer);
			}
		}, false);

		// Add component context menu
		addComponentContextMenu(m_Selected, m_Context->m_ViewportWidth, m_Context->m_ViewportHeight);

		drawComponent<TransformComponent>("Transform", entity, [](TransformComponent& transform) {
			drawVec3Control("Position", transform.position);

			auto rotation = glm::degrees(transform.rotation);
			drawVec3Control("Rotation", rotation);
			transform.rotation = glm::radians(rotation);

			drawVec3Control("Scale", transform.scale, 1.0);
		}, false);

		drawComponent<SpriteRendererComponent>("Sprite", entity, [](SpriteRendererComponent& sprite) {
			ImGui::ColorEdit4("Colour", glm::value_ptr(sprite.color));
			drawTextureControl(&sprite);
		});

		drawComponent<CircleRendererComponent>("Circle Renderer", entity, [](CircleRendererComponent& circle) {
			ImGui::ColorEdit4("Colour", glm::value_ptr(circle.color));
			ImGui::DragFloat("Thickness", &circle.thickness, 0.05);
			ImGui::DragFloat("Fade", &circle.fade, 0.01);

			drawTextureControl(&circle, "Circle");
		});

		drawComponent<CameraComponent>("Camera", entity, [](CameraComponent& camera) {
			// Select menu to change camera type
			const char* projectionTypesString[] = { "Orthographic", "Orbit (Perspective)" };
			const char* currentProjectionType = projectionTypesString[(int)camera.type];
			if (ImGui::BeginCombo("Type", currentProjectionType)) {

				for (int i = 0; i < 2; i++) {
					bool isSelected = currentProjectionType == projectionTypesString[i];

					if (ImGui::Selectable(projectionTypesString[i], isSelected)) {
						currentProjectionType = projectionTypesString[i];

						// Change camera type
						camera.setType((CameraComponent::Type)i);
					}

					if (isSelected)
						ImGui::SetItemDefaultFocus();
				}

				ImGui::EndCombo();
			}

			if (camera.type == CameraComponent::Type::Orthographic) {
				float size = camera.getSize();
				if (ImGui::DragFloat("Size", &size)) {
					camera.setSize(size);
				}
			}

			if (camera.type == CameraComponent::Type::Orbit) {
				OrbitCamera* cam = (OrbitCamera*)camera.camera.get();

				float fov = cam->getFOV();
				if (ImGui::DragFloat("FOV", &fov)) {
					cam->setFOV(fov);
				}
			}

			float nearClip = camera.camera->getNearClip(), farClip = camera.camera->getFarClip();
			if (ImGui::DragFloat("Near clip", &nearClip, 0.5f, camera.type == CameraComponent::Type::Orbit ? 1.0f : -30.0, 1000)) {
				camera.camera->setNearClip(nearClip);
			}

			if (ImGui::DragFloat("Far clip", &farClip)) {
				camera.camera->setFarClip(farClip);
			}

			ImGui::Checkbox("Primary", &camera.primary);
			ImGui::Checkbox("Fixed aspect ratio", &camera.fixedAspectRatio);
		});

		drawComponent<RigidBody2DComponent>("Rigid Body 2D", entity, [](RigidBody2DComponent& rb) {

			// Select menu to change body type
			const char* bodyTypes[] = { "Static", "Kinematic", "Dynamic"};
			const char* currentBodyType = bodyTypes[(int)rb.type];
			if (ImGui::BeginCombo("Type", currentBodyType)) {

				for (int i = 0; i < 3; i++) {
					bool isSelected = currentBodyType == bodyTypes[i];

					if (ImGui::Selectable(bodyTypes[i], isSelected)) {
						currentBodyType = bodyTypes[i];

						// Change camera type
						rb.type = (RigidBody2DComponent::BodyType)i;
					}

					if (isSelected)
						ImGui::SetItemDefaultFocus();
				}

				ImGui::EndCombo();
			}

			ImGui::Checkbox("Fixed rotation", &rb.fixedRotation);
		});

		drawComponent<BoxCollider2DComponent>("Box Collider 2D", entity, [](BoxCollider2DComponent& bc) {

			ImGui::DragFloat2("Size modifier", glm::value_ptr(bc.size), 0.1f);
			ImGui::DragFloat("Density", &bc.density, 0.01f, 0, 1.0f);
			ImGui::DragFloat("Friction", &bc.friction, 0.01f, 0, 1.0f);

			ImGui::Separator();
			ImGui::DragFloat("Restitution", &bc.restitution, 0.01f, 0);
			ImGui::DragFloat("Restitution Threshold", &bc.restitutionThreshold, 0.01f, 0);
		});

		drawComponent<CircleCollider2DComponent>("Circle Collider 2D", entity, [](CircleCollider2DComponent& bc) {

			ImGui::DragFloat("Radius modifier", &bc.radius.x, 0.1f);
			ImGui::DragFloat("Density", &bc.density, 0.01f, 0, 1.0f);
			ImGui::DragFloat("Friction", &bc.friction, 0.01f, 0, 1.0f);

			ImGui::Separator();
			ImGui::DragFloat("Restitution", &bc.restitution, 0.01f, 0);
			ImGui::DragFloat("Restitution Threshold", &bc.restitutionThreshold, 0.01f, 0);
		});

		drawComponent<ScriptComponent>("C# Script", entity, [](ScriptComponent& sc) {

			static char buffer[64];
			strcpy(buffer, sc.className.c_str());
			if(ImGui::InputText("Class", buffer, sizeof(buffer))) {

				// Check if the class is valid
				if(ScriptManager::hasClass(buffer)) {
					sc.className = buffer;
					sc.klass = ScriptManager::getClassByName(buffer);
				} else {
					SHADO_CORE_ERROR("Invalid class name {0}", buffer);
				}
			}

			ImGui::Text("Class name %s", sc.className.c_str());

			ImGui::Text("Fields");
			for(auto& fields : sc.object.getDescription().fields) {
				ImGui::Text("%s", fields.first.c_str());
			}

		});
	}
	
	template<typename T>
	static void drawComponent(const char* label, Entity entity, std::function<void(T&)> ui, bool allowDeletion) {
		SHADO_PROFILE_FUNCTION();

		const ImGuiTreeNodeFlags treeNodeFlags = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_AllowItemOverlap
				| ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_FramePadding
				| ImGuiTreeNodeFlags_AllowItemOverlap;

		if (entity.hasComponent<T>()) {
			auto& component = entity.getComponent<T>();
			ImVec2 contentRegionAvail = ImGui::GetContentRegionAvail();

			ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, { 4, 4 });
			float lineHeight = GImGui->Font->FontSize + GImGui->Style.FramePadding.y * 2.0f;
			ImGui::Separator();

			bool open = ImGui::TreeNodeEx((void*)typeid(T).hash_code(), treeNodeFlags, label);

			ImGui::PopStyleVar();

			// Component settings menu
			if (allowDeletion) {
				ImGui::SameLine(contentRegionAvail.x - lineHeight * 0.5f);
				if (ImGui::Button("...", { lineHeight, lineHeight })) {
					ImGui::OpenPopup("ComponentSettings");
				}
			}
			

			bool removeComponent = false;
			if (allowDeletion && ImGui::BeginPopup("ComponentSettings")) {

				if (ImGui::MenuItem("Remove component"))
					removeComponent = true;

				ImGui::EndPopup();
			}

			if (open) {
				ui(component);
				ImGui::TreePop();
			}

			if (removeComponent) {
				entity.removeComponent<T>();
			}
		}
	}

	// Draw a vector with colourful x, y and z
	static void drawVec3Control(const std::string& label, glm::vec3& values, float resetValue, float columnWidth) {
		ImGuiIO& io = ImGui::GetIO();
		auto boldFont = io.Fonts->Fonts[ImguiLayer::FontIndex::BOLD];

		ImGui::PushID(label.c_str());

		ImGui::Columns(2);
		ImGui::SetColumnWidth(0, columnWidth);
		ImGui::Text(label.c_str());
		ImGui::NextColumn();

		ImGui::PushMultiItemsWidths(3, ImGui::CalcItemWidth());
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { 0, 0 });

		float lineHeight = GImGui->Font->FontSize + GImGui->Style.FramePadding.y * 2.0f;
		ImVec2 buttonSize = { lineHeight + 3.0f, lineHeight };

		ImGui::PushStyleColor(ImGuiCol_Button, { 0.8f, 0.1f, 0.15f, 1.0f });
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, { 0.9f, 0.2f, 0.2f, 1.0f });
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, { 0.8f, 0.1f, 0.15f, 1.0f });
		ImGui::PushFont(boldFont);
		if (ImGui::Button("X", buttonSize))
			values.x = resetValue;
		ImGui::PopFont();
		ImGui::PopStyleColor(3);
		

		ImGui::SameLine();
		ImGui::DragFloat("##X", &values.x, 0.1f, 0, 0, "%.2f");
		ImGui::PopItemWidth();
		ImGui::SameLine();


		ImGui::PushStyleColor(ImGuiCol_Button, { 0.2f, 0.7f, 0.2f, 1.0f });
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, { 0.3f, 0.8f, 0.3f, 1.0f });
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, { 0.2f, 0.7f, 0.2f, 1.0f });
		ImGui::PushFont(boldFont);
		if (ImGui::Button("Y", buttonSize))
			values.y = resetValue;
		ImGui::PopFont();
		ImGui::PopStyleColor(3);


		ImGui::SameLine();
		ImGui::DragFloat("##Y", &values.y, 0.1f, 0, 0, "%.2f");
		ImGui::PopItemWidth();
		ImGui::SameLine();


		ImGui::PushStyleColor(ImGuiCol_Button, { 0.1f, 0.25f, 0.8f, 1.0f });
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, { 0.2f, 0.35f, 0.9f, 1.0f });
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, { 0.1f, 0.25f, 0.8f, 1.0f });
		ImGui::PushFont(boldFont);
		if (ImGui::Button("Z", buttonSize))
			values.z = resetValue;
		ImGui::PopFont();
		ImGui::PopStyleColor(3);

		ImGui::SameLine();
		ImGui::DragFloat("##Z", &values.z, 0.1f, 0, 0, "%.2f");
		ImGui::PopItemWidth();

		ImGui::PopStyleVar();

		ImGui::Columns(1);

		ImGui::PopID();
	}

	static void drawInputTextWithChooseFile(
		const std::string label, const std::string& text, const std::vector<std::string>& dragAndDropExtensions, int id,
		std::function<void(std::string)> onChange
	) {
		bool textureChanged = false;
		ImGui::InputText(label.c_str(), (char*)text.c_str(), text.length(), ImGuiInputTextFlags_ReadOnly);
		std::string texturePath;

		// For drag and drop
		if (ImGui::BeginDragDropTarget())
		{
			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM"))
			{
				const wchar_t* path = (const wchar_t*)payload->Data;
				std::filesystem::path dataPath = std::filesystem::path(g_AssetsPath) / path;

				bool acceptable = dragAndDropExtensions.empty();
				for (const auto& ext : dragAndDropExtensions) {
					if (ext == dataPath.extension()) {
						acceptable = true;
						break;
					}
				}

				if (acceptable)
					onChange(dataPath.string());
				else
					SHADO_CORE_WARN("Invalid drag and drop file extension {0}", dataPath.filename());
			}

			ImGui::EndDragDropTarget();
		}

		// File choose
		ImGui::PushID(id);
		ImGui::SameLine();
		if (ImGui::Button("...", { 24, 24 })) {

			std::string buffer = "";
			int count = 0;
			for (const auto& ext : dragAndDropExtensions) {
				buffer += "*" + ext;

				if (count != dragAndDropExtensions.size() - 1)
					buffer += ";";
				count++;
			}

			// Need to do this because we have \0 in string body
			std::string filter = "Files (";
			filter += std::string((buffer + ")\0").c_str(), buffer.length() + 2);
			filter += std::string((buffer + "\0").c_str(), buffer.length() + 1);

			texturePath = FileDialogs::openFile(filter.c_str());
			textureChanged = true;
		}
		ImGui::PopID();


		if (textureChanged && !texturePath.empty())
			onChange(texturePath);
	}

	static void addComponentContextMenu(Entity m_Selected, uint32_t vpWidth, uint32_t vpHeight) {
		ImGui::SameLine();
		ImGui::PushItemWidth(-1);
		// To Add components context menu
		if (ImGui::Button("Add Component"))
			ImGui::OpenPopup("AddComponent");

		if (ImGui::BeginPopup("AddComponent")) {
			if (!m_Selected.hasComponent<SpriteRendererComponent>() && ImGui::MenuItem("Sprite renderer")) {
				m_Selected.addComponent<SpriteRendererComponent>();
				ImGui::CloseCurrentPopup();
			}

			if (!m_Selected.hasComponent<CircleRendererComponent>() && ImGui::MenuItem("Circle renderer")) {
				m_Selected.addComponent<CircleRendererComponent>();
				ImGui::CloseCurrentPopup();
			}

			if (!m_Selected.hasComponent<RigidBody2DComponent>() &&  ImGui::MenuItem("Rigid Body 2D")) {
				m_Selected.addComponent<RigidBody2DComponent>();
				ImGui::CloseCurrentPopup();
			}

			if (!m_Selected.hasComponent<BoxCollider2DComponent>() &&  ImGui::MenuItem("Box Collider 2D")) {
				m_Selected.addComponent<BoxCollider2DComponent>();
				ImGui::CloseCurrentPopup();
			}

			if (!m_Selected.hasComponent<CircleCollider2DComponent>() && ImGui::MenuItem("Circle Collider 2D")) {
				m_Selected.addComponent<CircleCollider2DComponent>();
				ImGui::CloseCurrentPopup();
			}

			if (!m_Selected.hasComponent<CameraComponent>() &&  ImGui::MenuItem("Camera")) {
				m_Selected.addComponent<CameraComponent>(CameraComponent::Type::Orthographic, vpWidth, vpHeight);
				ImGui::CloseCurrentPopup();
			}

			if (!m_Selected.hasComponent<NativeScriptComponent>() && ImGui::MenuItem("Native script")) {
				m_Selected.addComponent<NativeScriptComponent>();
				ImGui::CloseCurrentPopup();
			}

			if (!m_Selected.hasComponent<ScriptComponent>() && ImGui::MenuItem("Script component")) {
				m_Selected.addComponent<ScriptComponent>();
				ImGui::CloseCurrentPopup();
			}

			ImGui::EndPopup();
		}
		ImGui::PopItemWidth();
	}

	static void drawTextureControl(void* spriteData, const std::string& type ) {
		SpriteRendererComponent& sprite = *(SpriteRendererComponent*)spriteData;

		// =========== Texture
		std::string texturePath = sprite.texture ? sprite.texture->getFilePath().c_str() : "No Texture";

		drawInputTextWithChooseFile("Texture", texturePath, {".jpg", ".png"}, typeid(sprite.texture).hash_code(),
			[&](std::string path) {
				Ref<Texture2D> texture = CreateRef<Texture2D>(path);
				if (texture->isLoaded())
					sprite.texture = texture;
				else
					SHADO_CORE_WARN("Could not load texture {0}", path);
				SHADO_CORE_INFO("Loaded texture {0}", path.c_str());
			}		
		);

		// Image
		if (sprite.texture) {
			ImGui::Image((void*)sprite.texture->getRendererID(), { 60, 60 }, ImVec2(0, 1), ImVec2(1, 0));
		}

		// =========== Tilling factor
		ImGui::DragFloat("Tilling factor", &sprite.tilingFactor, 0.01);

		ImGui::Separator();

		// =========== Shader
		// Create Shader file
		// only for quad for now
		if (type != "Quad")
			return;

		std::string shaderPath = sprite.shader ? sprite.shader->getName().c_str() : "Default Shader";
		drawInputTextWithChooseFile("Shader", shaderPath, { ".glsl", ".shader" }, typeid(sprite.shader).hash_code(),
			[&](std::string path) {
				sprite.shader = CreateRef<Shader>(path);
			}
		);

		if (ImGui::Button("+")) {
			std::string path = FileDialogs::saveFile("Shader file (*glsl)\0*.glsl\0");
			if (!path.empty()) {
				generateShaderFile(path, type);
				Dialog::openPathInExplorer(path);
			}
		}

		if (ImGui::Button("Recompile")) {
			if (sprite.shader) {
				sprite.shader = CreateRef<Shader>(sprite.shader->getFilepath());
			}
		}
	}

	static void generateShaderFile(const std::string& path, const std::string& type) {

		auto filepath = type == "Circle" ? CIRCLE_SHADER : QUAD_SHADER;

		std::ifstream file(filepath);
		std::string shader;
		if (file) {
			shader = std::string((std::istreambuf_iterator<char>(file)),
				std::istreambuf_iterator<char>());
			file.close();
		}
		else {
			SHADO_CORE_ERROR("Failed to generate shader file!");
			return;
		}

		std::ofstream output(path);
		output << shader << std::endl;
		output.close();
	}
}
