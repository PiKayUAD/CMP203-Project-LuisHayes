#pragma once
#include "Skateboard/Mathematics.h"
#include "Skateboard/Scene/SceneBuilder.h"
#include "Skateboard/Renderer/GraphicsContext.h"

namespace Skateboard
{

	// Forward declarations
	class RasterizationPipeline;
	class ComputePipeline;
	class RaytracingPipeline;
	class Scene;
	class SkeletalMesh;

	class RenderingApi
	{
	public:
		RenderingApi() {}
		virtual ~RenderingApi(){}

		virtual void SetViewport(Viewport Viewport, CommandBuffer& cb) = 0;
		virtual void SetScissor(Rect ScissorRectangle, CommandBuffer& cb) = 0;

		// RENDERER UTILITIES //////////////////////////////////////////////////////////////

		void WaitUntilIdle();

		void Flush();

		// DRAW COMMANDS ///////////////////////////////////////////////////////////////////

		virtual void BeginCommandBuffer(CommandBuffer& cb) = 0;
		virtual void EndCommandBuffer(CommandBuffer& cb) = 0;

		virtual void SubmitCommandBuffers(CommandBuffer**, uint32_t count) = 0;

		virtual void SetInline32bitDataGraphics(uint32_t InputSlot, void* Data, uint32_t Size, CommandBuffer& cb) = 0;
		virtual void SetInline32bitDataCompute(uint32_t InputSlot, void* Data, uint32_t Size, CommandBuffer& cb) = 0;

		virtual void SetInlineResourceViewGraphics(uint32_t InputSlot, Buffer* Buffer, BufferViewDesc desc, ViewAccessType_ Type,  CommandBuffer& cb) = 0;
		virtual void SetInlineResourceViewCompute(uint32_t InputSlot, Buffer* Buffer,	BufferViewDesc desc, ViewAccessType_ Type,  CommandBuffer& cb) = 0;

		virtual void SetDescriptorTableGraphics(uint32_t InputSlot, const DescriptorTable& table, CommandBuffer& cb) = 0;
		virtual void SetDescriptorTableCompute(uint32_t InputSlot, const DescriptorTable& table, CommandBuffer& cb) = 0;


		virtual void SetInputLayoutCompute(ShaderInputLayout* inputLayout, CommandBuffer& cb) = 0;
		virtual void SetInputLayoutGraphics(ShaderInputLayout* inputLayout, CommandBuffer& cb) = 0;


		virtual void SetPipelineState(const Pipeline& pipeline, CommandBuffer& cb) = 0;

		virtual void SetVertexBuffer(VertexBufferView* vbviews, uint32_t numViews, CommandBuffer& cb) = 0;
		virtual void SetPrimitiveTopology(SKTBD_PRIMITIVE_TOPOLOGY topology, CommandBuffer& cb) = 0;

		virtual void SetIndexBuffer(IndexBufferView* ibview, CommandBuffer& cb) = 0;

		virtual void SetRenderTargets(RenderTargetView** views, uint32_t numViews, DepthStencilView* DepthRenderTarget, CommandBuffer& cb) = 0;

		virtual void WaitForCommandBufer(CommandBuffer& cb) = 0;

		virtual void Dispatch(uint32_t X_groups, uint32_t Y_groups, uint32_t Z_groups, CommandBuffer& cb) = 0;

		virtual void Draw(uint32_t StartingVertex, uint32_t VertexCount, CommandBuffer& cb) = 0;
		virtual void DrawInstanced(uint32_t StartingVertex, uint32_t VertexCount, uint32_t InstanceCount, uint32_t StartingInstance,  CommandBuffer& cb) = 0;
		virtual void DrawIndexed(uint32_t StratingVertex, uint32_t StartingIndex, uint32_t IndexCount, CommandBuffer& cb) = 0;
		virtual void DrawIndexedInstanced(uint32_t StratingVertex, uint32_t StartingIndex, uint32_t StartingInstance, uint32_t IndexCount, uint32_t InstanceCount, CommandBuffer& cb) = 0;
	};

}
