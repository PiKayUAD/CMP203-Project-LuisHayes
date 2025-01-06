#include <sktbdpch.h>
#include "AGCGraphicsContext.h"
#include "Skateboard/Renderer/InternalFormats.h"

#define SKTBD_LOG_COMPONENT "GRAPHICS CONTEXT IMPL"
#include "Skateboard/Log.h"

namespace Skateboard
{
	AGCGraphicsContext* gAGCContext = nullptr;
	GraphicsContext* GraphicsContext::Context = nullptr;

	AGCGraphicsContext::AGCGraphicsContext(const PlatformProperties& props) :
		GraphicsContext(props.BackBufferWidth, props.BackBufferHeight),
		m_VideoHandle(0),
		m_VideoSetIndex(0),
		m_LatencyControl{}
	{
		// Assign the singleton
		SKTBD_CORE_ASSERT(!Context, "GraphicsContext already exists! Only one context can be created.");
		Context = this;
		gAGCContext = this;

		InitialiseAGC();
		CreateDrawCommandAndStateBuffers();
		CreateFilpLabels();
		CreateRenderTargets();
		CreateDepthTarget();
		InitialiseVideoOut();
		CreateViewPort();
	}

	AGCGraphicsContext::~AGCGraphicsContext()
	{
		// Release the video
		sceVideoOutUnregisterBuffers(m_VideoHandle, m_VideoSetIndex);

		// Release memory
		m_MemoryAllocator.TypedAlignedFree(m_ResourceRegistrationMemory);
		m_MemoryAllocator.TypedAlignedFree(m_DepthStencilTargetMemory);
		m_MemoryAllocator.TypedAlignedFree(m_DepthStencilHTileMemory);
		m_MemoryAllocator.TypedAlignedFree(m_FlipLabelsMemory);
		for (uint32_t i = 0u; i < GRAPHICS_SETTINGS_NUMFRAMERESOURCES; ++i)
		{
			m_MemoryAllocator.TypedAlignedFree(m_DrawCommandBuffersMemory[i]);
			m_MemoryAllocator.TypedAlignedFree(m_RenderTargetsMemory[i]);
		}
	}

	void AGCGraphicsContext::InitialiseAGC()
	{
		SceError error = sce::Agc::init();
		SCE_AGC_ASSERT(error == SCE_OK);

		size_t resourceRegistrationBufferSize;
		error = sce::Agc::ResourceRegistration::queryMemoryRequirements(&resourceRegistrationBufferSize, 1024*64, 64);
		if (error != SCE_AGC_ERROR_RESOURCE_REGISTRATION_NO_PA_DEBUG)
		{
			SCE_AGC_ASSERT(error == SCE_OK);
			m_ResourceRegistrationMemory = m_MemoryAllocator.TypedAlignedAllocate<uint8_t,sce::Agc::Alignment::kResourceRegistration>(resourceRegistrationBufferSize);
			error = sce::Agc::ResourceRegistration::init(m_ResourceRegistrationMemory.data, resourceRegistrationBufferSize, 64);
			SCE_AGC_ASSERT(error == SCE_OK);
			error = sce::Agc::ResourceRegistration::registerDefaultOwner(nullptr);
			SCE_AGC_ASSERT(error == SCE_OK);
		}

		error = sce::Agc::Toolkit::init();
		SCE_AGC_ASSERT(error == SCE_OK);
	}

	bool DCBOutOfMemoryCallback(sce::Agc::TwoSidedAllocator* alloc, uint32_t sizeInDwords, void* userData)
	{
		// This is the callback which is invoked when the current operation leaves less than m_reservedSpaceInDwords 
		// of free memory.

		// This function will return true if the requested space is available on alloc() otherwise false.



	}

	void DCBOutOfMemoryUserData()
	{



	}

	void AGCGraphicsContext::CreateDrawCommandAndStateBuffers()
	{
		// Create a draw command buffer and state buffer for each frame resource. Note that we will not consider
		// resizing the dcb when running out of memory.
		const uint32_t dcb_size = 2 * 1024 * 1024;	// ~8 MB per command list should be enough, (allocating uint 32s -> size 4)
		for (uint32_t i = 0; i < GRAPHICS_SETTINGS_NUMFRAMERESOURCES; ++i)
		{
			m_DrawCommandBuffersMemory[i] = m_MemoryAllocator.TypedAlignedAllocate<CommandBufferMem,sce::Agc::Alignment::kCommandBuffer>(dcb_size);
			m_DrawCommandBuffers[i].init(
				m_DrawCommandBuffersMemory[i].data,
				dcb_size,
				nullptr,
				nullptr);

			m_StateBuffers[i].init(
				256, // This is the size of a chunk in the StateBuffer, defining the largest size of a load packet's payload.
				&m_DrawCommandBuffers[i],
				&m_DrawCommandBuffers[i]);

#ifndef SKTBD_SHIP
			sce::Agc::Core::registerResource(&m_DrawCommandBuffers[i], "DCB %d", i);
			sce::Agc::Core::registerResource(&m_FlipLabels[i], "Flip label %d", i);
#endif // !SKTBD_SHIP
		}
	}

	void AGCGraphicsContext::CreateFilpLabels()
	{
		// The flip labels are used to track if a frame resource is currently being used by the GPU.
		// In other words, if the flabel.m_value is 0 then the GPU is currently using the frame resource and we should wait until it is done.
		m_FlipLabelsMemory = m_MemoryAllocator.TypedAlignedAllocate<sce::Agc::Label,sce::Agc::Alignment::kLabel>(GRAPHICS_SETTINGS_NUMFRAMERESOURCES);
		m_FlipLabels = m_FlipLabelsMemory.data;
		for (uint32_t i = 0; i < GRAPHICS_SETTINGS_NUMFRAMERESOURCES; ++i)
			m_FlipLabels[i].m_value = 1; // 1 means "not used by GPU"
	}

	void AGCGraphicsContext::CreateRenderTargets()
	{
		// Set up the RenderTarget spec
		sce::Agc::Core::RenderTargetSpec rtSpec;
		rtSpec.init();
		rtSpec.m_width = static_cast<uint32_t>(m_ClientWidth);
		rtSpec.m_height = static_cast<uint32_t>(m_ClientHeight);
		rtSpec.m_format = { sce::Agc::Core::TypedFormat::k8_8_8_8Srgb, sce::Agc::Core::Swizzle::kRGBA_R4S4 };
		//rtSpec.m_format = BufferFormatToAGC(); // TODO: Get it from props
		rtSpec.m_tileMode = sce::Agc::CxRenderTarget::TileMode::kRenderTarget;

		// Get the aligned size of the render target and allocate the require memory backing
		sce::Agc::SizeAlign rtSize = sce::Agc::Core::getSize(&rtSpec);
		m_RenderTargetsMemory[0] = m_MemoryAllocator.TypedAlignedAllocate<uint8_t,sce::Agc::Alignment::kMaxTiledAlignment>(rtSize.m_size);
		rtSpec.m_dataAddress = m_RenderTargetsMemory[0].data;

		// We can now initialize the render target. This will check that the dataAddress is properly aligned
		SceError error = sce::Agc::Core::initialize(&m_RenderTargets[0], &rtSpec);
		SCE_AGC_ASSERT_MSG(error == SCE_OK, "Failed to initialize RenderTarget.");

#ifndef SKTBD_SHIP
		sce::Agc::Core::registerResource(&m_RenderTargets[0], "Color %d", 0);
#endif // !SKTBD_SHIP

		// Now that we have the first RT set up, we will use it to initialise all the others.
		// The only difference is that each render target needs a unique dataAddress!
		for (uint32_t i = 1u; i < GRAPHICS_SETTINGS_NUMFRAMERESOURCES; ++i)
		{
			m_RenderTargets[i] = m_RenderTargets[0];
			m_RenderTargetsMemory[i] = m_MemoryAllocator.TypedAlignedAllocate<uint8_t, sce::Agc::Alignment::kMaxTiledAlignment>(rtSize.m_size);
			m_RenderTargets[i].setDataAddress(m_RenderTargetsMemory[i].data);
#ifndef SKTBD_SHIP
			sce::Agc::Core::registerResource(&m_RenderTargets[i], "Color %d", i);
#endif // !SKTBD_SHIP
		}

		// Enable writing to all the channels of the render target
		m_RenderTargetMask = sce::Agc::CxRenderTargetMask().init().setMask(0, 0xf);
	}

	void AGCGraphicsContext::CreateDepthTarget()
	{
		// TODO: Add stencil test?

		//Set up the DepthRenderTarget spec
		sce::Agc::Core::DepthRenderTargetSpec drtSpec = {};
		drtSpec.init();																	// Always call init()
		drtSpec.m_width = static_cast<uint32_t>(m_ClientWidth);							// The depth buffer will match our application client size
		drtSpec.m_height = static_cast<uint32_t>(m_ClientHeight);
		drtSpec.m_depthFormat = sce::Agc::CxDepthRenderTarget::DepthFormat::k32Float;	// We want a 32-bit floating point depth buffer
		drtSpec.m_compression = sce::Agc::Core::MetadataCompression::kHtileDepth;		// Compression for depth and stencil
		//TODO: drtSpec.m_stencilFormat = sce::Agc::CxDepthRenderTarget::StencilFormat::k8UInt;	// We want a 8-bit unsigned integer stencil buffer
		//TODO: drtSpec.m_compression = sce::Agc::Core::MetadataCompression::kHtileStencil;		// Compression for depth AND stencil

		// Get the aligned size of the depth target and allocate the required memory backing (for the locations on where to read and write to it)
		sce::Agc::SizeAlign dtSize = sce::Agc::Core::getSize(&drtSpec, sce::Agc::Core::DepthRenderTargetComponent::kDepth);
		m_DepthStencilTargetMemory = m_MemoryAllocator.TypedAlignedAllocate<uint8_t, sce::Agc::Alignment::kMaxTiledAlignment>(dtSize.m_size);
		drtSpec.m_depthReadAddress = drtSpec.m_depthWriteAddress = m_DepthStencilTargetMemory.data;

		//// Get the aligned size of the stencil target and allocate the required memory backing (for the locations on where to read and write to it)
		//TODO: sce::Agc::SizeAlign stSize = sce::Agc::Core::getSize(&drtSpec, sce::Agc::Core::DepthRenderTargetComponent::kStencil);
		//TODO: drtSpec.m_stencilReadAddress = drtSpec.m_stencilWriteAddress = allocDmem(stSize);

		// Retrieve the size of htile buffer for this depth render target and allocate the required memory backing.
		// The HTILE buffer is optional. It contains a DWORD that serves as a summary of the depth buffer,
		// which can be used to speed up depth clears or even sometimes avoid depth reads entirely!
		// Read: https://p.siedev.net/resources/documents/SDK/7.000/Agc-Reference/0795.html
		sce::Agc::SizeAlign htileSize = sce::Agc::Core::getSize(&drtSpec, sce::Agc::Core::DepthRenderTargetComponent::kHtile);
		m_DepthStencilHTileMemory = m_MemoryAllocator.TypedAlignedAllocate<uint8_t, sce::Agc::Alignment::kMaxTiledAlignment>(htileSize.m_size);
		drtSpec.m_htileAddress = m_DepthStencilHTileMemory.data;

		// We can now initialize the depth render target. This will check that the addresses are properly aligned
		SceError error = sce::Agc::Core::initialize(&m_DepthStencilTarget, &drtSpec);
		SCE_AGC_ASSERT_MSG(error == SCE_OK, "Failed to initialize DepthRenderTarget.");

#ifndef SKTBD_SHIP
		sce::Agc::Core::registerResource(&m_DepthStencilTarget, "Depth");
#endif // !SKTBD_SHIP

		// The depth/stencil render targets also own the clear value
		m_DepthStencilTarget.setDepthClearValue(1.0f);
		//TODO: m_DepthStencilTarget.setStencilClearValue(0u);

		// Enable depth testing and writing, as well as stencil testing
		m_DepthStencilControl.init();
		m_DepthStencilControl.setDepthWrite(sce::Agc::CxDepthStencilControl::DepthWrite::kEnable);
		m_DepthStencilControl.setDepth(sce::Agc::CxDepthStencilControl::Depth::kEnable);
		m_DepthStencilControl.setDepthFunction(sce::Agc::CxDepthStencilControl::DepthFunction::kLess);
		//TODO: m_DepthStencilControl.setStencil(sce::Agc::CxDepthStencilControl::Stencil::kEnable);
		//TODO: m_DepthStencilControl.setStencilFunction(sce::Agc::CxDepthStencilControl::StencilFunction::kAlways);

		m_BlendAlphaControl.init();
		m_BlendAlphaControl.setBlend(sce::Agc::CxBlendControl::Blend::kEnable);
		m_BlendAlphaControl.setAlphaBlendFunc(sce::Agc::CxBlendControl::AlphaBlendFunc::kAdd);
		m_BlendAlphaControl.setAlphaDestMultiplier(sce::Agc::CxBlendControl::AlphaDestMultiplier::kOneMinusSrcAlpha);
		m_BlendAlphaControl.setAlphaSourceMultiplier(sce::Agc::CxBlendControl::AlphaSourceMultiplier::kOne);
		m_BlendAlphaControl.setColorBlendFunc(sce::Agc::CxBlendControl::ColorBlendFunc::kAdd);
		m_BlendAlphaControl.setColorDestMultiplier(sce::Agc::CxBlendControl::ColorDestMultiplier::kOneMinusSrcAlpha);
		m_BlendAlphaControl.setColorSourceMultiplier(sce::Agc::CxBlendControl::ColorSourceMultiplier::kSrcAlpha);
	}

	void AGCGraphicsContext::InitialiseVideoOut()
	{
		// First we need to select what we want to display on, which in this case is the TV, also known as SCE_VIDEO_OUT_BUS_TYPE_MAIN.
		m_VideoHandle = sceVideoOutOpen(SCE_USER_SERVICE_USER_ID_SYSTEM, SCE_VIDEO_OUT_BUS_TYPE_MAIN, 0, NULL);
		SCE_AGC_ASSERT_MSG(m_VideoHandle >= 0, "sceVideoOutOpen() returns handle=%d\n", m_VideoHandle);

		// Next we need to inform scan-out about the format of our buffers. This can be done by directly talking to VideoOut or
		// by letting Agc::Core do the translation. To do so, we first need to get a RenderTargetSpec, which we can extract from
		// the list of CxRenderTargets passed into the function.
		sce::Agc::Core::RenderTargetSpec spec = {};
		SceError error = sce::Agc::Core::translate(&spec, &m_RenderTargets[0]);
		SCE_AGC_ASSERT(error == SCE_OK);

		// Next, we use this RenderTargetSpec to create a SceVideoOutBufferAttribute2 which tells VideoOut how it should interpret
		// our buffers. VideoOut needs to know how the color data in the target should be interpreted, and since our pixel shader has
		// been writing linear values into an sRGB RenderTarget, the data VideoOut will find in memory are sRGB encoded.
		SceVideoOutBufferAttribute2 attribute = {};
		error = sce::Agc::Core::translate(&attribute, &spec, sce::Agc::Core::Colorimetry::kSrgb, sce::Agc::Core::Colorimetry::kBt709);
		SCE_AGC_ASSERT(error == SCE_OK);

		// Ideally, all buffers should be registered with VideoOut in a single call to sceVideoOutRegisterBuffers2.
		// The reason for this is that the buffers provided in each call get associated with one attribute slot in the API.
		// Even if consecutive calls pass the same SceVideoOutBufferAttribute2 into the function, they still get assigned
		// new attribute slots. When processing a flip, there is significant extra cost associated with switching attribute
		// slots, which should be avoided.
		SceVideoOutBuffers* addresses = (SceVideoOutBuffers*)calloc(GRAPHICS_SETTINGS_NUMFRAMERESOURCES, sizeof(SceVideoOutBuffers));
		for (uint32_t i = 0; i < GRAPHICS_SETTINGS_NUMFRAMERESOURCES; ++i)
		{
			// We could manually call into VideoOut to set up the scan-out buffers, but Agc::Core provides a helper for this.
			addresses[i].data = m_RenderTargets[i].getDataAddress();
		}

		// VideoOut internally groups scan-out buffers in sets. Every buffer in a set has the same attributes and switching (flipping) between
		// buffers of the same set is a light-weight operation. Switching to a buffer from a different set is significantly more expensive
		// and should be avoided. If an application wants to change the attributes of a scan-out buffer or wants to unregister buffers,
		// these operations are done on whole sets and affect every buffer in the set. Here we only registers one set of buffers and never
		// modify the set.
		error = sceVideoOutRegisterBuffers2(
			m_VideoHandle,
			m_VideoSetIndex,
			0,
			addresses,
			GRAPHICS_SETTINGS_NUMFRAMERESOURCES,
			&attribute,
			SCE_VIDEO_OUT_BUFFER_ATTRIBUTE_CATEGORY_UNCOMPRESSED,
			nullptr
		);
		SCE_AGC_ASSERT(error == SCE_OK);
		free(addresses);

		// Define out to control the video synchronisation. This will be used in the StartDraw() in sceVideoOutLatencyControlWaitBeforeInput().
		// Essentially, we will wait until it is safe to process the frame based on these settings.
		m_LatencyControl.control = SCE_VIDEO_OUT_LATENCY_CONTROL_WAIT_BY_FLIP_QUEUE_NUM;	// This allows us to simply pass the frame number as the flipArg.
		m_LatencyControl.targetNum = 1;														// Allow for the CPU to get one frame ahead. This means the duration of CPU + GPU processing should be less than one frame to hold framerate. In your game application, you probably want this to be 2 or 3.
		m_LatencyControl.extraUsec = 0;
	}

	void AGCGraphicsContext::CreateViewPort()
	{
		// Set up a viewport using a helper function from Core.
		sce::Agc::Core::setViewport(&m_Viewport, static_cast<uint32_t>(m_ClientWidth), static_cast<uint32_t>(m_ClientHeight), 0u, 0u, -1.0f, 1.0f);
	}

	std::function<void(void**)> AGCGraphicsContext::GetBufferMemoryInitializer(const BufferDesc& desc)
	{
		return [desc](void** m_memoryHandle)
		{
			if (!*m_memoryHandle)
			{
				*m_memoryHandle = RequestSysMemBlock(desc.Size, 1024 * 64, SCE_KERNEL_PROT_GPU_RW | SCE_KERNEL_PROT_CPU_RW);

				if (desc.pInitialDataToTransfer)
				{
					memcpy(*m_memoryHandle, desc.pInitialDataToTransfer, desc.Size);
				}
			}
		};
	}

	std::function<void(void**)> AGCGraphicsContext::GetTextureMemoryInitializer(const TextureDesc& desc)
	{
		//SKTBD_ASSERT(desc.)

		sce::Agc::SizeAlign sizealign;

		switch (desc.Type)
		{
		case TextureType_RenderTarget:
			sce::Agc::Core::RenderTargetSpec rtSpec;
			rtSpec.init();
			rtSpec.setWidth(desc.Width);
			rtSpec.setHeight(desc.Height);
			rtSpec.setDepth(desc.Depth);
			rtSpec.setFormat(SkateboardDataFormatToAGC(desc.Format));
			rtSpec.setNumMips(desc.Mips);
			rtSpec.setDimension(SkateboardTextureDimensionToAGCRenderTarget(desc.Dimension));
			rtSpec.setTileMode(sce::Agc::CxRenderTarget::TileMode::kRenderTarget);

			sizealign = sce::Agc::Core::getSize(&rtSpec);
			break;

		case TextureType_::TextureType_DepthStencil:
			sce::Agc::Core::DepthRenderTargetSpec dtSpec;
			//dtSpec.

			sizealign = sce::Agc::Core::getSize(&dtSpec);
			break;

		default:
			sce::Agc::Core::TextureSpec TexSpec;
			TexSpec.init();
			TexSpec.setWidth(desc.Width);
			TexSpec.setHeight(desc.Height);
			TexSpec.setDepth(desc.Depth);
			TexSpec.setFormat(SkateboardDataFormatToAGC(desc.Format));
			TexSpec.setIsWriteable(desc.AccessFlags & ResourceAccessFlag_::ResourceAccessFlag_GpuWrite);
			TexSpec.setNumMips(desc.Mips);
			TexSpec.setTileMode(sce::Agc::Core::Texture::TileMode::kStandard64KB);
			TexSpec.setType(SkateboardTextureDimensionToAGC(desc.Dimension));

			sizealign = sce::Agc::Core::getSize(&TexSpec);
			break;
		};

		return [desc,sizealign](void** m_memoryHandle)
		{
			if (!*m_memoryHandle)
			{
				*m_memoryHandle = RequestSysMemBlock(sizealign.m_size,sizealign.m_align, SCE_KERNEL_PROT_GPU_RW | SCE_KERNEL_PROT_CPU_RW);

				if (desc.pInitialDataToTransfer)
				{
					memcpy(*m_memoryHandle, desc.pInitialDataToTransfer, desc.Width);
				};
			}
		};
	}

	std::function<void(void**)> AGCGraphicsContext::GetMemoryDeInitializer(GPUResourceType_ type)
	{
		return std::function<void(void**)>();
	}


}