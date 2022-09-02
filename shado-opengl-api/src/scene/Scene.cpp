#include "Scene.h"
#include "Components.h"
#include "renderer/Renderer2D.h"
#include "Entity.h"
#include <box2d/b2_world.h>
#include <mono/metadata/appdomain.h>
#include <mono/metadata/object.h>

#include "box2d/b2_body.h"
#include "box2d/b2_fixture.h"
#include "box2d/b2_polygon_shape.h"
#include "box2d/b2_circle_shape.h"
#include "box2d/b2_contact.h"

namespace Shado {
	/**
	 * **********************************
	 ************************************
	 * **********************************
	 */
	 // This is here to avoid havning to include box2d in header files
	class Physics2DCallback : public b2ContactListener {
	private:
		struct Collision2DInfo {
			glm::vec2 normal;
			MonoArray* points;
			MonoArray* separations;
		};
	public:
		Physics2DCallback(Scene* scene): scene(scene) {}

		void BeginContact(b2Contact* contact) override {
			invokeCollisionFunction(contact, "OnCollision2DEnter");
		}
		void EndContact(b2Contact* contact) override {
			invokeCollisionFunction(contact, "OnCollision2DLeave");
		}

	private:
		Collision2DInfo buildContactInfoObject(b2Contact* contact) {

			b2WorldManifold manifold;
			contact->GetWorldManifold(&manifold);

			// Create C# object
			glm::vec2 normal = { manifold.normal.x, manifold.normal.y};

			// Create points array
			static auto vector2f = ScriptManager::getClassByName("Vector2");
			auto* pointsCS = ScriptManager::createArray(vector2f, 2);

			ScriptClassDesc desc;		// Avoid creating and populating the whole object
			desc.klass = mono_get_single_class();
			auto* separationsCS = ScriptManager::createArray(desc, 2);

			for(int i = 0; i < 2; i++) {
				glm::vec2 points = { manifold.points[i].x, manifold.points[i].y };
				mono_array_set(pointsCS, glm::vec2, i, points);
			}

			for (int i = 0; i < 2; i++) {
				mono_array_set(separationsCS, float, i, manifold.separations[i]);
			}

			Collision2DInfo result = {
				normal,
				pointsCS,
				separationsCS
			};

			return result;
		}

		void invokeCollisionFunction(b2Contact* contact, const std::string& functionName) {
			// Entity A
			uint64_t idA = contact->GetFixtureA()->GetBody()->GetUserData().pointer;
			uint64_t idB = contact->GetFixtureB()->GetBody()->GetUserData().pointer;

			Entity entityA = scene->getEntityById(idA);
			Entity entityB = scene->getEntityById(idB);

			// Call the Entity::OnCollision2D in C#
			if (!ScriptManager::hasInit())
				return;

			const auto entityClassCSharp = ScriptManager::getClassByName("Entity");
			if (entityA.isValid() && entityA.hasComponent<ScriptComponent>()) {
				auto& script = entityA.getComponent<ScriptComponent>();
				auto* OnFunc = script.object.getMethod(functionName);

				if (OnFunc) 
				{
					auto entityBCSharp = ScriptManager::createEntity(entityClassCSharp, idB, scene);
					auto collisionInfo = buildContactInfoObject(contact);
					void* args[] = {
						&collisionInfo,
						entityBCSharp.getNative()
					};
					script.object.invokeMethod(OnFunc, args);
				}
					
			}

			if (entityB.isValid() && entityB.hasComponent<ScriptComponent>()) {
				auto& script = entityB.getComponent<ScriptComponent>();
				auto* OnFunc = script.object.getMethod(functionName);

				if (OnFunc) {

					auto entityACSharp = ScriptManager::createEntity(entityClassCSharp, idA, scene);
					auto collisionInfo = buildContactInfoObject(contact);
					void* args[] = {
						&collisionInfo,
						entityACSharp.getNative()
					};
					script.object.invokeMethod(OnFunc, args);
				}
			}
		}
	private:
		Scene* scene;
	};

	/**
	 * **********************************
	 ************ SCENE CLASS ***********
	 * **********************************
	 */
	static Physics2DCallback* s_physics2DCallback = nullptr;

	template<typename Component>
	static void CopyComponent(entt::registry& dst, entt::registry& src, const std::unordered_map<UUID, entt::entity>& enttMap);
	template<typename Component>
	static void CopyComponentIfExists(Entity dst, Entity src);

	Scene::Scene() {
	}

	Scene::Scene(Scene& other)
	{
		m_ViewportWidth = other.m_ViewportWidth;
		m_ViewportHeight = other.m_ViewportHeight;
		name = other.name + " [Runtime]";

		auto& srcSceneRegistry = other.m_Registry;
		auto& dstSceneRegistry = m_Registry;
		std::unordered_map<UUID, entt::entity> enttMap;

		auto idView = srcSceneRegistry.view<IDComponent>();
		for(auto e : idView) {
			UUID uuid = srcSceneRegistry.get<IDComponent>(e).id;
			const auto& tag = srcSceneRegistry.get<TagComponent>(e).tag;

			Entity entity = createEntityWithUUID(tag, uuid);
			enttMap[uuid] = (entt::entity)entity;
		}

		// Copy components (except ID Component and Tag component)
		CopyComponent<TransformComponent>(dstSceneRegistry, srcSceneRegistry, enttMap);
		CopyComponent<SpriteRendererComponent>(dstSceneRegistry, srcSceneRegistry, enttMap);
		CopyComponent<CircleRendererComponent>(dstSceneRegistry, srcSceneRegistry, enttMap);
		CopyComponent<CameraComponent>(dstSceneRegistry, srcSceneRegistry, enttMap);
		CopyComponent<NativeScriptComponent>(dstSceneRegistry, srcSceneRegistry, enttMap);
		CopyComponent<RigidBody2DComponent>(dstSceneRegistry, srcSceneRegistry, enttMap);
		CopyComponent<BoxCollider2DComponent>(dstSceneRegistry, srcSceneRegistry, enttMap);
		CopyComponent<CircleCollider2DComponent>(dstSceneRegistry, srcSceneRegistry, enttMap);
		CopyComponent<ScriptComponent>(dstSceneRegistry, srcSceneRegistry, enttMap);
	}

	Scene::~Scene() {
	}

	Entity Scene::createEntity(const std::string& name) {
		return createEntityWithUUID(name, UUID());
	}

	Entity Scene::createEntityWithUUID(const std::string& name, Shado::UUID uuid) {
		entt::entity id = m_Registry.create();
		Entity entity = { id, this };

		entity.addComponent<IDComponent>().id = uuid;
		entity.addComponent<TransformComponent>();

		auto& tag = entity.addComponent<TagComponent>();
		tag.tag = name.empty() ? std::string("Entity ") + std::to_string((uint64_t)uuid) : name;

		return entity;
	}

	Entity Scene::duplicateEntity(Entity source) {
		if (!source)
			return {};

		Entity newEntity = createEntity(source.getComponent<TagComponent>().tag + " (2)");

		CopyComponentIfExists<TransformComponent>(newEntity, source);
		CopyComponentIfExists<SpriteRendererComponent>(newEntity, source);
		CopyComponentIfExists<CircleRendererComponent>(newEntity, source);
		CopyComponentIfExists<CameraComponent>(newEntity, source);
		CopyComponentIfExists<NativeScriptComponent>(newEntity, source);
		CopyComponentIfExists<RigidBody2DComponent>(newEntity, source);
		CopyComponentIfExists<BoxCollider2DComponent>(newEntity, source);
		CopyComponentIfExists<CircleCollider2DComponent>(newEntity, source);

		return newEntity;
	}

	void Scene::destroyEntity(Entity entity) {
		if (!entity)
			return;

		// If currently inside animation
		if (m_World && m_World->IsLocked()) {
			toDestroy.push_back(entity);
			return;
		}

		// Destroy physics
		if (m_World) {

			if (entity.hasComponent<RigidBody2DComponent>()) {
				auto& rb = entity.getComponent<RigidBody2DComponent>();
				auto* body = (b2Body*)rb.runtimeBody;

				if (entity.hasComponent<BoxCollider2DComponent>()) {
					auto& bc = entity.getComponent<BoxCollider2DComponent>();
					body->DestroyFixture((b2Fixture*)bc.runtimeFixture);
				}				
				if (entity.hasComponent<CircleCollider2DComponent>()) {
					auto& bc = entity.getComponent<CircleCollider2DComponent>();
					body->DestroyFixture((b2Fixture*)bc.runtimeFixture);
				}
				
				m_World->DestroyBody((b2Body*)rb.runtimeBody);				
			}
	
		}

		m_Registry.destroy(entity);
	}

	void Scene::onRuntimeStart() {
		// Init all Script classes attached to entities
		// for (auto& temp : ScriptManager::getAssemblyClassList()) {
		// 	SHADO_CORE_INFO("{0}", temp.toString());
		// }

		auto view = m_Registry.view<ScriptComponent>();
		for (auto e : view) {
			Entity entity = { e, this };

			UUID id = entity.getComponent<IDComponent>().id;
			auto& script = entity.getComponent<ScriptComponent>();

			// Create object based on class
			script.object = ScriptManager::createEntity(script.klass, id, this);			
			auto* create = script.object.getMethod("OnCreate");

			if (create != nullptr)
				script.object.invokeMethod(create);
		}

		// TODO make the physics adjustable
		softResetPhysics();
	}

	void Scene::onRuntimeStop() {
		auto view = m_Registry.view<ScriptComponent>();
		for (auto e : view) {
			Entity entity = { e, this };

			auto& script = entity.getComponent<ScriptComponent>();

			auto* destroy = script.object.getMethod("OnDestroyed");
			if (destroy != nullptr)
				script.object.invokeMethod(destroy);
		}

		ScriptManager::requestThreadsDump();
		
		delete m_World;
		m_World = nullptr;

		delete s_physics2DCallback;
		s_physics2DCallback = nullptr;
	}

	void Scene::onUpdateRuntime(TimeStep ts) {
		// Update Native script
		{
			m_Registry.view<NativeScriptComponent>().each([this, ts](auto entity, NativeScriptComponent& nsc) {
				// TODO: mave to onScenePlay
				if (!nsc.script) {
					nsc.script = nsc.instantiateScript();
					nsc.script->m_EntityHandle = entity;
					nsc.script->m_Scene = this;
					nsc.script->onCreate();					
				}

				nsc.script->onUpdate(ts);
			});
		}

		// Update physics
		if (m_PhysicsEnabled) {
			const int32_t velocityIterations = 6;
			const int32_t positionIterations = 2;
			m_World->Step(ts, velocityIterations, positionIterations);

			// Get transforms from box 2d
			auto view = m_Registry.view<RigidBody2DComponent>();
			for (auto e : view) {
				Entity entity = { e, this };

				auto& transform = entity.getComponent<TransformComponent>();
				auto& rb2D = entity.getComponent<RigidBody2DComponent>();

				b2Body* body = (b2Body*)rb2D.runtimeBody;
				transform.position.x = body->GetPosition().x;
				transform.position.y = body->GetPosition().y;
				transform.rotation.z = body->GetAngle();
			}
		}

		// After world has update delete all entities that need to be deleted
		if (toDestroy.size() > 0) {
			for (auto& to_delete : toDestroy) {
				destroyEntity(to_delete);
			}
			toDestroy.clear();
		}


		// Update C# script
		auto scriptView = m_Registry.view<ScriptComponent>();
		for (auto e : scriptView) {
			Entity entity = { e, this };

			auto& script = entity.getComponent<ScriptComponent>();
			void* args[] = {
				&ts
			};

			auto* update = script.object.getMethod("OnUpdate");
			if (update != nullptr)
				script.object.invokeMethod(update, args);
		}
	}

	void Scene::onDrawRuntime() {

		// Render 2D: Cameras
		Camera* primaryCamera = nullptr;
		glm::mat4 cameraTransform;
		{
			// Loop through ortho cameras
			auto group = m_Registry.view<TransformComponent, CameraComponent>();
			for (auto entity : group) {
				auto [transform, camera] = group.get<TransformComponent, CameraComponent>(entity);

				if (camera.primary) {
					primaryCamera = camera.camera.get();
					cameraTransform = transform.getTransform();
					break;
				}
			}
		}

		static EditorCamera camera;
		if (primaryCamera) {
			Renderer2D::BeginScene(*primaryCamera, cameraTransform);

			// Render stuff
			auto group = m_Registry.group<TransformComponent>(entt::get<SpriteRendererComponent>);
			for (auto entity : group) {
				auto [transform, sprite] = group.get<TransformComponent, SpriteRendererComponent>(entity);

				if (!sprite.shader)
					Renderer2D::DrawSprite(transform.getTransform(), sprite, (int)entity);
				else
					Renderer2D::DrawQuad(transform.getTransform(), sprite.shader, sprite.color, (int)entity);
			}

			// Draw circles
			{
				auto view = m_Registry.view<TransformComponent, CircleRendererComponent>();
				for (auto entity : view)
				{
					auto [transform, circle] = view.get<TransformComponent, CircleRendererComponent>(entity);

					if (circle.texture)
						Renderer2D::DrawCircle(transform.getTransform(), circle.texture, circle.tilingFactor, circle.color, circle.thickness, circle.fade, (int)entity);
					else
						Renderer2D::DrawCircle(transform.getTransform(), circle.color, circle.thickness, circle.fade, (int)entity);
				}
			}

			Renderer2D::EndScene();
		}	
	}

	void Scene::onUpdateEditor(TimeStep ts, EditorCamera& camera) {
	}

	void Scene::onDrawEditor(EditorCamera& camera) {

		Renderer2D::BeginScene(camera);

		// Render Quads stuff
		auto group = m_Registry.group<TransformComponent>(entt::get<SpriteRendererComponent>);
		for (auto entity : group) {
			auto [transform, sprite] = group.get<TransformComponent, SpriteRendererComponent>(entity);

			if (!sprite.shader)
				Renderer2D::DrawSprite(transform.getTransform(), sprite, (int)entity);
			else
				Renderer2D::DrawQuad(transform.getTransform(), sprite.shader, sprite.color, (int)entity);
		}

		// Draw circles
		{
			auto view = m_Registry.view<TransformComponent, CircleRendererComponent>();
			for (auto entity : view)
			{
				auto [transform, circle] = view.get<TransformComponent, CircleRendererComponent>(entity);

				if (circle.texture)
					Renderer2D::DrawCircle(transform.getTransform(), circle.texture, circle.tilingFactor, circle.color, circle.thickness, circle.fade, (int)entity);
				else
					Renderer2D::DrawCircle(transform.getTransform(), circle.color, circle.thickness, circle.fade, (int)entity);
			}
		}

		Renderer2D::EndScene();
	}

	void Scene::onViewportResize(uint32_t width, uint32_t height) {
		m_ViewportWidth = width;
		m_ViewportHeight = height;

		// Resize cams
		// Loop through ortho cameras
		auto orthCams = m_Registry.view<CameraComponent>();
		for (auto entity : orthCams) {
			auto& camera = orthCams.get<CameraComponent>(entity);

			if (!camera.fixedAspectRatio) {
				camera.setViewportSize(width, height);
			}
		}
	}

	Entity Scene::getPrimaryCameraEntity() {
		auto cams = m_Registry.view<CameraComponent>();
		for (auto entity : cams) {
			const auto& camera = cams.get<CameraComponent>(entity);

			if (camera.primary)
				return { entity, this };			
		}
		return {};
	}

	Entity Scene::getEntityById(uint64_t __id) {
		auto ids = m_Registry.view<IDComponent>();
		for (auto entity : ids) {
			const auto& id = ids.get<IDComponent>(entity);

			if (id.id == __id)
				return { entity, this };
		}
		return {};
	}

	void Scene::softResetPhysics() {
		if (m_World) {
			delete m_World;
			m_World = nullptr;
		}

		// TODO make the physics adjustable
		s_physics2DCallback = new Physics2DCallback(this);
		m_World = new b2World({ 0.0f, -9.8f });
		m_World->SetContactListener(s_physics2DCallback);

		auto view = m_Registry.view<RigidBody2DComponent>();
		for (auto e : view) {
			Entity entity = { e, this };

			auto& id = entity.getComponent<IDComponent>();
			auto& transform = entity.getComponent<TransformComponent>();
			auto& rb2D = entity.getComponent<RigidBody2DComponent>();


			b2BodyDef bodyDef;
			bodyDef.type = (b2BodyType)rb2D.type;	// TODO : maybe change this
			bodyDef.fixedRotation = rb2D.fixedRotation;
			bodyDef.position.Set(transform.position.x, transform.position.y);
			bodyDef.angle = transform.rotation.z;

			b2BodyUserData useData;
			useData.pointer = id.id;
			bodyDef.userData = useData;

			b2Body* body = m_World->CreateBody(&bodyDef);
			rb2D.runtimeBody = body;
			

			if (entity.hasComponent<BoxCollider2DComponent>()) {
				auto& collider = entity.getComponent<BoxCollider2DComponent>();

				b2PolygonShape polygonShape;
				polygonShape.SetAsBox(transform.scale.x * collider.size.x, transform.scale.y * collider.size.y);

				b2FixtureDef fixtureDef;
				fixtureDef.restitution = collider.restitution;
				fixtureDef.restitutionThreshold = collider.restitutionThreshold;
				fixtureDef.friction = collider.friction;
				fixtureDef.density = collider.density;
				fixtureDef.shape = &polygonShape;

				b2Fixture* fixture = body->CreateFixture(&fixtureDef);
				collider.runtimeFixture = fixture;
			}

			if (entity.hasComponent<CircleCollider2DComponent>())
			{
				auto& cc2d = entity.getComponent<CircleCollider2DComponent>();

				b2CircleShape circleShape;
				circleShape.m_p.Set(cc2d.offset.x, cc2d.offset.y);
				circleShape.m_radius = transform.scale.x * cc2d.radius.x;

				b2FixtureDef fixtureDef;
				fixtureDef.shape = &circleShape;
				fixtureDef.density = cc2d.density;
				fixtureDef.friction = cc2d.friction;
				fixtureDef.restitution = cc2d.restitution;
				fixtureDef.restitutionThreshold = cc2d.restitutionThreshold;
				body->CreateFixture(&fixtureDef);
			}
		}
	}

	// Helpers
	template<typename Component>
	static void CopyComponent(entt::registry& dst, entt::registry& src, const std::unordered_map<UUID, entt::entity>& enttMap)
	{
		auto view = src.view<Component>();
		for (auto e : view)
		{
			UUID uuid = src.get<IDComponent>(e).id;
			SHADO_CORE_ASSERT(enttMap.find(uuid) != enttMap.end(), "");
			entt::entity dstEnttID = enttMap.at(uuid);

			auto& component = src.get<Component>(e);
			dst.emplace_or_replace<Component>(dstEnttID, component);
		}
	}

	template<typename Component>
	static void CopyComponentIfExists(Entity dst, Entity src)
	{
		if (src.hasComponent<Component>())
			dst.addOrReplaceComponent<Component>(src.getComponent<Component>());
	}


}