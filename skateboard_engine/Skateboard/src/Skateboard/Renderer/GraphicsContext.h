#pragma once
#include "Skateboard/Mathematics.h"
#include "Skateboard/Renderer/CommonResources.h"
#include "Skateboard/Renderer/GraphicsSettingsDefines.h"
#include "sktbdpch.h"

namespace Skateboard
{
	using CopyResult = void*;
	class Pipeline;
	class RenderingApi;

	class CommandBuffer : public GPUResource
	{
		CommandBuffer(CommandBuffer&& other) = default;
		CommandBufferType_ GetType() const { return m_Type; }
		CommandBufferPriority_ GetPriority() const { return m_Priority; }

	public:
		CommandBuffer(const std::wstring& debugName, CommandBufferType_ type, CommandBufferPriority_ prior) : m_Type(type), m_Priority(prior), GPUResource(debugName, GpuResourceType_CommandBuffer)
		{
		};

	protected:
		CommandBufferType_ m_Type;
		CommandBufferPriority_ m_Priority;
	};

	class GraphicsContext
	{
		friend class RenderCommand;
		friend class ResourceFacotry;

	public:
		GraphicsContext(int32_t clientWidth, int32_t clientHeight) :
			m_ClientWidth(clientWidth),
			m_ClientHeight(clientHeight),
			m_CurrentFrameResourceIndex(0u),
			m_DefaultGraphicsCB(MultiResource<CommandBuffer>(L"Default CommandBuffer", CommandBufferType_Graphics, CommandBufferPriority_Primary))
		{
		}
		virtual ~GraphicsContext() {}

		virtual void SetRenderTargetToBackBuffer() = 0;

		virtual void Resize(int clientWidth, int clientHeight) {}
		virtual void OnResized() {}
		virtual float GetClientAspectRatio() const { return static_cast<float>(m_ClientWidth) / m_ClientHeight; }

		virtual void Reset() {}
		virtual void Flush() {}
		virtual void WaitUntilIdle() {}

		virtual void BeginFrame() {}
		virtual void EndFrame() {}
		virtual void Present() {}

		virtual bool IsRaytracingSupported() const = 0;
		virtual bool AreWorkGraphsSupported() const = 0;

		virtual off_t GetViewHeapIndex(const ShaderView* View) const = 0;

		//Data Shuffling
		virtual CopyResult CopyDataToBuffer(Buffer* dest, off_t offset, size_t size, void* src) = 0 ;
		virtual CopyResult CopyDataToBuffer(Buffer* dest, off_t offset, size_t size, std::function<void(void*)> WriterFunct) = 0; 


		virtual SKTBDR CreatePipelineInputLayout(const std::wstring& debugname, ShaderInputLayout& layout, const ShaderInputsLayoutDesc& Desc) = 0;
		virtual SKTBDR CreatePipelineState(const std::wstring& debugname,  Pipeline& pipeline, const PipelineDesc& Desc) = 0;

		//Access to Variables

		const int32_t GetClientWidth() const { return m_ClientWidth; }
		const int32_t GetClientHeight() const  { return m_ClientHeight; }
		void NextFrame() { m_CurrentFrameResourceIndex = (m_CurrentFrameResourceIndex + 1) % GRAPHICS_SETTINGS_NUMFRAMERESOURCES; m_DefaultGraphicsCB.IncrementCounter(); }
		uint64_t GetCurrentFrameResourceIndex() const { return m_CurrentFrameResourceIndex; }
		CommandBuffer& GetDefaultCommandBuffer() { return m_DefaultGraphicsCB; }

		virtual RenderTargetView* GetBackBuffer() { return &m_SwapChainRTVs; };
		virtual DepthStencilView* GetDefaultDepthBuffer() { return &m_DefaultDSV; };

		void SetBackBufferClearColour(float4 nClearColour) { m_ClearColour = nClearColour; }
		void SetClearBackBuffer(bool ClearBackBuffer) { m_bClearBackBuffer = ClearBackBuffer; }

		//resources
		virtual std::function<void(void**)> GetBufferMemoryInitializer(const BufferDesc& desc) = 0;
		virtual std::function<void(void**)> GetTextureMemoryInitializer(const TextureDesc& desc) = 0;

#ifdef SKTBD_DEBUG
		virtual std::function<void(void**, const std::wstring& debugname)> RegisterMemoryResource(GPUResourceType_ Type) = 0;
#endif // SKTBD_DEBUG

		//rendertargets
		virtual std::function<void(void**)> CreateDepthRenderTargetView(void* Resource, DepthStencilDesc* desc = nullptr) = 0;
		virtual std::function<void(void**)> CreateRenderTargetView(void* Resource, RenderTargetDesc* desc = nullptr) = 0;

		////textures
		virtual std::function<void(void**)> CreateTextureView(void* Resource, ViewAccessType_ ViewAccessType, TextureViewDesc* desc = nullptr) = 0;

		////buffers
		virtual std::function<void(void**)> CreateBufferView(void* Resource, ViewAccessType_ ViewAccessType, const BufferViewDesc& desc) = 0;

		//Command Structures and State Management
		virtual std::function<void(void**)> CreateCommandBuffer(const CommandBufferType_& type, const CommandBufferPriority_& priority) = 0;

	protected:
		virtual RenderingApi* GetAPI() = 0;

	public:
		static GraphicsContext* Context;
	protected:

		int32_t m_ClientWidth, m_ClientHeight;		// Width and height of the client area (does not include the top bar and menus, this is the drawable surface)
		uint64_t m_CurrentFrameResourceIndex;

		float4 m_ClearColour = GRAPHICS_BACKBUFFER_DEFAULT_CLEAR_COLOUR;
		bool m_bClearBackBuffer = true;

		MultiResource<CommandBuffer> m_DefaultGraphicsCB;

		//Default Render Targets
		MultiResource<RenderTargetView, GRAPHICS_SETTINGS_NUMFRAMERESOURCES>	m_SwapChainRTVs;							// only views can be accessed

		//Default DSV
		SingleResource<TextureBuffer>											m_DepthStencilBuffer;						// The depth/stencil buffer
		SingleResource<DepthStencilView>										m_DefaultDSV;
	};
}