#pragma once
#include "Platform/AGC/AGCF.h"
#include "Skateboard/Renderer/Api/RenderingApi.h"

namespace Skateboard
{
	class AGCRenderingAPI final : public RenderingApi
	{
		virtual ~AGCRenderingAPI() final override {}

		virtual void SetViewport(uint32_t width, uint32_t height) final override;

		virtual void ResizeBackBuffers(uint32_t width, uint32_t height) final override;

		// RENDERER UTILITIES //////////////////////////////////////////////////////////////

		virtual void StartDraw(float4 clearColour) final override;

		virtual void EndDraw() final override;

		virtual void Present() final override;

		// DRAW COMMANDS ///////////////////////////////////////////////////////////////////

		virtual void Draw(RasterizationPipeline* pipeline, VertexBuffer* vb) final override;

		virtual void DrawIndexed(RasterizationPipeline* pipeline, VertexBuffer* vb, IndexBuffer* ib) final override;

		//virtual void DrawIndexedInstanced(RasterizationPipeline* pipeline, MeshID meshID, uint32_t* pInstanceOffsetRootConstant) final override;

		//virtual void DrawMesh(RasterizationPipeline* pipeline, Mesh* mesh) final override;

		virtual void Dispatch(ComputePipeline* pipeline) final override;


		/*virtual void DispatchRays(RaytracingPipeline* pipeline) final override;
		virtual void CopyUAVToBackBuffer(UnorderedAccessBuffer* pUAV) final override;
		virtual void CopyUAVToFrameBuffer(FrameBuffer* pFrame, UnorderedAccessBuffer* pUAV) final override;*/
	};
}
