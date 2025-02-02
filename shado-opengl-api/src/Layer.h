﻿#pragma once
#include <string>

#include "box2d/b2_world.h"
#include "Events/Event.h"
#include "util/Util.h"
#include "Entity.h"

namespace Shado {
	class Scene;
	
	/*class Layer {
	public:
		Layer(const std::string& name = "Layer");
		virtual ~Layer() = default;

		virtual void onInit()				{}
		virtual void onUpdate(TimeStep dt)	{}
		virtual void onDraw()				{}
		virtual void onImGuiRender()		{}
		virtual void onDestroy()			{}
		virtual void onEvent(Event& event)	{}

		std::string			getName()	const { return m_Name; }
		unsigned long long	getId()		const { return m_Id; }

		inline Scene* getScene()			{ return m_Scene; }

	private:
		std::string m_Name;
		unsigned long long m_Id;

		Scene* m_Scene;

		friend class Scene;
	};*/

	class Scene {
	public:
		Scene(const std::string& name);
		virtual ~Scene();

		/**
		 * Called when the scene is set as active
		 */
		virtual void onMount()		{}
		
		/**
		 * Called when the scene is unmounted (is inactive)
		 */
		virtual void onUnMount()	{}

		virtual void updatePhysics(TimeStep dt) final;

		/**
		 * This function is called on all scenes when the application runs
		 */
		virtual void onInit()				{}

		/**
		 * This function is called every frame
		 *
		 * @param dt time between 2 frames
		 */
		virtual void onUpdate(TimeStep dt)	{}

		/**
		 * This function is called every frame and draw everything to the screen
		 */
		virtual void onDraw()				{}

		/**
		 * This function draws all ImGui code to the screen
		 */
		virtual void onImGuiRender()		{}

		/**
		 * This function is called when the scene is delete
		 */
		virtual void onDestroy()			{}

		/**
		 * This functions is called whenever an event occures
		 */
		virtual void onEvent(Event& event)	{}

		//void pushLayer(Layer* layer);

		// Physics related functions
		Entity* addEntityToWorld(Entity* entity);
		Entity* addEntityToWorld(const EntityDefinition& def);
		void setWorldGravity(const glm::vec2& gravity);

		void destroyEntity(Entity* entity);

		Entity& getEntity(const std::string& name);
		Entity& getEntity(uint64_t id);
		
		// const std::vector<Layer*>& getLayers()	const;
		const std::string& getName()			const { return name; }
		b2World& getWorld()								{ return world; }
		const std::vector<Entity*>& getAllEntities()	const { return entities; }	// TODO: remove
		
	protected:
		// std::vector<Layer*> m_Layers;
		std::string name;


		std::vector<Entity*> entities;
		b2World world;
	};
}
