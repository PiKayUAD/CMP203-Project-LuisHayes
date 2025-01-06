#pragma once
#include "AGCF.h"
#include "Skateboard/Renderer/GraphicsContext.h"
#include "Skateboard/Platform.h"

#include "Memory/AGCMemoryAllocator.h"

namespace Skateboard
{
	class AGCGraphicsContext final : public GraphicsContext
	{
	public:
		AGCGraphicsContext(const PlatformProperties& props);
		virtual ~AGCGraphicsContext() final override;

		virtual void SetRenderTargetToBackBuffer() final override {}

		virtual void Resize(int clientWidth, int clientHeight) final override {}
		virtual void OnResized() final override {}

		virtual void Reset() final override {}
		virtual void Flush() final override {}
		virtual void WaitUntilIdle() final override {}

		virtual bool IsRaytracingSupported() const final override { return false; }

		virtual bool AreWorkGraphsSupported() const final override { return false; };
		virtual std::future<CopyResult> CopyData(GPUResource* dest, off_t offset, size_t size, void* src) final override { return std::future<CopyResult>(); };

		// Const getters
		int32_t GetVideoHandle() const { return m_VideoHandle; }
		const SceVideoOutLatencyControl& GetLatencyControl() const { return m_LatencyControl; }
		const sce::Agc::CxRenderTargetMask& GetRenderTargetMask() const { return m_RenderTargetMask; }
		const sce::Agc::CxDepthStencilControl& getDepthStencilControl() const { return m_DepthStencilControl; }
		const sce::Agc::CxBlendControl& getBlendControl() const { return m_BlendAlphaControl; }
		const sce::Agc::CxViewport& GetViewport() const { return m_Viewport; }

		// Non-const getters
		sce::Agc::DrawCommandBuffer& GetDrawCommandBuffer() { return m_DrawCommandBuffers[m_CurrentFrameResourceIndex]; }
		sce::Agc::Core::StateBuffer& GetStateBuffer() { return m_StateBuffers[m_CurrentFrameResourceIndex]; }
		sce::Agc::CxRenderTarget& GetRenderTarget() { return m_RenderTargets[m_CurrentFrameResourceIndex]; }
		sce::Agc::CxDepthRenderTarget& GetDepthStencilTarget() { return m_DepthStencilTarget; }
		sce::Agc::Label& GetFlipLabel() { return m_FlipLabels[m_CurrentFrameResourceIndex]; }

		TemplatedGPUMemoryPoolAllocator* GetMemAllocator() { return &m_MemoryAllocator; }
		//AGCMemoryPoolAllocator* GetMemoryPoolManager() { return &m_MemoryPool_Resources; }

	private:
		void InitialiseAGC();
		void CreateDrawCommandAndStateBuffers();
		void CreateFilpLabels();
		void CreateRenderTargets();
		void CreateDepthTarget();
		void InitialiseVideoOut();
		void CreateViewPort();

		virtual std::function<void(void**)> GetBufferMemoryInitializer(const BufferDesc& desc) final override;
		virtual std::function<void(void**)> GetTextureMemoryInitializer(const TextureDesc& desc) final override;

		virtual std::function<void(void**)> GetMemoryDeInitializer(GPUResourceType_ ResourceType) final override;

	private:
		// Video Out
		int32_t m_VideoHandle;
		const int32_t m_VideoSetIndex;
		SceVideoOutLatencyControl m_LatencyControl;

		// Command lists and state
		sce::Agc::DrawCommandBuffer m_DrawCommandBuffers[GRAPHICS_SETTINGS_NUMFRAMERESOURCES];
		sce::Agc::Core::StateBuffer m_StateBuffers[GRAPHICS_SETTINGS_NUMFRAMERESOURCES];

		// Render targets
		sce::Agc::CxRenderTarget m_RenderTargets[GRAPHICS_SETTINGS_NUMFRAMERESOURCES];	// Because the RT are flipped on the DCB we need one per frame resource
		sce::Agc::CxDepthRenderTarget m_DepthStencilTarget;
		sce::Agc::CxRenderTargetMask m_RenderTargetMask;
		sce::Agc::CxDepthStencilControl m_DepthStencilControl;
		sce::Agc::CxBlendControl m_BlendAlphaControl;


		// Flib labels for synchronisation (kinda like the swap chain i suppose)
		sce::Agc::Label* m_FlipLabels;

		// Viewport
		sce::Agc::CxViewport m_Viewport;

		// Memory allocator
		//AGCMemoryAllocator m_MemoryAllocator
		//{
		//	"ShaderHeap", 
		//	AGCMemAllocType_::ACGMemAllocType_GPU_ReadWrite_CPU_ReadWrite, 
		//	4096u
		//};

		// Memory pool management (EXPERIMENTAL)
		//AGCMemoryPoolAllocator m_MemoryPool_Resources = AGCMemoryPoolAllocator("Mem pool - resource block A", 2u * 1024 * 1024u * 1024u, /*alignment -: 0 = 2MiB default*/0u,
		//	0, (void*)SCE_KERNEL_APP_MAP_AREA_START_ADDR);


		// Direct memory allocations
		//AGCMemoryHandle m_ResourceRegistrationMemory;
		//AGCMemoryHandle m_DrawCommandBuffersMemory[GRAPHICS_SETTINGS_NUMFRAMERESOURCES];
		//AGCMemoryHandle m_RenderTargetsMemory[GRAPHICS_SETTINGS_NUMFRAMERESOURCES];
		//AGCMemoryHandle m_DepthStencilTargetMemory;
		//AGCMemoryHandle m_DepthStencilHTileMemory;
		//AGCMemoryHandle m_FlipLabelsMemory;

		TemplatedGPUMemoryPoolAllocator m_MemoryAllocator;

		MemoryHandle<uint8_t, sce::Agc::Alignment::kResourceRegistration> m_ResourceRegistrationMemory;

		typedef uint32_t CommandBufferMem; //struct to separate the command buffers into their own individual pool

		MemoryHandle<CommandBufferMem,sce::Agc::Alignment::kCommandBuffer> m_DrawCommandBuffersMemory[GRAPHICS_SETTINGS_NUMFRAMERESOURCES];
		MemoryHandle<uint8_t,sce::Agc::Alignment::kMaxTiledAlignment> m_RenderTargetsMemory[GRAPHICS_SETTINGS_NUMFRAMERESOURCES];
		MemoryHandle<uint8_t,sce::Agc::Alignment::kMaxTiledAlignment> m_DepthStencilTargetMemory;
		MemoryHandle<uint8_t,sce::Agc::Alignment::kMaxTiledAlignment> m_DepthStencilHTileMemory;
		MemoryHandle<sce::Agc::Label,sce::Agc::Alignment::kLabel> m_FlipLabelsMemory;
	};
}