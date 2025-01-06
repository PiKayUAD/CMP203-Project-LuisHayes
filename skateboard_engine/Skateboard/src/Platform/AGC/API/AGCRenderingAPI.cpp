#include <sktbdpch.h>
#include "AGCRenderingAPI.h"
#include "Platform/AGC/AGCGraphicsContext.h"
#include "Platform/AGC/AGCPipeline.h"
#include "Platform/AGC/AGCBuffer.h"

namespace Skateboard
{

	void AGCRenderingAPI::SetViewport(uint32_t width, uint32_t height)
	{
	}

	void AGCRenderingAPI::ResizeBackBuffers(uint32_t width, uint32_t height)
	{

	}

	void AGCRenderingAPI::StartDraw(float4 clearColour)
	{
		// Grab the relevant data for the current frame. 
		const uint32_t frameIndex = gAGCContext->GetCurrentFrameResourceIndex();
		int32_t videoHandle = gAGCContext->GetVideoHandle();
		const SceVideoOutLatencyControl& latencyControl = gAGCContext->GetLatencyControl();
		sce::Agc::DrawCommandBuffer& dcb = gAGCContext->GetDrawCommandBuffer();
		sce::Agc::Core::StateBuffer& sb = gAGCContext->GetStateBuffer();
		sce::Agc::CxRenderTarget& rt = gAGCContext->GetRenderTarget();
		sce::Agc::CxDepthRenderTarget& dst = gAGCContext->GetDepthStencilTarget();
		const sce::Agc::CxRenderTargetMask& rtMask = gAGCContext->GetRenderTargetMask();
		const sce::Agc::CxDepthStencilControl& dsControl = gAGCContext->getDepthStencilControl();
		const auto& baControl = gAGCContext->getBlendControl();
		sce::Agc::Label& flipLabel = gAGCContext->GetFlipLabel();
		const sce::Agc::CxViewport& viewport = gAGCContext->GetViewport();

		// Check if the command buffer has been fully processed, if so it's safe for us to overwrite it on the CPU.
		while (flipLabel.m_value != 1)
			sceKernelUsleep(1000);

		// We can now set the flip label to 0, which the GPU will set back to 1 when it's done.
		flipLabel.m_value = 0;

		// Delay processing on the CPU to control latency. If this causes GPU stalls, you need to increase targetNum or decrease extraUsec in latencyControl.
		SceError error = sceVideoOutLatencyControlWaitBeforeInput(videoHandle, &latencyControl, nullptr);
		SCE_AGC_ASSERT(error == SCE_OK);
		// Notify VideoOut that we are about to begin processing the frame. This the start point of the latency computation.
		static int64_t monotonicallyIncrementedInteger = 0;
		error = sceVideoOutLatencyMeasureSetStartPoint(videoHandle, monotonicallyIncrementedInteger++);
		SCE_AGC_ASSERT(error == SCE_OK);


		// First we reset our components, since we're writing a completely new DCB.
		// This is actually quite wasteful, since we could reuse the previous data, but the
		// point of this code is to demonstrate a Gnm-like approach to writing DCBs.
		dcb.resetBuffer();
		sb.reset();

		// This will stall the Command Processor (CP) until the buffer is no longer being displayed.
		// Note that we're actually pulling the DCB out of the context and accessing it
		// directly here. This is very much how Agc's contexts work. They do not hide away the underlying
		// components but mostly just try to remove redundant work.
		dcb.waitUntilSafeForRendering(videoHandle, frameIndex);

		// Clear our current RenderTarget by using Agc::Toolkit.
		sce::Agc::Toolkit::Result toolkitResult1 = sce::Agc::Toolkit::clearRenderTargetCs(&dcb, &rt, sce::Agc::Core::Encoder::encode({ (double)clearColour.x, (double)clearColour.y, (double)clearColour.z, (double)clearColour.w }));
		SCE_AGC_ASSERT(toolkitResult1.m_errorCode == SCE_OK);

		// Clear our current DepthRenderTarget by using Agc::Toolkit.
		sce::Agc::Toolkit::Result toolkitResult2 = sce::Agc::Toolkit::clearDepthRenderTargetCs(&dcb, &dst);
		SCE_AGC_ASSERT(toolkitResult2.m_errorCode == SCE_OK);

		// Merge the two toolkit results together so that we can issue a single gpuSyncEvent for both toolkit
		// operations.
		toolkitResult1 = toolkitResult1 | toolkitResult2;

		// Wait for the clears to complete
		sce::Agc::Core::gpuSyncEvent(&dcb,
			toolkitResult1.getSyncWaitMode(),
			toolkitResult1.getSyncCacheOp(sce::Agc::Toolkit::Result::Caches::kGl2));

		// Set up the viewport and render targets
		sb.setState(rtMask);
		sb.setState(viewport);
		sb.setState(rt);
		sb.setState(dst);
		sb.setState(dsControl);
		sb.setState(baControl);
	}

	void AGCRenderingAPI::EndDraw()
	{
		const uint32_t frameIndex = gAGCContext->GetCurrentFrameResourceIndex();
		int32_t videoHandle = gAGCContext->GetVideoHandle();
		sce::Agc::DrawCommandBuffer& dcb = gAGCContext->GetDrawCommandBuffer();
		sce::Agc::Label& flipLabel = gAGCContext->GetFlipLabel();

		// Submit a flip via the GPU.
		// Note: on PlayStation®5, RenderTargets write into the GL2 cache, but the scan-out
		// does not snoop any GPU caches. As such, it is necessary to flush these writes to memory before they can
		// be displayed. This flush is performed internally by setFlip() so we don't need to do it
		// on the application side.
		dcb.setFlip(videoHandle, frameIndex, SCE_VIDEO_OUT_FLIP_MODE_VSYNC, 0);

		// The last thing we do in the command buffer is write 1 to the flip label to signal that command buffer
		// processing has finished. 
		//
		// While Agc provides access to the lowest level of GPU synchronization faculties, it also provides
		// functionality that builds the correct synchronization steps in an easier fashion.
		// Since synchonization should be relatively rare, spending a few CPU cycles on letting the library
		// work out what needs to be done is generally a good idea.
		sce::Agc::Core::gpuSyncEvent(
			&dcb,
			// The SyncWaitMode controls how the GPU's Command Processor (CP) handles the synchronization.
			// By setting this to kAsynchronous, we tell the CP that it doesn't have to wait for this operation
			// to finish before it can start the next frame. Instead, we could ask it to drain all graphics work
			// first, but that would be more aggressive than we need to be here.
			sce::Agc::Core::SyncWaitMode::kAsynchronous,
			// Since we are making the label write visible to the CPU, it is not necessary to flush any caches
			// and we set the cache op to 'kNone'.
			sce::Agc::Core::SyncCacheOp::kNone,
			// Write the flip label and make it visible to the CPU.
			sce::Agc::Core::SyncLabelVisibility::kCpu,
			&flipLabel,
			// We write the value "1" to the flip label.
			1);

		// Finally, we submit the work to the GPU. Since this is the only work on the GPU, we set its priority to normal.
		// The only reason to set the priority to kInterruptPriority is to make a submit expel work from the GPU we have previously
		// submitted. 
		SceError error = sce::Agc::submitGraphics(
			sce::Agc::GraphicsQueue::kNormal,
			dcb.getSubmitPointer(),
			dcb.getSubmitSize());
		SCE_AGC_ASSERT(error == SCE_OK);

		// If the application is suspended, it will happen during this call. As a side-effect, this is equivalent to
		// calling resetQueue(ResetQueueOp::kAllAccessible).
		error = sce::Agc::suspendPoint();
		SCE_AGC_ASSERT(error == SCE_OK);
	}

	void AGCRenderingAPI::Present()
	{
		// AGC does not contain a present operation. Instead, we use the flip label to determine when the GPU has finished
		// The flip operation is stored on the draw command buffer and is submitted with its graphics
		// In other words, playstation doesn't support multi-swap chain techniques right out of the box

		// Could move submitGraphics() here... keeps it more in line with the other APIs i.e D3D12's Present()

	}

	void AGCRenderingAPI::Draw(RasterizationPipeline* pipeline, VertexBuffer* vb)
	{
	}

	void AGCRenderingAPI::DrawIndexed(RasterizationPipeline* pipeline, VertexBuffer* vb, IndexBuffer* ib)
	{
		// Fetch the draw command buffer
		sce::Agc::DrawCommandBuffer& dcb = gAGCContext->GetDrawCommandBuffer();
		AGCRasterizationPipeline* agcPipeline = static_cast<AGCRasterizationPipeline*>(pipeline);
		//AGCVertexBuffer* vertexBuffer = static_cast<AGCVertexBuffer*>(vb);
		//AGCIndexBuffer* indexBuffer = static_cast<AGCIndexBuffer*>(ib);

		//// Bind the vertex & index buffers, and the pipeline
	
		//agcPipeline->Bind();
		//agcPipeline->SetVertexBuffer(vertexBuffer);
		//dcb.setIndexSize(sce::Agc::IndexSize::k32);
		//dcb.drawIndex(indexBuffer->GetIndexCount(), indexBuffer->GetIndexBuffer(), agcPipeline->GetDrawModifier());
		//agcPipeline->Unbind();
	}

	

	//void AGCRenderingAPI::DrawIndexedInstanced(RasterizationPipeline* pipeline, MeshID meshID, uint32_t* pInstanceOffsetRootConstant)
	//{		
	//	//SKTBD_CORE_ASSERT(p_ActiveScene != nullptr, "No active was set. Did you forget to call BeginScene()?");

	//	//if (!p_ActiveScene->IsMeshValid(meshID))
	//	//{
	//	//	printf("Invalid mesh instance ID\n");
	//	//	return;
	//	//}

	//	//// Fetch the draw command buffer
	//	//sce::Agc::DrawCommandBuffer& dcb = gAGCContext->GetDrawCommandBuffer();
	//	//AGCRasterizationPipeline* agcPipeline = static_cast<AGCRasterizationPipeline*>(pipeline);

	//	//const uint32_t indexCount = p_ActiveScene->GetMeshIndexCount(meshID);
 //	//	const uint32_t instanceCount = p_ActiveScene->GetMeshInstanceCount(meshID);			
	//	//const uint32_t startIndexLocation = p_ActiveScene->GetMeshStartIndexLocation(meshID);
	//	//const uint32_t startVertexLocation = p_ActiveScene->GetMeshStartVertexLocation(meshID);
	//	//const uint32_t startInstanceLocation = p_ActiveScene->GetMeshStartInstanceLocation(meshID);
	//	//const uint32_t vertexCount = p_ActiveScene->GetMeshVertexCount(meshID);
	//	//*pInstanceOffsetRootConstant = startInstanceLocation;

	//	//AGCVertexBuffer* agcVertexBuffer = static_cast<AGCVertexBuffer*>(p_ActiveScene->GetVertexBuffer());
	//	//AGCIndexBuffer* indexBuffer = static_cast<AGCIndexBuffer*>(p_ActiveScene->GetIndexBuffer());

	//	//agcPipeline->Bind();
	//	//agcPipeline->SetVertexBuffer(agcVertexBuffer);
	//
	//	//uint32_t meshId = (uint32_t)meshID;

	//	//AGCIndirectArgumentBuffer* args = (AGCIndirectArgumentBuffer*)p_ActiveScene->GetIndirectArgumentBuffer(meshId);
	//	//dcb.setBaseDrawIndirectArgs((sce::Agc::DrawIndexIndirectArgs*)args->GetIndirectArgumentBuffer());

	//	//dcb.setIndexBuffer(indexBuffer->GetIndexBuffer());
	//	//dcb.setIndexSize(sce::Agc::IndexSize::k32);
	//	//
	//	//// TODO: Why does this break instancing??? - Overrides index call count for all subsequent indirect calls
	//	////dcb.setIndexCount(indexCount);
	//	////

	//	//dcb.drawIndexIndirect(0, agcPipeline->GetDrawModifier());

	//	//agcPipeline->Unbind();
	//}

	//void AGCRenderingAPI::DrawMesh(RasterizationPipeline* pipeline, Mesh* mesh)
	//{
	//	// Fetch the draw command buffer
	//	sce::Agc::DrawCommandBuffer& dcb = gAGCContext->GetDrawCommandBuffer();
	//	AGCRasterizationPipeline* agcPipeline = static_cast<AGCRasterizationPipeline*>(pipeline);

	//	AGCVertexBuffer* vertexBuffer = static_cast<AGCVertexBuffer*>(mesh->VertexResource);
	//	AGCIndexBuffer* indexBuffer = static_cast<AGCIndexBuffer*>(mesh->IndexResource);

	//	//// Bind the vertex & index buffers, and the pipeline
	//	agcPipeline->Bind();
	//	agcPipeline->SetVertexBuffer(vertexBuffer);
	//	dcb.setIndexSize(sce::Agc::IndexSize::k32);
	//	dcb.drawIndex(indexBuffer->GetIndexCount(), indexBuffer->GetIndexBuffer(), agcPipeline->GetDrawModifier());
	//	agcPipeline->Unbind();
	//}

	void AGCRenderingAPI::Dispatch(ComputePipeline* pipeline)
	{
		// 

		// Get API objects
		sce::Agc::DrawCommandBuffer& dcb = gAGCContext->GetDrawCommandBuffer();
		AGCComputePipeline* agcPipeline = static_cast<AGCComputePipeline*>(pipeline);

		// Dispatch just like D3D
		agcPipeline->Bind();
		const ComputePipelineDesc& desc = agcPipeline->GetDesc();
		dcb.dispatch(desc.DispatchSize.ThreadCountX, desc.DispatchSize.ThreadCountY, desc.DispatchSize.ThreadCountZ, agcPipeline->GetDispatchModifier());

		// Wait for the dispatch to finish and invalidate the stale cache lines.
		// If we don't do this, the compute shader may not have finished its execution while a UAV its writing to
		// is being read by a pixel shader (and therefore produces artefacts!). Now if we had a lot of compute shaders running before
		// any of their resources are used, it would probably be best to have only one of these sync events after the last candidate
		sce::Agc::Core::gpuSyncEvent(&dcb, sce::Agc::Core::SyncWaitMode::kDrainCompute, sce::Agc::Core::SyncCacheOp::kInvalidateGl01);
	}

	/*void AGCRenderingAPI::DispatchRays(RaytracingPipeline* pipeline)
	{
	}

	void AGCRenderingAPI::CopyUAVToBackBuffer(UnorderedAccessBuffer* pUAV)
	{
	}

	void AGCRenderingAPI::CopyUAVToFrameBuffer(FrameBuffer* pFrame, UnorderedAccessBuffer* pUAV)
	{
	}*/
}