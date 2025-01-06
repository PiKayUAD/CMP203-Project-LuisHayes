#include "sktbdpch.h"
#include "D3DGraphicsContext.h"
#include "Skateboard/Platform.h"
#include "Skateboard/Renderer/Buffer.h"
#include "Skateboard/Renderer/View.h"
#include "Skateboard/Renderer/Pipeline.h"
#include "Skateboard/Renderer/ResourceFactory.h"

#include "Memory/UploadManager.h"
#include "D3DDebugTools.h"

#define UPLOAD_MANAGER_SIZE 2*1024*1024

namespace Skateboard
{
	namespace GraphicsConstants
	{
		 size_t DEFAULT_RESOURCE_ALIGNMENT = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
		 size_t SMALL_RESOURCE_ALIGNMENT = D3D12_SMALL_RESOURCE_PLACEMENT_ALIGNMENT;
		 size_t MSAA_RESOURCE_ALIGNEMNT = D3D12_DEFAULT_MSAA_RESOURCE_PLACEMENT_ALIGNMENT;
		 size_t SMALL_MSAA_RESOURCE_ALIGNMENT = D3D12_SMALL_MSAA_RESOURCE_PLACEMENT_ALIGNMENT;
		 size_t CONSTANT_BUFFER_ALIGNMENT = D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT;
		 size_t BUFFER_ALIGNMENT = 0;
		 size_t RAYTRACING_STRUCT_ALIGNMENT = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT;
	}




	D3DGraphicsContext* gD3DContext = nullptr;
	GraphicsContext* GraphicsContext::Context = nullptr;

	D3DGraphicsContext::D3DGraphicsContext(HWND window, const PlatformProperties& props) :
		GraphicsContext(props.BackBufferWidth, props.BackBufferHeight),
		m_D3DDriverType(D3D_DRIVER_TYPE_HARDWARE),
		m_BackBufferFormat(DXGI_FORMAT_R8G8B8A8_UNORM),			// Each element has four 8-bit unsigned components mapped to the [0,1] range
		m_DepthStencilFormat(DXGI_FORMAT_D24_UNORM_S8_UINT),	// Specifies an unsigned 24-bit depth buffer mapped to the [0,1] range, with an 8-bit unsigned integer reserved for the stencil buffer ([0, 255] range) 
		m_Vsync(false),
		m_MSAAEnable(false),									// Set to false by default, change it to true in the constructor to enable MSAA
		m_MSAAQuality(0),										// Needs to be queried in Check4xMSAAQualitySupport
		m_MainWindow(window),
		m_LatestFenceValue(0u),
		a_FenceValues{0u},
		m_CurrentBackBuffer(0),									// Start by working on the first buffer
		m_RTVDescriptorSize(0u),
		m_DSVDescriptorSize(0u),
		m_CBVSRVUAVDescriptorSize(0u),
		m_HasDXR(false),										// This will be checked on initialisation, if the GPU supports DXR
		m_ClientResized(true)
	{
		// Assign the global context
		SKTBD_CORE_TRACE("Initialising the Graphics Context..");
		SKTBD_ASSERT_M(!gD3DContext, "Cannot initialise a second D3D graphics context. Illegal context creation.");
		gD3DContext = this;
		Context = this;

		// Initialise D3D components
		CreateDevice();
		CreateFence();
		CreateDescriptorSizes();
		Check4xMSAAQualitySupport();
		CreateCommandQueueAndCommandList();
		CreateSwapChain();
		
		// Allocate basic descriptor heaps
		CreateDescriptorHeaps();

		// Create the output buffers
		OnResized();

		//create upload manager
		CreateUploadManager();
	}

	D3DGraphicsContext::~D3DGraphicsContext()
	{
		WaitUntilIdle();

		//stop recieving messages from validation layers if there is an infoqueue
		if(m_InfoQueue.Get())
		m_InfoQueue->UnregisterMessageCallback(m_MessageCallbackCookie);

		SKTBD_CORE_TRACE("Destroying Device..");
	}

	void D3DGraphicsContext::SetRenderTargetToBackBuffer()
	{
		GetDefaultCommandList()->RSSetViewports(1, &m_Viewport);
		GetDefaultCommandList()->RSSetScissorRects(1, &m_ScissorRect);

		D3D12_CPU_DESCRIPTOR_HANDLE currentBackBufferHandle = CurrentBackBufferView();
		D3D12_CPU_DESCRIPTOR_HANDLE currentDepthStencilHandle = DepthStencilView();
		GetDefaultCommandList()->OMSetRenderTargets(1, &currentBackBufferHandle, true, &currentDepthStencilHandle);
	}

	void D3DGraphicsContext::BeginFrame()
	{
		// Get required API objects
		ID3D12GraphicsCommandList10* commandList = GetDefaultCommandList();
		const uint64_t currentFence = GetFenceValue();
		ID3D12CommandAllocator* currentAllocator = GetDefaultCommandAllocator();
		const D3D12_VIEWPORT viewport = GetViewport();
		const D3D12_RECT scissorsRect = GetScissorsRect();
		ID3D12Resource* backBuffer = GetCurrentBackBuffer();
		const D3D12_CPU_DESCRIPTOR_HANDLE currentBackBufferHandle = CurrentBackBufferView();
		const D3D12_CPU_DESCRIPTOR_HANDLE currentDepthStencilHandle = DepthStencilView();
		ID3D12DescriptorHeap* const pSrvHeap = GetSRVHeap();

		ID3D12DescriptorHeap* const pSamplerHeap = GetSamplerHeap();

		SKTBD_ASSERT_M(commandList, "Null command list!");

		// Check if the GPU is done with the next frame resource before proceeding
		// If not wait to avoid buffer overwrites
		if (currentFence && GetFence()->GetCompletedValue() < currentFence)
		{
#ifdef SKTBD_LOG_CPU_IDLE
			SKTBD_CORE_WARN("CPU idle, waiting on GPU to finish tasks!");
#endif
			HANDLE eventHandle = CreateEventEx(nullptr, NULL, NULL, EVENT_ALL_ACCESS);
			SKTBD_ASSERT_M(eventHandle, "Fence event creation failed!");

			// Fire the previously created event when GPU hits current fence
			D3D_CHECK_FAILURE(GetFence()->SetEventOnCompletion(currentFence, eventHandle));

			// Wait until the GPU hits the current fence and the event is received
			WaitForSingleObject(eventHandle, INFINITE);

			// Close the event so that it does not exist anymore
			CloseHandle(eventHandle);
		}

		// Process deferred resources here?
		ProcessDeferrals();
		// 

		// Reset the command list allocator to reuse its memory
		// Note: this can only be reset once the GPU has finished processing all the commands
		D3D_CHECK_FAILURE(currentAllocator->Reset());

		// Also reset the command list
		// Note: It can be reset anytime after calling ExecuteCommandList on the command queue
		// Note: The second argument will be used when drawing geometry
		D3D_CHECK_FAILURE(commandList->Reset(currentAllocator, nullptr));

		D3D12_TEXTURE_BARRIER TexBarriers[] =
		{
			CD3DX12_TEXTURE_BARRIER(
				D3D12_BARRIER_SYNC_ALL,									// SyncBefore
				D3D12_BARRIER_SYNC_RENDER_TARGET,						// SyncAfter
				D3D12_BARRIER_ACCESS_COMMON,							// AccessBefore
				D3D12_BARRIER_ACCESS_RENDER_TARGET,                     // AccessAfter
				D3D12_BARRIER_LAYOUT_PRESENT,							// LayoutBefore
				D3D12_BARRIER_LAYOUT_RENDER_TARGET,                     // LayoutAfter
				backBuffer,
				CD3DX12_BARRIER_SUBRESOURCE_RANGE(0xffffffff),			// All subresources
				D3D12_TEXTURE_BARRIER_FLAG_NONE
			)
		};

		D3D12_BARRIER_GROUP TexBarrierGroup[] =
		{
			CD3DX12_BARRIER_GROUP(1,TexBarriers)
		};

		commandList->Barrier(1, TexBarrierGroup);

		// Set the viewport and scissor rects. They need to be reset everytime the command list is reset
		commandList->RSSetViewports(1, &viewport);
		commandList->RSSetScissorRects(1, &scissorsRect);

		if (m_bClearBackBuffer)
		{
			// Clear the back buffer and depth buffer
			commandList->ClearRenderTargetView(
				currentBackBufferHandle,								// RTV to the resource we want to clear
				&m_ClearColour.x,											// The colour to clear the render target to
				0,														// The number of items in the pRects array (next parameter)
				nullptr													// An array of D3D12_RECTs that identify rectangle regions on the render target to clear. When nullptr, the entire render target is cleared
			);
		}

		commandList->ClearDepthStencilView(
			currentDepthStencilHandle,								// DSV to the resource we want to clear
			D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL,		// Flags indicating which part of the depth/stencil buffer to clear (here both)
			GRAPHICS_DEPTH_DEFAULT_CLEAR_COLOUR,					// Defines the value to clear the depth buffer
			GRAPHICS_STENCIL_DEFAULT_CLEAR_COLOUR,					// Defines the value to clear the stencil buffer
			0,														// The number of items in the pRects array (next parameter)
			nullptr													// An array of D3D12_RECTs that identify rectangle regions on the render target to clear. When nullptr, the entire render target is cleared
		);

		// Set the render target and depth/stencil target to the output merger
		commandList->OMSetRenderTargets(
			1,														// Defines the number of RTVs we are going to bind (next param)
			&currentBackBufferHandle,								// Pointer to an array of RTVs we want to bind to the pipeline
			true,													// Specify true if all the RTVs in the previous array are contiguous in the descriptor heap
			&currentDepthStencilHandle								// Pointer to a DSV we want to bind to the pipeline
		);

		// Set the main GPU descriptor heap, that is a region of memory where we have stored the descriptors for all the resources we want to use in this frame

		ID3D12DescriptorHeap* heaps[] = { pSrvHeap, pSamplerHeap };

		commandList->SetDescriptorHeaps(2, heaps);
	}

	void D3DGraphicsContext::EndFrame()
	{
		// Get required API objects
		ID3D12GraphicsCommandList10* commandList = GetDefaultCommandList();
		ID3D12CommandQueue* commandQueue = CommandQueue();
		ID3D12Resource* backBuffer = GetCurrentBackBuffer();
		ID3D12Fence* fence = GetFence();

		// Transition the back buffer from a PRESENT state to a RT state so that it can be written onto
		//D3D12_RESOURCE_BARRIER barrier = D3D::TransitionBarrier(backBuffer, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);

		D3D12_TEXTURE_BARRIER TexBarriers[] =
		{
			CD3DX12_TEXTURE_BARRIER(
				D3D12_BARRIER_SYNC_RENDER_TARGET,					 // SyncBefore
				D3D12_BARRIER_SYNC_ALL,								 // SyncAfter
				D3D12_BARRIER_ACCESS_RENDER_TARGET,					 // AccessBefore
				D3D12_BARRIER_ACCESS_COMMON,                         // AccessAfter
				D3D12_BARRIER_LAYOUT_RENDER_TARGET,                  // LayoutBefore
				D3D12_BARRIER_LAYOUT_PRESENT,                        // LayoutAfter
				backBuffer,
				CD3DX12_BARRIER_SUBRESOURCE_RANGE(0xffffffff),       // All subresources
				D3D12_TEXTURE_BARRIER_FLAG_NONE
			)
		};

		D3D12_BARRIER_GROUP TexBarrierGroup[] =
		{
			CD3DX12_BARRIER_GROUP(1,TexBarriers)
		};

		commandList->Barrier(1, TexBarrierGroup);

		// Close the command list for recording.
		D3D_CHECK_FAILURE(commandList->Close());

		//Add the command list to the command queue for execution
		ID3D12CommandList* commandLists[] = { commandList };
		commandQueue->ExecuteCommandLists(_countof(commandLists), commandLists);

		// Rendering processed, mark the frame fence
		commandQueue->Signal(fence, NextFence());
	}

	void D3DGraphicsContext::CreateDevice()
	{
		SKTBD_CORE_TRACE("Creating d3d12 device..");

		// First, enable the debug layer of D3D12 when running the application in debug mode
		// This will output debug messages when warnings, errors or crashes occur during rutime in the output window
		UINT DXGIFlags = 0;

#ifndef SKTBD_SHIP
		Microsoft::WRL::ComPtr<ID3D12Debug> debugController;
		DXGIFlags |= DXGI_CREATE_FACTORY_DEBUG;
		HRESULT res = D3D12GetDebugInterface(IID_PPV_ARGS(debugController.ReleaseAndGetAddressOf()));

		if (SUCCEEDED(res))
		{
			SKTBD_CORE_INFO("Debug Layers Enabled");
			debugController->EnableDebugLayer();
		}
		else
		{
			SKTBD_CORE_INFO("Debug Layers Disabled");
		}

		// Enable DRED
		ComPtr<ID3D12DeviceRemovedExtendedDataSettings1> dredSettings;
		if (m_Flags & D3D_DEVICE_REMOVED_EXTENDED_DATA_ENABLE_FLAG && SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&dredSettings))))
		{
			// Turn on AutoBreadcrumbs and Page Fault reporting
			dredSettings->SetAutoBreadcrumbsEnablement(D3D12_DRED_ENABLEMENT_FORCED_ON);
			dredSettings->SetPageFaultEnablement(D3D12_DRED_ENABLEMENT_FORCED_ON);

			SKTBD_CORE_INFO("DRED Enabled");
		}
#endif

		// Create the DXGI interface 1.1
		D3D_CHECK_FAILURE(CreateDXGIFactory2(DXGIFlags,IID_PPV_ARGS(m_DXGIInterface.ReleaseAndGetAddressOf())));

		// Create DXC Utils 

		DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(m_Utils.GetAddressOf()));

		// Create a list of every GFX adapter available
		IDXGIAdapter4* pAdapter = nullptr;
		std::vector<IDXGIAdapter4*> vAdapters;
		UINT adapterIndex = 0;
		for (UINT i = 0u; m_DXGIInterface->EnumAdapterByGpuPreference(i, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&pAdapter)) != DXGI_ERROR_NOT_FOUND; ++i)
			vAdapters.push_back(pAdapter);

		// Find the best GFX adapter on this machine
		SIZE_T maximumVideoMemory = 0;
		DXGI_ADAPTER_DESC adapterDescription;
		for (auto& currentAdapter : vAdapters)
		{
			// Get the description of this adapter
			// This will help identify the adapter's properties
			currentAdapter->GetDesc(&adapterDescription);

			// Select the best adapter based on the maximum video memory
			if (adapterDescription.DedicatedVideoMemory > maximumVideoMemory)
			{
				maximumVideoMemory = adapterDescription.DedicatedVideoMemory;
				pAdapter = currentAdapter;
			}
		}


		// Try to create a device on the selected adapter
		// We want to limit this application to adapter capable of handling DX12
		if (pAdapter)
		{
			if (FAILED(D3D12CreateDevice(pAdapter, D3D_FEATURE_LEVEL_12_2, IID_PPV_ARGS(m_Device.ReleaseAndGetAddressOf()))))
			{
				SKTBD_CORE_INFO("D3D12 Feature Level 12_2 not supported dropping to 12_1");
				SKTBD_CORE_INFO("D3D12 Feature Level 12_2 not supported! Excercise caution when using Skateboard interfaces to create D3D12 Objects, Refer to Microsoft for Gpu Feature Support");


				if (FAILED(D3D12CreateDevice(pAdapter, D3D_FEATURE_LEVEL_12_1, IID_PPV_ARGS(m_Device.ReleaseAndGetAddressOf()))))
				{
					SKTBD_CORE_INFO("D3D12 Feature Level 12_1 not supported dropping to 12_0");


					if (FAILED(D3D12CreateDevice(pAdapter, D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(m_Device.ReleaseAndGetAddressOf()))))
					{
						SKTBD_CORE_INFO("D3D12 Feature Level 12_0 not supported dropping to 11_0");

						if (FAILED(D3D12CreateDevice(pAdapter, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(m_Device.ReleaseAndGetAddressOf()))))
						{
							SKTBD_CORE_ERROR("Adapter doesnt support DX12, What are you using this on ????");
						}
					}
				}
			}
		}
		
		//check support for enhanced barriers, otherwise will have to make do with a Warp device
		D3D12_FEATURE_DATA_D3D12_OPTIONS12 options{};
		if(m_Device)
		m_Device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS12, &options, sizeof(options));

		if(!pAdapter || !options.EnhancedBarriersSupported)
		{
			// Fallback to a WARP device
			SKTBD_CORE_WARN(L"No GPU detected or Gpu lacks support for enhaned barriers, fallback to a warp device... rendering will be fully cpu, but most d3d features will be supported\n");

			if (pAdapter) pAdapter->Release();
			// Get a suitable WARP adapter and create a device with it
			D3D_CHECK_FAILURE(m_DXGIInterface->EnumWarpAdapter(IID_PPV_ARGS(&pAdapter)));

			// Create a device on the warp adapter. If failed, throw an exception as no adapter was selected
			D3D_CHECK_FAILURE(D3D12CreateDevice(pAdapter, D3D_FEATURE_LEVEL_12_2, IID_PPV_ARGS(m_Device.ReleaseAndGetAddressOf())));

			//// Get description and release memory
			//pAdapter->GetDesc(&adapterDescription);
			//pAdapter->Release(), pAdapter = nullptr;
		}

#ifndef SKTBD_SHIP
		// Output the selected device on the console
		std::wstring selectedAdapterInfo;
		if (pAdapter) pAdapter->GetDesc(&adapterDescription);
		selectedAdapterInfo += L"Selected device:\n\t";
		selectedAdapterInfo += adapterDescription.Description;
		selectedAdapterInfo += L"\n\tAvailable Dedicated Video Memory: ";
		selectedAdapterInfo += std::to_wstring(adapterDescription.DedicatedVideoMemory / 1000000000.f);
		selectedAdapterInfo += L" GB";
		SKTBD_CORE_INFO(selectedAdapterInfo.c_str());

		SKTBD_CORE_INFO("Registering Device for Debug Layer Messages");

		// Set up the info queue for the device
		// Debug layer must be enabled for this: so only perform this in debug
		// Note that means that m_InfoQueue should always be checked for existence before use
		if (!FAILED(m_Device->QueryInterface(IID_PPV_ARGS(&m_InfoQueue))))
		{
			// Set up message callback
			D3D_CHECK_FAILURE(m_InfoQueue->RegisterMessageCallback(D3DDebugTools::D3DMessageHandler, D3D12_MESSAGE_CALLBACK_FLAG_NONE, nullptr, &m_MessageCallbackCookie));
			D3D_CHECK_FAILURE(m_InfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, true));
			SKTBD_CORE_INFO("D3D Info Queue message callback created.");
		}
		else
		{
			SKTBD_CORE_WARN("D3D Info Queue interface not available! D3D device messages will not be received.");
		}

#endif

		//Initialise Memory allocator
		{
			D3D12MA::ALLOCATOR_DESC allocatorDesc = {};
			allocatorDesc.pDevice = m_Device.Get();
			allocatorDesc.pAdapter = pAdapter;
			// These flags are optional but recommended.
			allocatorDesc.Flags = (D3D12MA::ALLOCATOR_FLAGS) (D3D12MA::ALLOCATOR_FLAG_MSAA_TEXTURES_ALWAYS_COMMITTED | D3D12MA::ALLOCATOR_FLAG_DEFAULT_POOLS_NOT_ZEROED);

			HRESULT hr = D3D12MA::CreateAllocator(&allocatorDesc, &p_MemoryAllocator);
			D3D_CHECK_FAILURE(hr)
		}


		// Release unused adapters to clean memory
		for (auto& currentAdapter : vAdapters) currentAdapter->Release(), currentAdapter = nullptr;

		// Verify compatibility with raytracing
		D3D12_FEATURE_DATA_D3D12_OPTIONS5 featureSupportData = {};
		if (FAILED(m_Device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS5, &featureSupportData, sizeof(featureSupportData))) || featureSupportData.RaytracingTier == D3D12_RAYTRACING_TIER_NOT_SUPPORTED)
		{
			SKTBD_CORE_WARN(L"DirectX Raytracing unsupported on this hardware. Raytracing functionalities disabled.");
			return;
		}

		// If compatible, enable raytracing support on this application
		m_HasDXR = true;
	}

	void D3DGraphicsContext::CreateFence()
	{
		SKTBD_CORE_TRACE("Creating fence..");

		// Create a fence object for CPU/GPU synchornisation
		D3D_CHECK_FAILURE(m_Device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(m_Fence.ReleaseAndGetAddressOf())));
	}

	void D3DGraphicsContext::CreateDescriptorSizes()
	{
		// When we will work with descriptors, we are going to need their sizes
		// Descriptor sizes can vary across GPUs, so we need to query this information
		// We cache the descriptors sizes on this class so that it is available when we need them for various descriptor types
		// It could be queried every time we need them instead

		m_RTVDescriptorSize = m_Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
		m_DSVDescriptorSize = m_Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
		m_CBVSRVUAVDescriptorSize = m_Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	}

	void D3DGraphicsContext::Check4xMSAAQualitySupport()
	{
		// Check for 4x MultiSample Anti-Aliasing support
		// 4x is chosen because it guaranteed to be supported by DX12 hardware
		// and because it provides good improvement without being too expensive
		// However, we do have to check for the supported quality level

		D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS multisampleQualityLevels = {};
		multisampleQualityLevels.Format = m_BackBufferFormat;							//
		multisampleQualityLevels.SampleCount = 4;										// 4 samples for a 4x MSAA
		multisampleQualityLevels.Flags = D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE;	//
		multisampleQualityLevels.NumQualityLevels = 0;									// Initially 0, will be determined by the next call

		D3D_CHECK_FAILURE(m_Device->CheckFeatureSupport(D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS, &multisampleQualityLevels, sizeof(multisampleQualityLevels)));

		// Since 4x should natively be supported, m_MSAAQuality should always be greater than 0
		m_MSAAQuality = multisampleQualityLevels.NumQualityLevels;
		SKTBD_ASSERT_M(m_MSAAQuality > 0, "Unexpected MSAA quality level.");
	}

	void D3DGraphicsContext::CreateCommandQueueAndCommandList()
	{
		SKTBD_CORE_TRACE("Creating command queue and command list..");

		// Start with describing the command queue
		D3D12_COMMAND_QUEUE_DESC queueDesc = {};
		queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;	// Specifies a command buffer that the GPU can execute. A direct command list doesn't inherit any GPU state.
		queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;	// No particular flags

		// Create the command queue
		D3D_CHECK_FAILURE(m_Device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(m_CommandQueue.ReleaseAndGetAddressOf())));

		//Create Copy Queue
		queueDesc.Type = D3D12_COMMAND_LIST_TYPE_COPY;

		D3D_CHECK_FAILURE(m_Device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(m_CopyCommandQueue.ReleaseAndGetAddressOf())));

		m_DefaultGraphicsCB.ForEachMemoryHandle(CreateCommandBuffer(CommandBufferType_Graphics, CommandBufferPriority_Primary));
	}

	void D3DGraphicsContext::CreateSwapChain()
	{
		SKTBD_CORE_TRACE("Creating swap chain..");

		// First reset any previous swap chain that was created, in the event that a new swap chain is being created during runtime (i.e. if the user changes some settings!)
		m_SwapChain.Reset();

		//// Then describe the characteristics of the swap chain
		//// DXGI_SWAP_CHAIN_DESC contains a structure DXGI_MODE_DESC inside, which represents the properties of the back buffer
		//// The main properties we are concerned with are width and height, as well as pixel format
		//DXGI_SWAP_CHAIN_DESC swapchainDesc = {};
		//swapchainDesc.BufferDesc.Width = m_ClientWidth;
		//swapchainDesc.BufferDesc.Height = m_ClientHeight;
		//swapchainDesc.BufferDesc.RefreshRate.Numerator = 60;								// 60/1 = 60 Hertz; Note: this seems to have no real effect on the refresh rate
		//swapchainDesc.BufferDesc.RefreshRate.Denominator = 1;
		//swapchainDesc.BufferDesc.Format = m_BackBufferFormat;
		//swapchainDesc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
		//swapchainDesc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;					// Scaling (how the image is stretched over the monitor) is unspecified
		//swapchainDesc.SampleDesc.Count = 1;													// MSAA is not supported on pipeline state buffers anymore, so sample count is 1 and quality is 0
		//swapchainDesc.SampleDesc.Quality = 0;												// Use MSAA on render targets instead
		//swapchainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;						// Use the back buffer as a render target
		//swapchainDesc.BufferCount = g_SwapChainBufferCount;									// Single, Double or Triple Buffering
		//swapchainDesc.OutputWindow = m_MainWindow;											// Reference the window to which we need to swap these buffers onto
		//swapchainDesc.Windowed = true;														// Always starts in windowed mode, the application layer is responsible for fullscreen
		//swapchainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;							// Discard the content of the back buffer after you present it. All Windows Store apps must use a flip effect
		//swapchainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH | DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;// Allow to switch to fullscreen or windowed mode in runtime

		//// Create the swapchain
		//// It uses the queue to perform a flush
		//D3D_CHECK_FAILURE(m_DXGIInterface->CreateSwapChain(m_CommandQueue.Get(), &swapchainDesc, m_SwapChain.ReleaseAndGetAddressOf()));

		// Describe and create the swap chain.
		DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
		swapChainDesc.BufferCount = GRAPHICS_SETTINGS_NUMFRAMERESOURCES;
		swapChainDesc.Width = m_ClientWidth;
		swapChainDesc.Height = m_ClientHeight;
		swapChainDesc.Format = m_BackBufferFormat;
		swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
		swapChainDesc.SampleDesc.Count = 1;
		swapChainDesc.SampleDesc.Quality = 0;
		swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH | DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;

		ComPtr<IDXGISwapChain1> swapChain;
		D3D_CHECK_FAILURE(m_DXGIInterface->CreateSwapChainForHwnd(
			m_CommandQueue.Get(),        // Swap chain needs the queue so that it can force a flush on it.
			m_MainWindow,
			&swapChainDesc,
			nullptr,
			nullptr,
			&swapChain
		));

		D3D_CHECK_FAILURE(swapChain.As(&m_SwapChain));
	}

	void D3DGraphicsContext::Resize(int clientWidth, int clientHeight)
	{
		// Assign new dimensions if existing (we don't want a 0 dimension, would crash)
		SKTBD_ASSERT_M(clientWidth || clientHeight, "Impossible client surface dimensions");

		m_ClientWidth = clientWidth;
		m_ClientHeight = clientHeight;
		m_ClientResized = true;
	}

	void D3DGraphicsContext::OnResized()
	{
		// Do not perform unnecessary resizing
		if (!m_ClientResized)
			return;

		SKTBD_CORE_INFO("Resizing back buffers, two GPU flush expected..");

		// This function will be called everytime the application is being resized
		// It needs to re-create RTVs, a DSV, a viewport and scissor rectangles
		// First verify that we have all the required objects
		SKTBD_ASSERT_M(m_Device, "Device does not exists when trying to resize buffers!");
		SKTBD_ASSERT_M(m_SwapChain, "Swapchain does not exists when trying to resize buffers!");
		SKTBD_ASSERT_M(GetDefaultCommandAllocator(), "Command allocator does not exists when trying to resize buffers!");

		// Flush before changing any resource
		WaitUntilIdle();

		// Reset the command list to a NULL state
		D3D_CHECK_FAILURE(GetDefaultCommandList()->Reset(GetDefaultCommandAllocator(), nullptr));

		// Release the previous resources we will be recreating.
		for (int i = 0; i < g_SwapChainBufferCount; ++i) m_SwapChainBuffers[i].Reset();

		if (m_DepthStencilBuffer.GetResource())
		{
			static_cast<ID3D12Resource*>(m_DepthStencilBuffer.GetResource())->Release();
			m_DSVDescriptorHeap.Free(*static_cast<D3DDescriptorHandle*>(m_DefaultDSV.GetResource()));
			delete static_cast<D3DDescriptorHandle*>(m_DefaultDSV.GetResource());
		}

		// Resize the swap chain.
		D3D_CHECK_FAILURE(m_SwapChain->ResizeBuffers(g_SwapChainBufferCount, m_ClientWidth, m_ClientHeight, m_BackBufferFormat, DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH | DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING));

		// Reset the current back buffer to the first so we can properly re-create the render targets
		m_CurrentBackBuffer = 0;

		// Create new render target views
		CreateRenderTargetViews();

		// Create the detph/stencil buffer view
		CreateDepthStencilBuffer();

		D3D12_TEXTURE_BARRIER text_b = {};

		text_b.pResource = static_cast<ID3D12Resource*>(m_DepthStencilBuffer.GetResource());
		text_b.LayoutBefore = D3D12_BARRIER_LAYOUT_UNDEFINED;
		text_b.LayoutAfter = D3D12_BARRIER_LAYOUT_DEPTH_STENCIL_WRITE;

		text_b.AccessBefore = D3D12_BARRIER_ACCESS_NO_ACCESS;
		text_b.AccessAfter = D3D12_BARRIER_ACCESS_DEPTH_STENCIL_WRITE;

		text_b.SyncBefore = D3D12_BARRIER_SYNC_NONE;
		text_b.SyncAfter = D3D12_BARRIER_SYNC_DEPTH_STENCIL;
		text_b.Subresources = CD3DX12_BARRIER_SUBRESOURCE_RANGE(0xffffffff);
		text_b.Flags = D3D12_TEXTURE_BARRIER_FLAG_NONE;

		//CD3DX12_TEXTURE_BARRIER()

		D3D12_BARRIER_GROUP b_group[1];
		b_group[0].NumBarriers = 1;
		b_group[0].Type = D3D12_BARRIER_TYPE_TEXTURE;
		b_group[0].pTextureBarriers = &text_b;

	//	GetDefaultCommandList()->Barrier(1, b_group);

		//// Transition the state of the depth/stencil resource from its initial state so it can be used as a depth buffer
		//D3D12_RESOURCE_BARRIER barrier = {};
		//barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;						// A transition barrier that indicates a transition of a set of subresources between different usages
		//barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;							// 
		//barrier.Transition.pResource = static_cast<ID3D12Resource*>(m_DepthStencilBuffer.GetResource());					// The resources in transition
		//barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COMMON;				// The "before" usages of the SUBresources
		//barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_DEPTH_WRITE;			// The "after" usages of the subresources
		//barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;	// The index of the subresource for transition: here we transition all the subresources at the same time
		//GetDefaultCommandList()->ResourceBarrier(1, &barrier);

		// Execute the resize commands
		Flush();

		// Update the viewport transform to cover the client area
		SetViewPort();

		// Update the scissor rectangles
		SetScissorRectangles();

		// Reset resized tracker
		m_ClientResized = false;
	}

	void D3DGraphicsContext::CreateDescriptorHeaps()
	{
		// Create basic descriptor heaps
		m_RWSRVDescriptorHeap.Create(L"Main Copy Descriptor Heap", D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 4096, false);

		m_SRVDescriptorHeap.Create(L"Main SRV Descriptor Heap", D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 4096, true);
		m_SamplerHeap.Create(L"Main Sampler Heap", D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, 1024, true);

		m_RTVDescriptorHeap.Create(L"Main RTV Descriptor Heap", D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 4096, false);
		m_DSVDescriptorHeap.Create(L"Main DSV Descriptor Heap", D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 4096, false);


		// Allocate the backbuffers and depth/stencil buffer
		m_ImGuiHandle = m_SRVDescriptorHeap.Allocate(1);
	}

	void D3DGraphicsContext::CreateUploadManager()
	{
		m_UploadManager.Init(p_MemoryAllocator, UPLOAD_MANAGER_SIZE);
	}

	void D3DGraphicsContext::CreateRenderTargetViews()
	{
		// Get the descriptor handle of the first buffer in the heap
		//D3D12_CPU_DESCRIPTOR_HANDLE rtvHeapHandle = m_RTVHeap->GetCPUDescriptorHandleForHeapStart();
		//D3DDescriptorHandle rtvHeapHandle = m_RTVDescriptorHandle;
		
		auto init = [this, i = 0](void** memHandle) mutable {

			D3D_CHECK_FAILURE(m_SwapChain->GetBuffer(i, IID_PPV_ARGS(m_SwapChainBuffers[i].ReleaseAndGetAddressOf())));

			auto handle = m_RTVDescriptorHeap.Allocate();
			m_Device->CreateRenderTargetView(m_SwapChainBuffers[i].Get(), nullptr, handle.GetCPUHandle());
			*memHandle = new D3DDescriptorHandle(std::move(handle));

			++i;
		};
		// init 
		m_SwapChainRTVs.ForEachMemoryHandle(init);
	}

	void D3DGraphicsContext::CreateDepthStencilBuffer()
	{
		// The depth/stencil buffer is a 2D texture, which differs from the render target views
		// Therefore, we must first initialise a D3D12_RESOURCE_DESC with the given charasteristics for this resource

		TextureDesc Desc;
		Desc.Dimension = TextureDimension_Texture2D;				// Specify the dimensions of this resource
		Desc.Width = m_ClientWidth;									// The width of the texture in texels; for buffers, this is the number of bytes in the buffer
		Desc.Height = m_ClientHeight;								// The height of the texture in texels
		Desc.Depth = 1;												// The depth of the texture in texels, or the texture array size (for 1D and 2D textures)
		Desc.Mips = 1;												// 
		Desc.Format = DataFormat_DEFAULT_DEPTHSTENCIL;				// 
		Desc.Type = TextureType_DepthStencil;
		Desc.AccessFlags = ResourceAccessFlag_GpuRead | ResourceAccessFlag_GpuWrite;
		Desc.Clear.Depth = GRAPHICS_DEPTH_DEFAULT_CLEAR_COLOUR;
		Desc.Clear.Stencil = GRAPHICS_STENCIL_DEFAULT_CLEAR_COLOUR;

		ResourceFactory::CreateTextureResource(m_DepthStencilBuffer, L"DefaultDepthBuffer", Desc);

		// Create the Depth/Stencil view
		// If the resource was created with a typed format (i.e. not typeless), then the D3D12_DEPTH_STENCIL_VIEW_DESC parameter can be null
		// It indicates to create a view to the first mipmap level of this resource with the format the resource was created with
		ResourceFactory::CreateDepthRenderTargetView(m_DefaultDSV, L"DefaultDSV", &m_DepthStencilBuffer, nullptr);
	}

	void D3DGraphicsContext::SetViewPort()
	{
		// Describe the viewport charasteristics to fit the entire window with a normal depth buffer range
		m_Viewport.TopLeftX = 0.f;
		m_Viewport.TopLeftY = 0.f;
		m_Viewport.Width = static_cast<float>(m_ClientWidth);
		m_Viewport.Height = static_cast<float>(m_ClientHeight);
		m_Viewport.MinDepth = 0.f;
		m_Viewport.MaxDepth = 1.f;
	}

	void D3DGraphicsContext::SetScissorRectangles()
	{
		// For now we don't have any HUDs, so assign the area to fit the client surface
		m_ScissorRect = { 0, 0, m_ClientWidth, m_ClientHeight };
	}

	void D3DGraphicsContext::Reset()
	{
		SKTBD_CORE_WARN("Reset called and this might be bad!");
		D3D_CHECK_FAILURE(GetDefaultCommandList()->Reset(GetDefaultCommandAllocator(), nullptr));
	}

	void D3DGraphicsContext::Flush()
	{
		// Execute the new commands and wait until completion
		D3D_CHECK_FAILURE(GetDefaultCommandList()->Close());

		auto cb = &GetDefaultCommandBuffer();

		SubmitCommandBuffers(&cb, 1);
		WaitUntilIdle();

		//Really just needs a Update function;
		m_UploadManager.Update();
	}

	void D3DGraphicsContext::WaitUntilIdle()
	{
		SKTBD_CORE_WARN("Flushing GPU commands -- Woosh!");

		// Advance the fence value to mark commands up to this fence point
		uint64_t currentFence = NextFence();

		// Set a new fence point on the command queue. Because we are on the GPU timeline, the new fence point won't
		// be set until the GPU finishes processing all the commands prior to this Signal()
		D3D_CHECK_FAILURE(m_CommandQueue->Signal(m_Fence.Get(), currentFence));

		// Wait until the GPU has completed commands up to the fence point
		if (m_Fence->GetCompletedValue() < currentFence)
		{
			HANDLE eventHandle = CreateEventEx(nullptr, NULL, NULL, EVENT_ALL_ACCESS);
			SKTBD_ASSERT_M(eventHandle != NULL, "Fence event created an invalid handle!");

			// Fire the previously created event when GPU hits current fence
			D3D_CHECK_FAILURE(m_Fence->SetEventOnCompletion(currentFence, eventHandle));

			// Wait until the GPU hits the current fence and the event is received
			WaitForSingleObject(eventHandle, INFINITE);

			// Close the event so that it does not exist anymore
			CloseHandle(eventHandle);
		}
	}

	void D3DGraphicsContext::Present()
	{
		// Get required API objects
		ID3D12GraphicsCommandList* commandList = GetDefaultCommandList();
		ID3D12CommandQueue* commandQueue = CommandQueue();

		// Swap the back and front buffers
		// Arg1 - An integer that specifies how to synchronize presentation of a frame with the vertical blank:
		//	0 - Cancel the remaining time on the previously presented frame and discard this frame if a newer frame is queued
		// Arg2 - An integer value that contains swap-chain presentation options. These options are defined by the DXGI_PRESENT constants
		//D3D_CHECK_FAILURE(m_SwapChain->Present(m_Vsync, 0));
		if (FAILED(m_SwapChain->Present(m_Vsync, 0)))
		{
			HRESULT hr = Device()->GetDeviceRemovedReason();
			D3D_CHECK_FAILURE(hr);
		}

		// Update the index to the current back buffer so that we render to the other buffer next frame
		NextBackBuffer();
	}

	off_t D3DGraphicsContext::GetViewHeapIndex(const ShaderView* View) const
	{
		if (View)
			return static_cast<D3DDescriptorHandle*>(View->GetResource())->GetIndex();
		else return 0;
	}

	//TODO
	CopyResult D3DGraphicsContext::CopyDataToBuffer(Buffer* dest, off_t offset, size_t size, void* src)
	{
		SKTBD_ASSERT_M(dest, "Can not copy to null or from null");
		auto DEST = static_cast<D3D12MA::Allocation*>(dest->GetResource())->GetResource();

		if (dest->GetAccessFlags() & ResourceAccessFlag_CpuWrite)
		{
			// Copy the triangle data to the vertex buffer.
			UINT8* pDataBegin;
			CD3DX12_RANGE readRange(0, 0);        // We do not intend to read from this resource on the CPU.
			D3D_CHECK_FAILURE(DEST->Map(0, &readRange, reinterpret_cast<void**>(&pDataBegin)));
			memcpy(pDataBegin + offset, src, size);
			DEST->Unmap(0, nullptr);
		}
		else
		{
			m_UploadManager.UploadBuffer(DEST, offset, src, size);
		}

		return CopyResult();
	}

	CopyResult D3DGraphicsContext::CopyDataToBuffer(Buffer* dest, off_t offset, size_t size, std::function<void(void*)> WriterFunct)
	{
		SKTBD_ASSERT_M(dest, "Can not copy to null or from null");

		auto DEST = static_cast<D3D12MA::Allocation*>(dest->GetResource())->GetResource();

		if (dest->GetAccessFlags() & ResourceAccessFlag_CpuWrite)
		{
			// Copy the triangle data to the vertex buffer.
			UINT8* pDataBegin;
			CD3DX12_RANGE readRange(0, 0);        // We do not intend to read from this resource on the CPU.
			D3D_CHECK_FAILURE(DEST->Map(0, &readRange, reinterpret_cast<void**>(&pDataBegin)));
			WriterFunct(pDataBegin + offset);
			DEST->Unmap(0, nullptr);
		}
		else
		{
			m_UploadManager.UploadBuffer(DEST, offset, size, WriterFunct);
		}

		return CopyResult();
	}

	/*Microsoft::WRL::ComPtr<ID3D12CommandAllocator> D3DGraphicsContext::CreateCommandAllocator(ComPtr<ID3D12Device2> device, D3D12_COMMAND_LIST_TYPE type)
	{
		ComPtr<ID3D12CommandAllocator> commandAllocator;
		device->CreateCommandAllocator(type, IID_PPV_ARGS(&commandAllocator));

		return commandAllocator;
	}

	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> D3DGraphicsContext::GetAvailableCommandAllocator(D3D12_COMMAND_LIST_TYPE type)
	{
		if (m_AvailableCommandAllocators[type].empty())
		{
			return CreateCommandAllocator(m_Device, type);
		}
		else
		{
			auto ret = m_AvailableCommandAllocators[type].top();
			m_AvailableCommandAllocators[type].pop();
			return ret;
		}
			
	}*/

	void D3DGraphicsContext::BeginCommandBuffer(CommandBuffer& cb)
	{
		auto& handles = *static_cast<CommandBufferMemHandle*>(cb.GetResource());

		handles.m_Allocator->Reset();
		handles.m_Commandlist->Reset(handles.m_Allocator.Get(), nullptr);
	}

	void D3DGraphicsContext::EndCommandBuffer(CommandBuffer& cb)
	{
		auto& handles = *static_cast<CommandBufferMemHandle*>(cb.GetResource());
		handles.m_Commandlist->Close();
	}

	void D3DGraphicsContext::SubmitCommandBuffers(CommandBuffer** cb, const uint32_t count)
	{
		std::vector<ID3D12CommandList*> CommandLists = std::vector<ID3D12CommandList*>(count);
		std::transform(cb, &cb[count], CommandLists.begin(), [](CommandBuffer* cb) -> ID3D12CommandList* { return static_cast<CommandBufferMemHandle*>(cb->GetResource())->m_Commandlist.Get(); });

		m_CommandQueue->ExecuteCommandLists(count, CommandLists.data());
	}

	void D3DGraphicsContext::SetDeferredReleasesFlag()
	{
		m_DeferredFlags[GetCurrentFrameResourceIndex()] = 1;
	}

	void D3DGraphicsContext::DeferredRelease(IUnknown* resource)
	{
		const uint64_t frame = GetCurrentFrameResourceIndex();

		// Lock this function, prevents threads from over writing deferrals.
		std::lock_guard lock(m_DeferredMutex);

		m_DeferredReleases[frame].push_back(resource);
		SetDeferredReleasesFlag();
	}

	void __declspec(noinline) D3DGraphicsContext::ProcessDeferrals()
	{
		std::lock_guard lock(m_DeferredMutex);

		m_DeferredFlags[m_CurrentFrameResourceIndex] = 0;

		// Process deferred descriptors.
		m_RWSRVDescriptorHeap.ProcessDeferredFree(m_CurrentFrameResourceIndex);
		m_SRVDescriptorHeap.ProcessDeferredFree(m_CurrentFrameResourceIndex);
		m_RTVDescriptorHeap.ProcessDeferredFree(m_CurrentFrameResourceIndex);
		m_DSVDescriptorHeap.ProcessDeferredFree(m_CurrentFrameResourceIndex);

		//Clear the deferred resources.
		std::vector<IUnknown*>& resources{ m_DeferredReleases[m_CurrentFrameResourceIndex] };
		if(!resources.empty())
		{
			for(const auto& resource : resources)
			{
				resource->Release();
			}
			resources.clear();
		}

	}

	std::function<void(void**)> D3DGraphicsContext::GetBufferMemoryInitializer(const BufferDesc& desc)
	{
		D3D12_RESOURCE_DESC1 buffdesc = {};
		buffdesc.Alignment = 0;//(desc.Alignment + SKTBD_DESKTOP_PLATFORM_MIN_BUFFER_ALIGNMENT) & ~SKTBD_DESKTOP_PLATFORM_MIN_BUFFER_ALIGNMENT;
		buffdesc.DepthOrArraySize = 1;
		buffdesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		buffdesc.Flags |= (desc.Flags & BufferFlags_RaytacingStructures) ? D3D12_RESOURCE_FLAG_RAYTRACING_ACCELERATION_STRUCTURE : D3D12_RESOURCE_FLAG_NONE;
		buffdesc.Flags |= (desc.AccessFlags & ResourceAccessFlag_GpuRead) ? D3D12_RESOURCE_FLAG_NONE : D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;
		buffdesc.Flags |= (desc.AccessFlags & ResourceAccessFlag_GpuWrite) ? D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS : D3D12_RESOURCE_FLAG_NONE;
		buffdesc.Format = DXGI_FORMAT_UNKNOWN;
		buffdesc.Height = 1;
		buffdesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		buffdesc.MipLevels = 1;
		buffdesc.SampleDesc.Count = 1;
		buffdesc.SampleDesc.Quality = 0;
		buffdesc.Width = desc.Size;

		D3D12MA::ALLOCATION_DESC allocdesc = {};
		allocdesc.HeapType = [desc]() ->D3D12_HEAP_TYPE
		{
			if ((desc.AccessFlags & ResourceAccessFlag_CpuWrite) && !(desc.AccessFlags & ResourceAccessFlag_DesktopPlatformPrimaryResidenceGPU))
				return D3D12_HEAP_TYPE_UPLOAD;
			else if (desc.AccessFlags == ResourceAccessFlag_CpuRead)
				return D3D12_HEAP_TYPE_READBACK;
			else
				return D3D12_HEAP_TYPE_DEFAULT;
		}();

		return [buffdesc, allocdesc, this](void** m_memoryHandle)
		{
				return GetMemoryAllocator()->CreateResource3(
					&allocdesc,
					&buffdesc,
					D3D12_BARRIER_LAYOUT_UNDEFINED,
					0,
					0,
					NULL,
					reinterpret_cast<D3D12MA::Allocation**>(m_memoryHandle),
					IID_NULL,
					nullptr
				);
		};

	}

	std::function<void(void**)> D3DGraphicsContext::GetTextureMemoryInitializer(const TextureDesc& desc)
	{
		D3D12_RESOURCE_DESC1 texturedesc = {};
		texturedesc.Alignment = 0;
		texturedesc.DepthOrArraySize = desc.Depth;
		texturedesc.Dimension = SkateboardTextureDimensionToD3D(desc.Dimension);

		texturedesc.Flags |= (desc.AccessFlags & ResourceAccessFlag_GpuRead) ? D3D12_RESOURCE_FLAG_NONE : D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;

		//Depth Stencil is not compatible with this flag
		if (desc.Type != TextureType_DepthStencil)
			texturedesc.Flags |= (desc.AccessFlags & ResourceAccessFlag_GpuWrite) ? D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS : D3D12_RESOURCE_FLAG_NONE;

		texturedesc.Flags |= (desc.Type == TextureType_RenderTarget) ? D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET : D3D12_RESOURCE_FLAG_NONE;
		texturedesc.Flags |= (desc.Type == TextureType_DepthStencil) ? D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL : D3D12_RESOURCE_FLAG_NONE;
		texturedesc.Format = SkateboardBufferFormatToD3D(desc.Format);
		texturedesc.Width = desc.Width;
		texturedesc.Height = desc.Height;
		texturedesc.MipLevels = desc.Mips;
		texturedesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
		texturedesc.SampleDesc.Count = 1;
		texturedesc.SampleDesc.Quality = 0;

		SKTBD_ASSERT_M((desc.AccessFlags & (ResourceAccessFlag_GpuRead | ResourceAccessFlag_GpuWrite)) && (desc.Type != TextureType_Default), "RenderTargets Must be GPU Read/Write Access")

			D3D12MA::ALLOCATION_DESC allocdesc = {};
		allocdesc.HeapType = [desc]()->D3D12_HEAP_TYPE
			{
				if (desc.Type != TextureType_Default) return D3D12_HEAP_TYPE_DEFAULT;

				if (desc.AccessFlags & ResourceAccessFlag_GpuWrite | ResourceAccessFlag_DesktopPlatformPrimaryResidenceGPU)
					return D3D12_HEAP_TYPE_DEFAULT;
				else if (desc.AccessFlags == ResourceAccessFlag_CpuRead)
					return D3D12_HEAP_TYPE_READBACK;
				else
					return D3D12_HEAP_TYPE_UPLOAD;
			}();

		auto layout = D3D12_BARRIER_LAYOUT_COMMON;
		D3D12_CLEAR_VALUE clear = {};

		clear.Format = SkateboardBufferFormatToD3D(desc.Format);

		if (desc.Type == TextureType_DepthStencil)
		{
			layout = D3D12_BARRIER_LAYOUT_DEPTH_STENCIL_WRITE;
			clear.DepthStencil.Depth = desc.Clear.Depth;
			clear.DepthStencil.Stencil = desc.Clear.Stencil;
		}
		if (desc.Type == TextureType_RenderTarget)
		{
				layout = D3D12_BARRIER_LAYOUT_RENDER_TARGET;
				memcpy(&clear.Color, &desc.Clear.RenderClearValue, sizeof(glm::float4));
		}

		auto allocator = GetMemoryAllocator();

		return [allocdesc, texturedesc, layout, clear, allocator](void** m_memoryHandle)
			{
				return allocator->CreateResource3(
					&allocdesc,
					&texturedesc,
					layout,
					&clear,
					0,
					NULL,
					reinterpret_cast<D3D12MA::Allocation**>(m_memoryHandle),
					IID_NULL,
					nullptr
				);
			};

	}

#ifdef SKTBD_DEBUG
	std::function<void(void**, const std::wstring& debugname)> D3DGraphicsContext::RegisterMemoryResource(GPUResourceType_ Type)
	{
		switch (Type)
		{
		case Skateboard::GpuResourceType_Buffer:
		case Skateboard::GpuResourceType_Texture:

			return [](void** m_memoryHandle, const std::wstring& debugname)
				{
					auto handle = *reinterpret_cast<D3D12MA::Allocation**>(m_memoryHandle);
					handle->SetName(debugname.c_str());
					return handle->GetResource()->SetName(debugname.c_str());

				};

		break;

		/*case Skateboard::GpuResourceType_ShaderView:
			break;
		case Skateboard::GpuResourceType_DescriptorTable:
			break;
		case Skateboard::GpuResourceType_RenderTargetView:
			break;
		case Skateboard::GpuResourceType_DepthStencilView:
			break;
		case Skateboard::GpuResourceType_ShaderInputsLayout:
			break;
		case Skateboard::GpuResourceType_PipelineState:
			break;
		case Skateboard::GpuResourceType_CommandBuffer:
			break;*/

		default:
			return [](void**, const std::wstring& debugname) { SKTBD_CORE_INFO("Resource type cant be registed or already is registered with the debug system"); return OOPS_FAIL; };
			break;
		}
	}
#endif // SKTBD_DEBUG

	std::function<void(void**)> D3DGraphicsContext::CreateDepthRenderTargetView(void* Resource, DepthStencilDesc* desc)
	{
		if (desc)
		{
			D3D12_DEPTH_STENCIL_VIEW_DESC DSTdesc = {};

			DSTdesc.Format = SkateboardBufferFormatToD3D(desc->Format);

			switch (desc->Dimension)
			{
			case TextureDimension_Texture1D:
				DSTdesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE1D;
				DSTdesc.Texture1D.MipSlice = desc->MipSlice;
				break;
			case TextureDimension_Texture1D_ARRAY:
				DSTdesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE1DARRAY;
				DSTdesc.Texture1DArray.ArraySize = desc->ArraySize;
				DSTdesc.Texture1DArray.FirstArraySlice = desc->FirstArraySlice;
				DSTdesc.Texture1DArray.MipSlice = desc->MipSlice;
				break;
			case TextureDimension_Texture2D:
				DSTdesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
				DSTdesc.Texture2D.MipSlice = desc->MipSlice;
				break;
			case TextureDimension_Texture2D_ARRAY:
				DSTdesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DARRAY;
				DSTdesc.Texture2DArray.ArraySize = desc->ArraySize;
				DSTdesc.Texture2DArray.FirstArraySlice = desc->FirstArraySlice;
				DSTdesc.Texture2DArray.MipSlice = desc->MipSlice;
				break;
			case TextureDimension_Texture2D_MS:
				DSTdesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DMS;
				break;
			case TextureDimension_Texture2D_MS_ARRAY:
				DSTdesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DMSARRAY;
				DSTdesc.Texture2DArray.ArraySize = desc->ArraySize;
				DSTdesc.Texture2DArray.FirstArraySlice = desc->FirstArraySlice;
				break;
			case TextureDimension_Texture3D:
				SKTBD_CORE_WARN("3D Depth Render Targets are illegal");
				SKTBD_ASSERT(FALSE);
				break;
			case TextureDimension_CubeMap:
				DSTdesc.ViewDimension = D3D12_DSV_DIMENSION_UNKNOWN;
				break;

			default:
				DSTdesc.ViewDimension = D3D12_DSV_DIMENSION_UNKNOWN;
				break;
			}

			DSTdesc.Flags |= (desc->Flags & DRT_Flags_::DRT_Flags_ReadOnly_Depth) ? D3D12_DSV_FLAG_READ_ONLY_DEPTH : D3D12_DSV_FLAG_NONE;
			DSTdesc.Flags |= (desc->Flags & DRT_Flags_::DRT_Flags_ReadOnly_Stencil) ? D3D12_DSV_FLAG_READ_ONLY_STENCIL : D3D12_DSV_FLAG_NONE;

			auto device = Device();
			auto* Heap = &m_DSVDescriptorHeap;

			return [DSTdesc, device, Heap, Resource](void** m_memoryHandle)
			{
					auto handle = Heap->Allocate();
					device->CreateDepthStencilView((Resource) ? reinterpret_cast<D3D12MA::Allocation*>(Resource)->GetResource() : nullptr, &DSTdesc, handle.GetCPUHandle());
					*m_memoryHandle = new D3DDescriptorHandle(std::move(handle));
					return OKEYDOKEY;
			};
		}
		else
		{
			auto device = Device();
			auto* Heap = &m_DSVDescriptorHeap;

			return [device, Heap, Resource](void** m_memoryHandle)
				{
					auto handle = Heap->Allocate();
					device->CreateDepthStencilView((Resource) ? reinterpret_cast<D3D12MA::Allocation*>(Resource)->GetResource() : nullptr, nullptr, handle.GetCPUHandle());
					*m_memoryHandle = new D3DDescriptorHandle(std::move(handle));
					return OKEYDOKEY;
				};
			};
	}

	std::function<void(void**)> D3DGraphicsContext::CreateRenderTargetView(void* Resource, RenderTargetDesc* desc)
	{
		if (desc)
		{
			D3D12_RENDER_TARGET_VIEW_DESC RTdesc = {};

			RTdesc.Format = SkateboardBufferFormatToD3D(desc->Format);

			switch (desc->Dimension)
				{
				case TextureDimension_Texture1D:
					RTdesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE1D;
					RTdesc.Texture1D.MipSlice = desc->MipSlice;
					break;
				case TextureDimension_Texture1D_ARRAY:
					RTdesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE1DARRAY;
					RTdesc.Texture1DArray.ArraySize = desc->ArraySize;
					RTdesc.Texture1DArray.FirstArraySlice = desc->FirstArraySlice;
					RTdesc.Texture1DArray.MipSlice = desc->MipSlice;
					break;
				case TextureDimension_Texture2D:
					RTdesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
					RTdesc.Texture2D.MipSlice = desc->MipSlice;
					RTdesc.Texture2D.PlaneSlice = desc->PlaneSlice;
					break;
				case TextureDimension_Texture2D_ARRAY:
					RTdesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
					RTdesc.Texture2DArray.ArraySize = desc->ArraySize;
					RTdesc.Texture2DArray.FirstArraySlice = desc->FirstArraySlice;
					RTdesc.Texture2DArray.MipSlice = desc->MipSlice;
					RTdesc.Texture2DArray.PlaneSlice = desc->PlaneSlice;
					break;
				case TextureDimension_Texture2D_MS:
					RTdesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DMS;
					break;
				case TextureDimension_Texture2D_MS_ARRAY:
					RTdesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DMSARRAY;
					RTdesc.Texture2DArray.ArraySize = desc->ArraySize;
					RTdesc.Texture2DArray.FirstArraySlice = desc->FirstArraySlice;
					break;
				case TextureDimension_Texture3D:
					RTdesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DMSARRAY;
					RTdesc.Texture3D.FirstWSlice = desc->FirstWSlice;
					RTdesc.Texture3D.WSize = desc->Width;
					RTdesc.Texture3D.MipSlice = desc->MipSlice;
				
					break;
				case TextureDimension_CubeMap:
					SKTBD_CORE_ERROR("Cube Map RTVs not supported");
					RTdesc.ViewDimension = D3D12_RTV_DIMENSION_UNKNOWN;
					break;

				default:
					RTdesc.ViewDimension = D3D12_RTV_DIMENSION_UNKNOWN;
					break;
				}

			return [RTdesc, this, Resource](void** m_memoryHandle)
			{
				auto handle = m_RTVDescriptorHeap.Allocate();
				m_Device->CreateRenderTargetView((Resource) ? reinterpret_cast<D3D12MA::Allocation*>(Resource)->GetResource() : nullptr, &RTdesc, handle.GetCPUHandle());
				*m_memoryHandle = new D3DDescriptorHandle(std::move(handle));			
				return OKEYDOKEY;
			};
		}
		else
		{
			return [desc, this, Resource](void** m_memoryHandle)
			{
				auto handle = m_RTVDescriptorHeap.Allocate();
				m_Device->CreateRenderTargetView((Resource) ? reinterpret_cast<D3D12MA::Allocation*>(Resource)->GetResource() : nullptr, nullptr, handle.GetCPUHandle());
				*m_memoryHandle = new D3DDescriptorHandle(std::move(handle));
				return OKEYDOKEY;
			};
		}
		
	}

	std::function<void(void**)> D3DGraphicsContext::CreateTextureView(void* Resource, ViewAccessType_ ViewAccessType, TextureViewDesc* desc)
	{
		D3D12_SHADER_RESOURCE_VIEW_DESC SRVdesc = {};
		D3D12_UNORDERED_ACCESS_VIEW_DESC UAVdesc = {};

		switch (ViewAccessType)
		{
			case ViewAccessType_::ViewAccessType_GpuRead:
				if (desc)
				{
						
					SRVdesc.Format = SkateboardBufferFormatToD3D(desc->Format);
					SRVdesc.Shader4ComponentMapping = D3D12_ENCODE_SHADER_4_COMPONENT_MAPPING(D3D12_SHADER_COMPONENT_MAPPING_FROM_MEMORY_COMPONENT_0, D3D12_SHADER_COMPONENT_MAPPING_FROM_MEMORY_COMPONENT_1, D3D12_SHADER_COMPONENT_MAPPING_FROM_MEMORY_COMPONENT_2, D3D12_SHADER_COMPONENT_MAPPING_FROM_MEMORY_COMPONENT_3);

					switch (desc->Dimension)
					{
					case TextureDimension_Texture1D:
						SRVdesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE1D;
						SRVdesc.Texture1D.MipLevels = desc->MipLevels;
						SRVdesc.Texture1D.MostDetailedMip = desc->MostDetailedMip;
						SRVdesc.Texture1D.ResourceMinLODClamp = desc->ResourceMinLodClamp;
						break;
					case TextureDimension_Texture1D_ARRAY:
						SRVdesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE1DARRAY;
						SRVdesc.Texture1DArray.ArraySize = desc->ArraySize;
						SRVdesc.Texture1DArray.FirstArraySlice = desc->FirstArraySlice;
						SRVdesc.Texture1DArray.MipLevels = desc->MipLevels;
						SRVdesc.Texture1DArray.MostDetailedMip = desc->MostDetailedMip;
						SRVdesc.Texture1DArray.ResourceMinLODClamp = desc->ResourceMinLodClamp;
						break;
					case TextureDimension_Texture2D:
						SRVdesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
						SRVdesc.Texture2D.MipLevels = desc->MipLevels;
						SRVdesc.Texture2D.PlaneSlice = desc->PlaneSlice;
						SRVdesc.Texture2D.MostDetailedMip = desc->MostDetailedMip;
						SRVdesc.Texture2D.ResourceMinLODClamp = desc->ResourceMinLodClamp;
						break;
					case TextureDimension_Texture2D_ARRAY:
						SRVdesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
						SRVdesc.Texture2DArray.ArraySize = desc->ArraySize;
						SRVdesc.Texture2DArray.FirstArraySlice = desc->FirstArraySlice;
						SRVdesc.Texture2DArray.MipLevels = desc->MipLevels;
						SRVdesc.Texture2DArray.PlaneSlice = desc->PlaneSlice;
						SRVdesc.Texture2DArray.MostDetailedMip = desc->MostDetailedMip;
						SRVdesc.Texture2DArray.ResourceMinLODClamp = desc->ResourceMinLodClamp;

						break;
					case TextureDimension_Texture2D_MS:
						SRVdesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DMS;
						break;
					case TextureDimension_Texture2D_MS_ARRAY:
						SRVdesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DMSARRAY;
						SRVdesc.Texture2DArray.ArraySize = desc->ArraySize;
						SRVdesc.Texture2DArray.FirstArraySlice = desc->FirstArraySlice;
						SRVdesc.Texture2DArray.MipLevels = desc->MipLevels;
						SRVdesc.Texture2DArray.MostDetailedMip = desc->MostDetailedMip;
						SRVdesc.Texture2DArray.PlaneSlice = desc->PlaneSlice;
						SRVdesc.Texture2DArray.ResourceMinLODClamp = desc->ResourceMinLodClamp;
						break;
					case TextureDimension_Texture3D:
						SRVdesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DMSARRAY;
						SRVdesc.Texture3D.MipLevels = desc->MipLevels;
						SRVdesc.Texture3D.MostDetailedMip = desc->MostDetailedMip;
						SRVdesc.Texture3D.ResourceMinLODClamp = desc->ResourceMinLodClamp;
						break;
					case TextureDimension_CubeMap:
						SRVdesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
						SRVdesc.TextureCube.MipLevels = desc->MipLevels;
						SRVdesc.TextureCube.MostDetailedMip = desc->MostDetailedMip;
						SRVdesc.TextureCube.ResourceMinLODClamp = desc->ResourceMinLodClamp;
						break;
					case TextureDimension_CubeMap_ARRAY:
						SRVdesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBEARRAY;
						SRVdesc.TextureCubeArray.First2DArrayFace = desc->FirstArraySlice;
						SRVdesc.TextureCubeArray.MipLevels = desc->MipLevels;
						SRVdesc.TextureCubeArray.MostDetailedMip = desc->MostDetailedMip;
						SRVdesc.TextureCubeArray.NumCubes = desc->ArraySize;
						SRVdesc.TextureCubeArray.ResourceMinLODClamp = desc->ResourceMinLodClamp;
						break;
					default:
						SRVdesc.ViewDimension = D3D12_SRV_DIMENSION_UNKNOWN;
						break;
					}

					return [SRVdesc, this, Resource](void** m_memoryHandle)
						{
								auto handle = m_SRVDescriptorHeap.Allocate();
								m_Device->CreateShaderResourceView((Resource) ? reinterpret_cast<D3D12MA::Allocation*>(Resource)->GetResource() : nullptr, &SRVdesc, handle.GetCPUHandle());
								*m_memoryHandle = new D3DDescriptorHandle(std::move(handle));
								return OKEYDOKEY;
						};
					}
					else
					{
						return [desc, this, Resource](void** m_memoryHandle)
							{
								auto handle = m_SRVDescriptorHeap.Allocate();
								m_Device->CreateShaderResourceView((Resource) ? reinterpret_cast<D3D12MA::Allocation*>(Resource)->GetResource() : nullptr, nullptr, handle.GetCPUHandle());
								*m_memoryHandle = new D3DDescriptorHandle(std::move(handle));
								return OKEYDOKEY;
							};
					}
					break;

			case ViewAccessType_GpuReadWrite:
				if (desc)
				{
					UAVdesc.Format = SkateboardBufferFormatToD3D(desc->Format);
					switch (desc->Dimension)
					{
					case TextureDimension_Texture1D:
						UAVdesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE1D;
						UAVdesc.Texture1D.MipSlice = desc->MipSlice;
						break;
					case TextureDimension_Texture1D_ARRAY:
						UAVdesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE1DARRAY;
						UAVdesc.Texture1DArray.ArraySize = desc->ArraySize;
						UAVdesc.Texture1DArray.FirstArraySlice = desc->FirstArraySlice;
						UAVdesc.Texture1DArray.MipSlice = desc->MipSlice;
						break;
					case TextureDimension_Texture2D:
						UAVdesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
						UAVdesc.Texture2D.MipSlice = desc->MipLevels;
						UAVdesc.Texture2D.PlaneSlice = desc->PlaneSlice;
						break;
					case TextureDimension_Texture2D_ARRAY:
						UAVdesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
						UAVdesc.Texture2DArray.ArraySize = desc->ArraySize;
						UAVdesc.Texture2DArray.FirstArraySlice = desc->FirstArraySlice;
						UAVdesc.Texture2DArray.MipSlice = desc->MipSlice;
						UAVdesc.Texture2DArray.PlaneSlice = desc->PlaneSlice;
						break;
					case TextureDimension_Texture2D_MS:
						UAVdesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2DMS;
						break;
					case TextureDimension_Texture2D_MS_ARRAY:
						UAVdesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2DMSARRAY;
						UAVdesc.Texture2DArray.ArraySize = desc->ArraySize;
						UAVdesc.Texture2DArray.FirstArraySlice = desc->FirstArraySlice;
						UAVdesc.Texture2DArray.MipSlice = desc->MipSlice;
						UAVdesc.Texture2DArray.PlaneSlice = desc->PlaneSlice;
						break;
					case TextureDimension_Texture3D:
						UAVdesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2DMSARRAY;
						UAVdesc.Texture3D.FirstWSlice = desc->FirstArraySlice;
						UAVdesc.Texture3D.MipSlice = desc->MipSlice;
						UAVdesc.Texture3D.WSize = desc->ArraySize;
						break;
					case TextureDimension_CubeMap:
						SKTBD_ASSERT_M(false, "Creating a Write TextureCube is illegal")
							break;
					case TextureDimension_CubeMap_ARRAY:
						SKTBD_ASSERT_M(false, "Creating a Write TextureCubeArray is illegal")
							break;
					default:
						UAVdesc.ViewDimension = D3D12_UAV_DIMENSION_UNKNOWN;
						break;
					};

					return [UAVdesc, this, Resource](void** m_memoryHandle)
						{
							auto handle = m_SRVDescriptorHeap.Allocate();
							m_Device->CreateUnorderedAccessView((Resource) ? reinterpret_cast<D3D12MA::Allocation*>(Resource)->GetResource() : nullptr, nullptr, &UAVdesc, handle.GetCPUHandle());
							*m_memoryHandle = new D3DDescriptorHandle(std::move(handle));

						};
				}
				else
				{
					return [desc, this, Resource](void** m_memoryHandle)
						{
							auto handle = m_SRVDescriptorHeap.Allocate();
							m_Device->CreateUnorderedAccessView((Resource) ? reinterpret_cast<D3D12MA::Allocation*>(Resource)->GetResource() : nullptr, nullptr, nullptr, handle.GetCPUHandle());
							*m_memoryHandle = new D3DDescriptorHandle(std::move(handle));
						};
				};
		};

		
	}

	std::function<void(void**)> D3DGraphicsContext::CreateBufferView(void* Resource, ViewAccessType_ ViewAccessType, const BufferViewDesc& desc)
	{
		
		D3D12_UNORDERED_ACCESS_VIEW_DESC UAVdesc{};
		D3D12_SHADER_RESOURCE_VIEW_DESC SRVdesc{};

		switch(ViewAccessType)
		{
		case ViewAccessType_ConstantBuffer:


			return[this, desc, Resource](void** m_memoryHandle)
			{
				D3D12_CONSTANT_BUFFER_VIEW_DESC CBVdesc{};
				CBVdesc.BufferLocation = static_cast<D3D12MA::Allocation*>(Resource)->GetResource()->GetGPUVirtualAddress() + desc.Offset;
				CBVdesc.SizeInBytes = desc.ElementSize;

				auto handle = m_SRVDescriptorHeap.Allocate();
				m_Device->CreateConstantBufferView(&CBVdesc, handle.GetCPUHandle());
				*m_memoryHandle = new D3DDescriptorHandle(std::move(handle));
			};
			
			break;
		case ViewAccessType_::ViewAccessType_GpuRead:
			SRVdesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
			SRVdesc.Buffer.NumElements = desc.ElementCount;
			SRVdesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

			switch (desc.Type)
			{
			case BufferType_ByteBuffer:
				SRVdesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_RAW;
				SRVdesc.Buffer.FirstElement = desc.Offset;
				SRVdesc.Buffer.StructureByteStride = 1;
				SRVdesc.Format = DXGI_FORMAT_UNKNOWN;
				break;
			case BufferType_StructureBuffer:
				SRVdesc.Buffer.FirstElement = desc.Offset;
				SRVdesc.Buffer.StructureByteStride = desc.ElementSize;
				SRVdesc.Format = DXGI_FORMAT_UNKNOWN;
				break;
			case BufferType_FormattedBuffer:
				SRVdesc.Buffer.FirstElement = desc.Offset;
				SRVdesc.Buffer.StructureByteStride = 0;
				SRVdesc.Format = SkateboardBufferFormatToD3D(desc.Format);
				break;
			default:
				break;
			}

			return[SRVdesc, this, Resource](void** m_memoryHandle)
			{
				auto handle = m_SRVDescriptorHeap.Allocate();
				m_Device->CreateShaderResourceView((Resource) ? static_cast<D3D12MA::Allocation*>(Resource)->GetResource() : nullptr, &SRVdesc, handle.GetCPUHandle());
				*m_memoryHandle = new D3DDescriptorHandle(std::move(handle));
			};

			break;

		case ViewAccessType_::ViewAccessType_GpuReadWrite:
			switch (desc.Type)
			{
			case BufferType_ByteBuffer:
				UAVdesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_RAW;
				UAVdesc.Buffer.FirstElement = desc.Offset;
				UAVdesc.Buffer.StructureByteStride = 1;
				UAVdesc.Format = DXGI_FORMAT_UNKNOWN;
				break;

			case BufferType_StructureBuffer:
				UAVdesc.Buffer.FirstElement = desc.Offset;
				UAVdesc.Buffer.StructureByteStride = desc.ElementSize;
				UAVdesc.Format = DXGI_FORMAT_UNKNOWN;
				break;

			case BufferType_FormattedBuffer:
				UAVdesc.Buffer.FirstElement = desc.Offset;
				UAVdesc.Buffer.StructureByteStride = 0;
				UAVdesc.Format = SkateboardBufferFormatToD3D(desc.Format);
				break;

			case BufferType_ConstantBuffer:
				SKTBD_ASSERT_M(false, "Cant Create a Writable Constant Buffer, use Structured Buffer View to Inject a value into CBV")
			};

			UAVdesc.Buffer.CounterOffsetInBytes = 0;

			return [UAVdesc, this, Resource](void** m_memoryHandle)
			{
				auto handle = m_SRVDescriptorHeap.Allocate();
				m_Device->CreateUnorderedAccessView((Resource) ? reinterpret_cast<D3D12MA::Allocation*>(Resource)->GetResource() : nullptr, nullptr, &UAVdesc, handle.GetCPUHandle());
				*m_memoryHandle = new D3DDescriptorHandle(handle);
			};
			break;
		}

	}

	std::function<void(void**)> D3DGraphicsContext::CreateCommandBuffer(const CommandBufferType_& Type, const CommandBufferPriority_& Priority)
	{
		return [this, Type, Priority](void** m_memHandle)
		{
				auto handle = new CommandBufferMemHandle;

				D3D_CHECK_FAILURE(m_Device->CreateCommandAllocator((Type == CommandBufferType_Graphics) ? D3D12_COMMAND_LIST_TYPE_DIRECT : D3D12_COMMAND_LIST_TYPE_COMPUTE, IID_PPV_ARGS(handle->m_Allocator.ReleaseAndGetAddressOf())));

				// Create the command list
				D3D_CHECK_FAILURE(m_Device->CreateCommandList1(
					0,											// For single GPU operation, set this to zero. Otherwise use this to identify the adapter
					(Type == CommandBufferType_Graphics) ? D3D12_COMMAND_LIST_TYPE_DIRECT : D3D12_COMMAND_LIST_TYPE_COMPUTE,
					D3D12_COMMAND_LIST_FLAG_NONE,							
					IID_PPV_ARGS(handle->m_Commandlist.ReleaseAndGetAddressOf())
				));

				*m_memHandle = handle;
		};
	}

	SKTBDR D3DGraphicsContext::CreatePipelineInputLayout(const std::wstring& debugname, ShaderInputLayout& layout, const ShaderInputsLayoutDesc& Desc)
	{
		ID3D12RootSignature* RootSig;

		std::vector<D3D12_ROOT_PARAMETER1> vRootParams(Desc.vPipelineInputs.size());
		std::vector<D3D12_STATIC_SAMPLER_DESC1> vStaticSamplers(Desc.vStaticSamplers.size());

		//RootParams
		std::transform(Desc.vPipelineInputs.begin(), Desc.vPipelineInputs.end(), vRootParams.begin(), [](const ShaderResourceDesc& desc) -> D3D12_ROOT_PARAMETER1
			{
				D3D12_ROOT_PARAMETER1 param = {};

				param.ShaderVisibility = ShaderVisibilityToD3D(desc.ShaderVisibility);

				switch (desc.ShaderElementType)
				{
					case ShaderElementType_RootConstant				:
						param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
						param.Constants.Num32BitValues = desc.Constant.Num32BitValues;
						param.Constants.RegisterSpace = desc.Constant.RegisterSpace;
						param.Constants.ShaderRegister = desc.Constant.ShaderRegister;
						break;

					case ShaderElementType_ConstantBufferView		:
						param.ParameterType = D3D12_ROOT_PARAMETER_TYPE::D3D12_ROOT_PARAMETER_TYPE_CBV;
						//		param.Descriptor.Flags =
						param.Descriptor.RegisterSpace = desc.Descriptor.RegisterSpace;
						param.Descriptor.ShaderRegister = desc.Descriptor.ShaderRegister;
						break;

					case ShaderElementType_ReadResourceView			:
						param.ParameterType = D3D12_ROOT_PARAMETER_TYPE::D3D12_ROOT_PARAMETER_TYPE_SRV;
						//		param.Descriptor.Flags =
						param.Descriptor.RegisterSpace = desc.Descriptor.RegisterSpace;
						param.Descriptor.ShaderRegister = desc.Descriptor.ShaderRegister;
						break;

					case ShaderElementType_ReadWriteResourceView	:
						param.ParameterType = D3D12_ROOT_PARAMETER_TYPE::D3D12_ROOT_PARAMETER_TYPE_UAV;
						//		param.Descriptor.Flags =
						param.Descriptor.RegisterSpace = desc.Descriptor.RegisterSpace;
						param.Descriptor.ShaderRegister = desc.Descriptor.ShaderRegister;
						break;

				//	case ShaderElementType_DescriptorTable			:
				//		param.ParameterType = D3D12_ROOT_PARAMETER_TYPE::D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
				//		param.DescriptorTable.NumDescriptorRanges = desc.DescriptorTable.NumberOfRanges;
				//
				//		std::vector<D3D12_DESCRIPTOR_RANGE1> Ranges(desc.DescriptorTable.NumberOfRanges); oh fuck thats gonna go out of scope
				//		std::transform(desc.DescriptorTable.vRanges.begin(), desc.DescriptorTable.vRanges.end(), Ranges.begin(), [](const DescriptorRange& range) -> D3D12_DESCRIPTOR_RANGE
				//			{
				//				D3D12_DESCRIPTOR_RANGE Range;
				//			});
				//
				//		param.DescriptorTable.pDescriptorRanges = Ranges.data();
				//		break;
				default:
					break;
				}
				
				return param;
		});

		//StaticSamplers
		std::transform(Desc.vStaticSamplers.begin(), Desc.vStaticSamplers.end(), vStaticSamplers.begin(), [](const SamplerSlotDesc& slot) -> D3D12_STATIC_SAMPLER_DESC1
			{
				D3D12_STATIC_SAMPLER_DESC1 SD{};

				SD.ShaderRegister = slot.ShaderRegister;
				SD.RegisterSpace = slot.ShaderRegisterSpace;
				SD.Filter = SamplerFilterToD3D(slot.SamplerDesc.Filter);
				SD.AddressU = SamplerModeToD3D(slot.SamplerDesc.ModeU);
				SD.AddressV = SamplerModeToD3D(slot.SamplerDesc.ModeV);
				SD.AddressW = SamplerModeToD3D(slot.SamplerDesc.ModeW);
				SD.MipLODBias = slot.SamplerDesc.MipMapLevelOffset;
				SD.MaxAnisotropy = slot.SamplerDesc.MaxAnisotropy;
				SD.ComparisonFunc = SamplerComparisonFunctionToD3D(slot.SamplerDesc.ComparisonFunction);
				SD.BorderColor = SamplerBorderColourToD3D(slot.SamplerDesc.BorderColour);
				SD.MinLOD = slot.SamplerDesc.MipMapMinSampleLevel;
				SD.MaxLOD = slot.SamplerDesc.MipMapMaxSampleLevel;
				SD.ShaderVisibility = ShaderVisibilityToD3D(slot.ShaderVisibility);
				SD.Flags = (D3D12_SAMPLER_FLAGS)slot.SamplerDesc.Flags;

				return SD;
			}
		);

		D3D12_ROOT_SIGNATURE_FLAGS flags = D3D12_ROOT_SIGNATURE_FLAG_NONE;
		flags |= (Desc.DescriptorsDirctlyAddresssed) ? D3D12_ROOT_SIGNATURE_FLAG_CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED : D3D12_ROOT_SIGNATURE_FLAG_NONE;
		flags |= (Desc.CanUseInputAssembler) ? D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT : D3D12_ROOT_SIGNATURE_FLAG_NONE;
		flags |= (Desc.SamplersDirectlyAddressed) ? D3D12_ROOT_SIGNATURE_FLAG_SAMPLER_HEAP_DIRECTLY_INDEXED : D3D12_ROOT_SIGNATURE_FLAG_NONE;

		CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC RootSigDesc;
		RootSigDesc.Init_1_2(RootSigDesc, vRootParams.size(), vRootParams.data(), vStaticSamplers.size(), vStaticSamplers.data(),flags);

		ComPtr<ID3DBlob> signature;
		ComPtr<ID3DBlob> error;

		auto err = D3DX12SerializeVersionedRootSignature(&RootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1_2, &signature, &error);

		D3D_CHECK_FAILURE(err);

		D3D_CHECK_FAILURE(m_Device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&RootSig)));

#ifndef SKTBD_SHIP
		RootSig->SetName(debugname.c_str());
#endif // SKTBD_DEBUG

		*static_cast<void**>(layout.GetResourcePtr()) = RootSig;
		return OKEYDOKEY;
	}

	SKTBDR D3DGraphicsContext::CreatePipelineState(const std::wstring& debugname, Pipeline& pipeline, const PipelineDesc& Desc)
	{
		//helper lambda
		auto ValidatePathNLoad = [](const wchar_t* Filename, IDxcBlobEncoding** Blob) -> bool
			{
			  if (!Filename) return false;
			  std::filesystem::path file(Filename);
			  file.replace_extension(L".cso"); 

			  auto abs_path = (std::filesystem::current_path() += L"\\") += file;

			  if (std::filesystem::exists(abs_path))
			  {
				  gD3DContext->Utils()->LoadFile(file.c_str(), nullptr, Blob);
				  return true;
			  }
			  return false;
			};

		pipeline = Pipeline(debugname, Desc.Type);

		switch (Desc.Type)
		{
		case PipelineType_Graphics:
			{
				D3D12_GRAPHICS_PIPELINE_STATE_DESC PipelineDesc = {};

				// Create the input layout based on the given description layout
				RasterizationPipelineDesc desc = *std::get<RasterizationPipelineDesc*>(Desc.TypeDesc);

				const BufferLayout& layout = desc.InputVertexLayout;
				std::vector<D3D12_INPUT_ELEMENT_DESC> inputElementDescs;
				for (const BufferElement& element : layout)
				{
					const DXGI_FORMAT format = ShaderDataTypeToD3D(element.Type);
					inputElementDescs.push_back({ element.SemanticName.c_str(), 0, format, element.InputSlot , element.Offset, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 });
				}

				auto& RSconfig = desc.Rasterizer;

				// Describe the rasterizer state (how will objects be rasterized ?)
				D3D12_RASTERIZER_DESC rasterizerDesc = {};
				rasterizerDesc.FillMode = RSconfig.Wireframe ? D3D12_FILL_MODE_WIREFRAME : D3D12_FILL_MODE_SOLID;		// Specify which fill mode to use when rendering
				rasterizerDesc.CullMode = (D3D12_CULL_MODE)RSconfig.Cull;										// Specify which face to cull (none, front, back)
				rasterizerDesc.FrontCounterClockwise = RSconfig.FrontCC;											// Determines if a triangle is front or back facing
				rasterizerDesc.DepthBias = RSconfig.DepthBias;													// Depth value added to a given pixel
				rasterizerDesc.DepthBiasClamp = RSconfig.DepthBiasClamp;											// Maximum depth bias for a given pixel
				rasterizerDesc.SlopeScaledDepthBias = RSconfig.SlopeScaledDepthBias;								// Scalar on a given pixel's slope. More info on Depth Bias on MSDN
				rasterizerDesc.DepthClipEnable = RSconfig.DepthClipEnable;										// Specifies whether or not to enable clipping based on distance
				rasterizerDesc.MultisampleEnable = RSconfig.MultisampleEnable;									// Set to TRUE to use the quadrilateral line anti-aliasing algorithm and to FALSE to use the alpha line anti-aliasing algorithm
				rasterizerDesc.AntialiasedLineEnable = RSconfig.AntialiasedLineEnable;							// Specifies whether to enable line antialiasing
				rasterizerDesc.ForcedSampleCount = RSconfig.ForcedSampleCount;									// The sample count that is forced while UAV rendering or rasterizing. 0 is not forced
				rasterizerDesc.ConservativeRaster = (RSconfig.ConservativeRasterEnable) ? D3D12_CONSERVATIVE_RASTERIZATION_MODE_ON : D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;									// Identifies whether conservative rasterization is on or off

				auto& Blendconfig = desc.Blend;

				// Describe a blend state
				D3D12_BLEND_DESC blendDesc = {};
				blendDesc.AlphaToCoverageEnable = Blendconfig.AlphaToCoverage;											// Specifies whether to use alpha-to-coverage as a multisampling technique when setting a pixel to a render target
				blendDesc.IndependentBlendEnable = Blendconfig.IndependentBlendEnable;									// Specifies whether to enable independent blending in simultaneous render targets (FALSE only uses RenderTarget[0])#

				auto& BlendRTConfig = Blendconfig.RTBlendConfigs;

				std::transform(BlendRTConfig.cbegin(), BlendRTConfig.cend(), blendDesc.RenderTarget, [](const RT_BlendConfig& config)->D3D12_RENDER_TARGET_BLEND_DESC
					{
						D3D12_RENDER_TARGET_BLEND_DESC desc;
						desc.BlendEnable = config.BlendEnable;
						desc.BlendOp = (D3D12_BLEND_OP)config.BlendOp;
						desc.BlendOpAlpha = (D3D12_BLEND_OP)config.BlendOpAlpha;
						desc.DestBlend = (D3D12_BLEND)config.DestBlend;
						desc.DestBlendAlpha = (D3D12_BLEND)config.DestBlendAlpha;
						desc.LogicOp = (D3D12_LOGIC_OP)config.LogicOp;
						desc.LogicOpEnable = config.LogicOpEnable;
						desc.RenderTargetWriteMask = config.RenderTargetWriteMask;
						desc.SrcBlend = (D3D12_BLEND)config.SrcBlend;
						desc.SrcBlendAlpha = (D3D12_BLEND)config.SrcBlendAlpha;
						return desc;
					});

				auto& DSconfig = desc.DepthStencil;

				D3D12_DEPTH_STENCIL_DESC dsDesc = {};
				dsDesc.DepthEnable = DSconfig.DepthEnable;									// Specify whether to enable depth testing
				dsDesc.DepthWriteMask = DSconfig.DepthWriteAll ? D3D12_DEPTH_WRITE_MASK_ALL : D3D12_DEPTH_WRITE_MASK_ZERO;		// Enable or disable writing to sections or all of the depth testing buffer
				dsDesc.DepthFunc = (D3D12_COMPARISON_FUNC)DSconfig.DepthFunc;				// Function to compare new depth data to existing depth data
				dsDesc.StencilEnable = DSconfig.StencilEnable;								// Spicify whether to enable stencil testing
				dsDesc.StencilReadMask = DSconfig.StencilReadMask;							// Identify a portion of the depth-stencil buffer for reading stencil data
				dsDesc.StencilWriteMask = DSconfig.StencilWriteMask;						// Identify a portion of the depth-stencil buffer for writing stencil data

				dsDesc.FrontFace.StencilDepthFailOp = (D3D12_STENCIL_OP)DSconfig.FrontFace.StencilDepthFailOp;
				dsDesc.FrontFace.StencilFailOp = (D3D12_STENCIL_OP)DSconfig.FrontFace.StencilFailOp;
				dsDesc.FrontFace.StencilFunc = (D3D12_COMPARISON_FUNC)DSconfig.FrontFace.StencilFunc;
				dsDesc.FrontFace.StencilPassOp = (D3D12_STENCIL_OP)DSconfig.FrontFace.StencilPassOp;

				dsDesc.BackFace.StencilDepthFailOp = (D3D12_STENCIL_OP)DSconfig.BackFace.StencilDepthFailOp;
				dsDesc.BackFace.StencilFailOp = (D3D12_STENCIL_OP)DSconfig.BackFace.StencilFailOp;
				dsDesc.BackFace.StencilFunc = (D3D12_COMPARISON_FUNC)DSconfig.BackFace.StencilFunc;
				dsDesc.BackFace.StencilPassOp = (D3D12_STENCIL_OP)DSconfig.BackFace.StencilPassOp;

				// Describe and create the graphics pipeline state object (PSO).
				D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
				psoDesc.InputLayout = { inputElementDescs.data(), (uint32_t)inputElementDescs.size() };
				psoDesc.pRootSignature = static_cast<ID3D12RootSignature*>(Desc.GlobalLayoutSignature.GetResource());

				std::array<IDxcBlobEncoding*, 5> Blobs = {}; //store blobs as a temp array as we need the for lifetime

				if (ValidatePathNLoad(desc.VertexShader.FileName	, &Blobs[0])) {	psoDesc.VS = { Blobs[0]->GetBufferPointer(), Blobs[0]->GetBufferSize() }; }
				if (ValidatePathNLoad(desc.HullShader.FileName		, &Blobs[1])) { psoDesc.HS = { Blobs[1]->GetBufferPointer(), Blobs[1]->GetBufferSize() }; }
				if (ValidatePathNLoad(desc.DomainShader.FileName	, &Blobs[2])) {	psoDesc.DS = { Blobs[2]->GetBufferPointer(), Blobs[2]->GetBufferSize() }; }
				if (ValidatePathNLoad(desc.GeometryShader.FileName	, &Blobs[3])){	psoDesc.GS = { Blobs[3]->GetBufferPointer(), Blobs[3]->GetBufferSize() }; }
				if (ValidatePathNLoad(desc.PixelShader.FileName		, &Blobs[4])){	psoDesc.PS = { Blobs[4]->GetBufferPointer(), Blobs[4]->GetBufferSize() }; }

				psoDesc.RasterizerState = rasterizerDesc;																	// Specify how objects should be rasterized
				psoDesc.BlendState = blendDesc;																				// Specify how pixels should be blended
				psoDesc.DepthStencilState = dsDesc;																			// Specify how the depth/stencil tests should be performed
				psoDesc.SampleMask = UINT_MAX;																				// The sample mask for the blend state
				psoDesc.PrimitiveTopologyType = (D3D12_PRIMITIVE_TOPOLOGY_TYPE)desc.InputPrimitiveType;				// Specify the primitive typology of the data (a lot of define in skateboard are just relabled dx12 defines)
				psoDesc.NumRenderTargets = desc.RenderTargetCount;													// The number of render targets
				std::transform(desc.RenderTargetDataFormats.cbegin(),desc.RenderTargetDataFormats.cend(), &psoDesc.RTVFormats[0], SkateboardBufferFormatToD3D);	// Specify the render target formats
				psoDesc.SampleDesc.Count = desc.SampleCount;															// MSAA SampleCount
				psoDesc.SampleDesc.Quality = desc.SampleQuality;														// MSAA SampleQuality
				psoDesc.DSVFormat = SkateboardBufferFormatToD3D(desc.DepthstencilTargetFormat);					    // Specify the format of the depth/stencil buffer
				psoDesc.IBStripCutValue = (D3D12_INDEX_BUFFER_STRIP_CUT_VALUE)desc.TriangleStripCutValue;
				psoDesc.NodeMask = 0u;

				ID3D12PipelineState* PSO;
				// Create the pipeline state objects
				D3D_CHECK_FAILURE(gD3DContext->Device()->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&PSO)));
				*static_cast<void**>(pipeline.GetResourcePtr()) = PSO;

				//Clean up the unneeded shader binaries;
				for (auto i : Blobs) { if (i) i->Release(); };

	#ifndef SKTBD_SHIP
				PSO->SetName((debugname + L" - Graphics PSO").c_str());
	#endif // !SKTBD_SHIP
				
			}
			break;
		case PipelineType_Compute:
				D3D12_COMPUTE_PIPELINE_STATE_DESC psoDesc{};

				psoDesc.pRootSignature = static_cast<ID3D12RootSignature*>(Desc.GlobalLayoutSignature.GetResource());
				psoDesc.NodeMask = 0u;

				ComputePipelineDesc desc = *std::get<ComputePipelineDesc*>(Desc.TypeDesc);

				IDxcBlobEncoding* Blob;

				if (ValidatePathNLoad(desc.ComputeShader.FileName, &Blob))
				{ psoDesc.CS = { Blob->GetBufferPointer(), Blob->GetBufferSize() }; };

				ID3D12PipelineState* PSO;
				// Create the pipeline state objects
				D3D_CHECK_FAILURE(gD3DContext->Device()->CreateComputePipelineState(&psoDesc, IID_PPV_ARGS(&PSO)));
				*static_cast<void**>(pipeline.GetResourcePtr()) = PSO;

				if (Blob) Blob->Release();

	#ifndef SKTBD_SHIP
				PSO->SetName((debugname + L" - Compute PSO").c_str());
	#endif // !SKTBD_SHIP

				break;

			//case PipelineType_Ray	  :

			//	break;

			//case PipelineType_Mesh    :

			//	break;
		}

		return OKEYDOKEY;
	}

	//std::function<void(void**)> D3DGraphicsContext::GetMemoryDeInitializer(GPUResourceType_ Type)
	//{
	//	switch (Type)
	//	{
	//	case Skateboard::GPUResourceType_Buffer:
	//	case Skateboard::GpuResourceType_Texture:
	//		return[this](void** m_handle)
	//		{
	//			reinterpret_cast<D3D12MA::Allocation*>(*m_handle)->Release();
	//		};

	//		break;
	//	case Skateboard::GpuResourceType_View:
	//		return[this](void** m_handle)
	//			{
	//				//	reinterpret_cast<D3DDescriptorHandle*>(*m_handle)
	//			};
	//		break;
	//	//case Skateboard::GpuResourceType_Fence:
	//		break;
	//	//case Skateboard::GpuResourceType_Semaphore:
	//		break;
	//	default:
	//		break;
	//	}

	//	return[](void** ptr) {delete* ptr; };
	//}


	//}

	bool D3DGraphicsContext::CheckDeviceRemovedStatus() const
	{
		const HRESULT result = m_Device->GetDeviceRemovedReason();
		if (result != S_OK)
		{
			SKTBD_CORE_ERROR(L"Device removed reason: {}", DXException(result).ToString().c_str());
			return true;
		}
		return false;
	}

	D3D12_CPU_DESCRIPTOR_HANDLE D3DGraphicsContext::CurrentBackBufferView() const
	{
		// Offset to the RTV of the current back buffer and return the location of the according descriptor
		//D3D12_CPU_DESCRIPTOR_HANDLE handle = {};
		//handle.ptr = m_RTVHeap->GetCPUDescriptorHandleForHeapStart().ptr + m_CurrentBackBuffer * m_RTVDescriptorSize;		// Note: Overflow higly improbable as m_CurrentBackBuffer usually ranges in [0,3]
		//return handle;
		return static_cast<D3DDescriptorHandle*>(m_SwapChainRTVs[m_CurrentBackBuffer])->GetCPUHandle();
	}

	D3D12_CPU_DESCRIPTOR_HANDLE D3DGraphicsContext::DepthStencilView() const
	{
		// There is only one descriptor for this heap, just return where it starts
		//return m_DSVHeap->GetCPUDescriptorHandleForHeapStart();
		return static_cast<D3DDescriptorHandle*>(m_DefaultDSV.GetResource())->GetCPUHandle();
	}

}