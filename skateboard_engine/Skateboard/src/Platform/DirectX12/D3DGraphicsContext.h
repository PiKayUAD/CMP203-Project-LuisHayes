#pragma once

#include "sktbdpch.h"

#include "Skateboard/Renderer/GraphicsContext.h"
#include "Platform/DirectX12/Api/D3DRenderingApi.h"

#include "Skateboard/Platform.h"
#include "D3DTypes.h"
#include "Memory/D3DDescriptorHeap.h"
#include "Skateboard/Memory/VirtualAllocator.h"
#include "Platform/DirectX12/Memory/UploadManager.h"

#ifndef SKTBD_DESKTOP_PLATFORM_MIN_BUFFER_ALIGNMENT
#define SKTBD_DESKTOP_PLATFORM_MIN_BUFFER_ALIGNMENT (64*1024)
#endif // !SKTBD_DESKTOP_PLATFORM_MIN_BUFFER_ALIGNMENT

#define D3D12MA_D3D12_HEADERS_ALREADY_INCLUDED
#define D3D12MA_OPTIONS16_SUPPORTED 1
#include "D3D12MemAlloc.h"

using namespace Microsoft::WRL;

#define D3D_DEVICE_REMOVED_EXTENDED_DATA_ENABLE_FLAG 0b1

namespace Skateboard
{
	struct CommandBufferMemHandle
	{
		ComPtr<ID3D12CommandAllocator> m_Allocator;
		ComPtr<ID3D12GraphicsCommandList10> m_Commandlist;
		uint64_t m_CompletionFenceValue;
	};

	//easy access d3dContext
	extern D3DGraphicsContext* gD3DContext;

	class D3DGraphicsContext final : public GraphicsContext
	{
		friend class UploadManager;
		friend class D3DRenderingApi;

	public:
		//Flags
		uint32_t m_Flags = D3D_DEVICE_REMOVED_EXTENDED_DATA_ENABLE_FLAG;

	protected:
		RenderingApi* GetAPI() final override { return &D3DAPI; };

	public:
		// Let's not make things confusing and initialise everything in the constructor
		D3DGraphicsContext(HWND window, const PlatformProperties& props);
		// And release all in destructor
		virtual ~D3DGraphicsContext() final override;

		//---------------------------------------OVERRIDES

		// Public functions to resize the buffers according to the new dimensions stored in lParam based on the size description in wParam
		virtual void Resize(int clientWidth, int clientHeight) final override;
		virtual void OnResized() final override;

		virtual void SetRenderTargetToBackBuffer() final override;

		virtual void BeginFrame() final override;
		virtual void EndFrame() final override;

		virtual void Reset() final override;
		virtual void Flush() final override;
		virtual void WaitUntilIdle() final override;
		virtual void Present() final override;

		virtual bool IsRaytracingSupported() const final override { return m_HasDXR; }
		virtual bool AreWorkGraphsSupported() const final override { return false; };

		virtual CopyResult CopyDataToBuffer(Buffer* dest, off_t offset, size_t size, void* src)final override;
		virtual CopyResult CopyDataToBuffer(Buffer* dest, off_t offset, size_t size, std::function<void(void*)> WriterFunct) final override;

		virtual off_t GetViewHeapIndex(const ShaderView* View) const;

		//virtual std::future<CopyResult> CopyData(MemoryResource* dest, MemoryResource* src) final override;

		virtual std::function<void(void**)> GetBufferMemoryInitializer(const BufferDesc& desc)	 final override;
		virtual std::function<void(void**)> GetTextureMemoryInitializer(const TextureDesc& desc) final override;

#ifdef SKTBD_DEBUG
		virtual std::function<void(void**, const std::wstring& debugname)> RegisterMemoryResource(GPUResourceType_ Type) final override;
#endif // SKTBD_DEBUG

		//VIEWS 
		 
		//rendertargets
		virtual std::function<void(void**)> CreateDepthRenderTargetView(void* ResourceHandle, DepthStencilDesc* desc = nullptr) final override;
		virtual std::function<void(void**)> CreateRenderTargetView(void* ResourceHandle, RenderTargetDesc* desc = nullptr) final override;

		////textures
		virtual std::function<void(void**)> CreateTextureView(void* ResourceHandle, ViewAccessType_ ViewAccessType, TextureViewDesc* desc = nullptr) final override;

		////buffers
		virtual std::function<void(void**)> CreateBufferView(void* ResourceHandle, ViewAccessType_ ViewAccessType, const BufferViewDesc& desc) final override;

		//State objects are different to reources in that they dont need to persist as they are read only and are copied into the hardware
		virtual std::function<void(void**)> CreateCommandBuffer(const CommandBufferType_& ViewAccessType, const CommandBufferPriority_& desc) final override;

		virtual SKTBDR CreatePipelineInputLayout(const std::wstring& debugname, ShaderInputLayout& layout, const ShaderInputsLayoutDesc& Desc);
		virtual SKTBDR CreatePipelineState(const std::wstring& debugname, Pipeline& pipeline, const PipelineDesc& Desc) final override;

		//END OVERRIDES

		void NextBackBuffer() { m_CurrentBackBuffer = (m_CurrentBackBuffer + 1) % g_SwapChainBufferCount; }
		
		IDXGISwapChain* SwapChain() const { return m_SwapChain.Get(); }

		DXGI_FORMAT GetBackBufferFormat() const { return m_BackBufferFormat; }
		DXGI_FORMAT GetDepthStencilFormat() const { return m_DepthStencilFormat; }

		ID3D12DescriptorHeap* const GetSRVHeap() const { return m_SRVDescriptorHeap.GetHeap(); }

		ID3D12DescriptorHeap* const GetSamplerHeap() const { return m_SamplerHeap.GetHeap(); }

		D3DDescriptorHandle GetImGuiDescriptorHandle() const { return m_ImGuiHandle; }
		
	//	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> CreateCommandAllocator(ComPtr<ID3D12Device2> device, D3D12_COMMAND_LIST_TYPE type);
	//	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> GetAvailableCommandAllocator(D3D12_COMMAND_LIST_TYPE type);

		ID3D12GraphicsCommandList10* GetDefaultCommandList() const { return static_cast<CommandBufferMemHandle*>(m_DefaultGraphicsCB.GetResource())->m_Commandlist.Get(); }
		ID3D12CommandAllocator* GetDefaultCommandAllocator() const { return static_cast<CommandBufferMemHandle*>(m_DefaultGraphicsCB.GetResource())->m_Allocator.Get(); }

		void BeginCommandBuffer(CommandBuffer& cb);
		void EndCommandBuffer(CommandBuffer& cb);

		void SubmitCommandBuffers(CommandBuffer** cb, const uint32_t count);


		ID3D12Device14* Device() const { return m_Device.Get(); }
		auto Utils() const { return m_Utils.Get(); }
		//ID3D12Device5* RaytracingDevice() const { return m_DxrDevice.Get(); }
	//	ID3D12GraphicsCommandList10* GraphicsCommandList() const { return m_CommandList.Get(); }
		//ID3D12GraphicsCommandList4* RaytracingCommandList() const { return m_DxrCommandList.Get(); }
	//	ID3D12CommandAllocator* CommandAllocator() const { return a_CommandAllocators[m_CurrentFrameResourceIndex].Get(); }
		ID3D12CommandQueue* CommandQueue() const { return m_CommandQueue.Get(); }
		ID3D12CommandQueue* CopyQueue() const { return m_CopyCommandQueue.Get(); }

		D3DDescriptorHandle AllocateCPUSRV(uint32_t count) { return m_RWSRVDescriptorHeap.Allocate(count); }
		D3DDescriptorHandle AllocateGPUSRV(uint32_t count) { return m_SRVDescriptorHeap.Allocate(count); }
		D3DDescriptorHandle AllocateRTV(uint32_t count) { return m_RTVDescriptorHeap.Allocate(count); }
		D3DDescriptorHandle AllocateDSV(uint32_t count) { return m_DSVDescriptorHeap.Allocate(count); }
		D3DDescriptorHeap* GetCPUSRVDescriptorHeap() { return &m_RWSRVDescriptorHeap; }
		D3DDescriptorHeap* GetGPUSRVDescriptorHeap() { return &m_SRVDescriptorHeap; }
		D3DDescriptorHeap* GetRTVDescriptorHeap() { return &m_RTVDescriptorHeap; }
		D3DDescriptorHeap* GetDSVDescriptorHeap() { return &m_DSVDescriptorHeap; }


		//void ExecuteGraphicsCommandList(void* args = nullptr);

		_NODISCARD D3D12_RECT GetScissorsRect() { return m_ScissorRect; }
		_NODISCARD D3D12_RECT GetScissorsRect() const { return m_ScissorRect; }

		_NODISCARD D3D12_VIEWPORT GetViewport() { return m_Viewport; }
		_NODISCARD D3D12_VIEWPORT GetViewport() const { return m_Viewport; }

		_NODISCARD ID3D12Resource* GetCurrentBackBuffer() { return m_SwapChainBuffers[m_CurrentBackBuffer].Get(); }
		_NODISCARD const ID3D12Resource* GetCurrentBackBuffer() const { return m_SwapChainBuffers[m_CurrentBackBuffer].Get(); }

		uint64_t NextFence() { ++m_LatestFenceValue; GetFenceValue() = m_LatestFenceValue; return m_LatestFenceValue; }
		_NODISCARD ID3D12Fence* GetFence() { return m_Fence.Get(); }
		_NODISCARD const ID3D12Fence* GetFence() const { return m_Fence.Get(); }
		_NODISCARD uint64_t& GetFenceValue() { return a_FenceValues[m_CurrentFrameResourceIndex]; }
		
		// We need to be able to access the descriptors stored in the respective heaps of our buffer
		D3D12_CPU_DESCRIPTOR_HANDLE CurrentBackBufferView() const;
		D3D12_CPU_DESCRIPTOR_HANDLE DepthStencilView() const;



		void SetDeferredReleasesFlag();

		void DeferredRelease(IUnknown* resource);

		void ProcessDeferrals();

		bool CheckDeviceRemovedStatus() const;

		D3D12MA::Allocator* GetMemoryAllocator() { return p_MemoryAllocator; }

	private:
		void CreateDevice();
		void CreateFence();
		void CreateDescriptorSizes();
		void Check4xMSAAQualitySupport();
		void CreateCommandQueueAndCommandList();
		void CreateSwapChain();

		void CreateRenderTargetViews();
		void CreateDepthStencilBuffer();
		void SetViewPort();
		void SetScissorRectangles();

		void CreateDescriptorHeaps();

		void CreateUploadManager();

	private:
		// Derived class should set these in derived constructor to customize starting values.
		D3D_DRIVER_TYPE	m_D3DDriverType;					// Driver type options
		DXGI_FORMAT		m_BackBufferFormat;					// Format of the back buffer
		DXGI_FORMAT		m_DepthStencilFormat;				// Format of the depth/stencil buffer

		// Window settings, mostly in regards with resizing
		bool			m_Vsync;								// Note: Fullscreen in this framework is handled by the platform API

		// MSAA support (not used in this project, but could be enabled)
		bool m_MSAAEnable;
		UINT m_MSAAQuality;

		// Working environment
		HWND m_MainWindow;

		//DXC Utils for shader reflection logic
		Microsoft::WRL::ComPtr<IDxcUtils> m_Utils;

		//upload manager for uploading buffers
		UploadManager m_UploadManager;

		// COM Objects
		// Device and DXGI Factory
		Microsoft::WRL::ComPtr<ID3D12Device14>				m_Device;						// The device is the display adapter, like the graphics card

		DWORD												m_MessageCallbackCookie;
		ComPtr<ID3D12InfoQueue1>							m_InfoQueue;

		Microsoft::WRL::ComPtr<IDXGIFactory7>				m_DXGIInterface;				// The interface to generate any DXGI objects (version 4 provides more functionalities, such as EnumWarpAdapters)

		//Memory Allocator // INITIALISED IN THE CREATE DEVICE
		D3D12MA::Allocator*									p_MemoryAllocator;

		// Fence
		Microsoft::WRL::ComPtr<ID3D12Fence>					m_Fence;						// The fence object to synchronise the CPU/GPU
		uint64_t											m_LatestFenceValue;
		std::array<uint64_t, GRAPHICS_SETTINGS_NUMFRAMERESOURCES> a_FenceValues;			// Identify a fence point in time. Everytime we mark a new fence point, increment this integer

		// Command Objects
		Microsoft::WRL::ComPtr<ID3D12CommandQueue>			m_CommandQueue;					// The command queue we will use for this application to submit commands to the GPU
		Microsoft::WRL::ComPtr<ID3D12CommandQueue>			m_CopyCommandQueue;					// The command queue we will use for this application to submit commands to the GPU

		//Microsoft::WRL::ComPtr<ID3D12CommandAllocator>		m_DirectCommandAllocator;
//		std::array<Microsoft::WRL::ComPtr<ID3D12CommandAllocator>, GRAPHICS_SETTINGS_NUMFRAMERESOURCES> a_CommandAllocators;
//		Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList10>	m_CommandList;

		// array of Command Allocators for device queues
//		std::array<std::stack<ComPtr<ID3D12CommandAllocator>>, 7 > m_AvailableCommandAllocators;
//		std::array<std::stack<ComPtr<ID3D12CommandAllocator>>, 7 > m_InUseCommandAllocators;

		// SwapChain
		Microsoft::WRL::ComPtr<IDXGISwapChain4>				m_SwapChain;

		// Buffers
		static const int									g_SwapChainBufferCount = GRAPHICS_SETTINGS_NUMFRAMERESOURCES;
		int													m_CurrentBackBuffer;

		Microsoft::WRL::ComPtr<ID3D12Resource>				m_SwapChainBuffers[g_SwapChainBufferCount];	// The back buffers to use in the swapchain

		// Descriptor sizes
		UINT m_RTVDescriptorSize;
		UINT m_DSVDescriptorSize;
		UINT m_CBVSRVUAVDescriptorSize;

		// Viewports and scissor rectangles
		D3D12_VIEWPORT	m_Viewport;							// The viewport to which the 3D world will be rendered onto
		D3D12_RECT		m_ScissorRect;						// Pixels outside of this rectangle are culled (not rasterized onto the back buffer)

		// Bools
		bool	m_HasDXR;			// A bool that will be checked on init to initialise raytracing components
		bool	m_HasWorkGraphs;
		bool	m_ClientResized;

		std::vector<IUnknown*> m_DeferredReleases[GRAPHICS_SETTINGS_NUMFRAMERESOURCES]{};
		UINT32 m_DeferredFlags[GRAPHICS_SETTINGS_NUMFRAMERESOURCES];
		std::mutex m_DeferredMutex;

		D3DDescriptorHeap m_RWSRVDescriptorHeap;		// Read-Write
		D3DDescriptorHeap m_SRVDescriptorHeap;			// Write only as shader visible

		D3DDescriptorHeap m_RTVDescriptorHeap;
		D3DDescriptorHeap m_DSVDescriptorHeap;

		D3DDescriptorHeap m_SamplerHeap;

		//Imgui
		D3DDescriptorHandle m_ImGuiHandle;

		//RenderAPI
		D3DRenderingApi D3DAPI;
	};
}
