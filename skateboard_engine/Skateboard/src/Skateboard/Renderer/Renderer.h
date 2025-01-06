#pragma once

#include "sktbdpch.h"

namespace Skateboard
{
	class GraphicsContext;
	class Camera;

	class IRenderer
	{
		public:
		virtual void Init() = 0;
		virtual void Begin() = 0;
		virtual void End() = 0;
		virtual void Shutdown() = 0;
	};

	class RenderManager
	{
		public:
		template<class Renderer, typename ... Args>
		constexpr static Renderer* RegisterRenderer(Args&& ...args)
		{
			static_assert(std::is_convertible<Renderer*, IRenderer*>::value, "Your Renderer, doth not inherit from IRenderer");
			m_Renderers.push_back(std::make_unique<Renderer>(std::forward<Args>(args)...));
			return m_Renderers.back().get();
		}

	protected:
		static void Init()
		{
			for (auto& R : m_Renderers) R->Init();
		};

		static void StartDraw()
		{
			for (auto& R : m_Renderers) R->Begin();
		};

		static void EndDraw()
		{
			for (auto& R : m_Renderers) R->End();
		};
	

		static void ShutDown()
		{
			for (auto& R : m_Renderers) R->Shutdown();
		};
		
	private:
		static std::vector<std::unique_ptr<IRenderer>> m_Renderers;
	};
}