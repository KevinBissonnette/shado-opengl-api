﻿#include "Application.h"

#include "Debug.h"
#include "GL/glew.h"
#include "Renderer2D.h"
#include <algorithm>
#include "cameras/OrthoCamera.h"
#include "Events/ApplicationEvent.h"
#include "Events/KeyEvent.h"
#include "Events/MouseEvent.h"
#include "Renderer3D.h"
#include "util/Random.h"

namespace Shado {

	// =========================== APPLICATION CLASS ===========================

	Application* Application::singleton = new Application();

	Application::Application(unsigned width, unsigned height, const std::string& title)
		: window(new Window(width, height, title)), uiScene(new ImguiLayer)
	{
		Log::init();
		Random::init();

		/* Initialize the library */
		// TODO: Might want to un comment this if application crashes
		//window = std::make_unique<Window>(width, height, title);
	}

	Application::Application()
		: Application(1280, 720, "Shado OpenGL simple Rendering engine")
	{
	}

	Application::~Application() {

		for (Layer* layer : layers) {
			if (layer == nullptr)
				continue;
			layer->onDestroy();
			delete layer;
		}

		glfwTerminate();
	}

	void Application::run() {

		// Init Renderer if it hasn't been done
		if (!Renderer2D::hasInitialized()) {
			Renderer2D::Init();
			Renderer3D::Init();
			uiScene->onInit();
		}

		if (layers.size() == 0)
			SHADO_CORE_WARN("No layers to draw");


		/* Loop until the user closes the window */
		while (m_Running) {

			float time = (float)glfwGetTime();	// TODO: put it in platform specific
			float timestep = time - m_LastFrameTime;
			m_LastFrameTime = time;

			if (!m_minimized) {
				/* Render here */
				Renderer2D::Clear();

				// Draw scenes here
				for (Layer* layer : layers) {
					if (layer == nullptr) {
						continue;
					}

					layer->onUpdate(timestep);
					layer->onDraw();
				}
				uiScene->onUpdate(timestep);
				uiScene->onDraw();

				// Render UI
				uiScene->begin();
				for (Layer* layer : layers) {
					if (layer != nullptr)
						layer->onImGuiRender();
				}
				uiScene->onImGuiRender();
				uiScene->end();
			}

			/* Swap front and back buffers */
			/* Poll for and process events */
			window->onUpdate();
		}
	}

	void Application::submit(Layer* layer) {

		// Init Renderer if it hasn't been done
		if (!Renderer2D::hasInitialized()) {
			Renderer2D::Init();
			Renderer3D::Init();
			uiScene->onInit();
		}

		// Init all layers
		layer->onInit();
		layers.push_back(layer);

		// Reverse sorting
		/*std::sort(allScenes.begin(), allScenes.end(), [](Scene* a, Scene* b) {
			return a->getZIndex() < b->getZIndex();
			});*/
	}

	void Application::onEvent(Event& e) {
		EventDispatcher dispatcher(e);
		dispatcher.dispatch<WindowResizeEvent>([this](WindowResizeEvent& evt) {
			int width = evt.getWidth(), height = evt.getHeight();
			if (width == 0 || height == 0)
			{
				m_minimized = true;
				return false;
			}
			m_minimized = false;
			glViewport(0, 0, width, height);

			return false;
		});

		dispatcher.dispatch<WindowCloseEvent>([this](WindowCloseEvent& evt) {
			m_Running = false;
			return true;
		});

		// Distaptch the event and excute the required code
		// Pass Events to layer
		// for (Layer* layer : layers) {
		// 	if (layer != nullptr) {
		// 		layer->onEvent(e);
		// 		if (e.isHandled())
		// 			break;
		// 	}
		// }

		for (auto it = layers.end(); it != layers.begin(); )
		{
			Layer* layer = (*--it);
			if (layer != nullptr) {
				layer->onEvent(e);
				if (e.isHandled())
					break;
			}			
		}
	}
	
	void Application::destroy() {
		for (Layer*& layer : singleton->layers) {
			layer->onDestroy();
			delete layer;
			layer = nullptr;
		}
		singleton->uiScene->onDestroy();

		singleton->m_Running = false;

		delete singleton;
	}

	void Application::close() {
		singleton->m_Running = false;
	}

}
