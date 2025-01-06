#pragma once
#include "Skateboard/Renderer/Api/RenderingApi.h"

namespace Skateboard
{
	class D3DGraphicsContext;
	class Scene;

	// <summary>
	class D3DRenderingApi final : public RenderingApi
	{
	public:

		void SetViewport(Viewport n_viewport, CommandBuffer& cb) final override;
		void SetScissor(Rect n_scissor, CommandBuffer& cb) final override;

//		void ResizeBackBuffers(uint32_t width, uint32_t height) final override;

		virtual void Dispatch(uint32_t X_groups, uint32_t Y_groups, uint32_t Z_groups, CommandBuffer& cb) final override;
		virtual void Draw(uint32_t StartingVertex, uint32_t VertexCount, CommandBuffer& cb) final override;
		virtual void DrawInstanced(uint32_t StartingVertex, uint32_t VertexCount, uint32_t InstanceCount, uint32_t StartingInstance, CommandBuffer& cb) final override;
		virtual void DrawIndexed(uint32_t StratingVertex, uint32_t StartingIndex, uint32_t IndexCount, CommandBuffer& cb) final override;
		virtual void DrawIndexedInstanced(uint32_t StratingVertex, uint32_t StartingIndex, uint32_t StartingInstance, uint32_t IndexCount, uint32_t InstanceCount, CommandBuffer& cb) final override;

	//	virtual void DispatchRays(RaytracingPipeline* pipeline) final override;

		virtual void BeginCommandBuffer(CommandBuffer& cb)																						final override;		
		virtual void EndCommandBuffer(CommandBuffer& cb)																						final override;
		virtual void SubmitCommandBuffers(CommandBuffer** cb, uint32_t count)																	final override;

		virtual void SetInline32bitDataGraphics(uint32_t InputSlot, void* Data, uint32_t size, CommandBuffer& cb)								final override;
		virtual void SetInline32bitDataCompute(uint32_t InputSlot, void* Data, uint32_t size, CommandBuffer& cb)								final override;

		virtual void SetInlineResourceViewGraphics(uint32_t InputSlot, Buffer* Buffer, BufferViewDesc desc, ViewAccessType_ Type, CommandBuffer& cb) final override;
		virtual void SetInlineResourceViewCompute(uint32_t InputSlot, Buffer* Buffer, BufferViewDesc desc, ViewAccessType_ Type, CommandBuffer& cb)  final override;

		virtual void SetDescriptorTableGraphics(uint32_t InputSlot, const DescriptorTable& table, CommandBuffer& cb)							final override;
		virtual void SetDescriptorTableCompute(uint32_t InputSlot, const DescriptorTable& table, CommandBuffer& cb)								final override;
												
		virtual void SetInputLayoutCompute(ShaderInputLayout* inputLayout, CommandBuffer& cb)													final override;
		virtual void SetInputLayoutGraphics(ShaderInputLayout* inputLayout, CommandBuffer& cb) 													final override;

		virtual void SetPipelineState(const Pipeline& pipeline, CommandBuffer& cb)																final override;

		virtual void SetVertexBuffer(VertexBufferView* vbview, uint32_t numViews, CommandBuffer& cb)											final override;

		virtual void SetPrimitiveTopology(SKTBD_PRIMITIVE_TOPOLOGY, CommandBuffer& cb)															final override;


		virtual void SetIndexBuffer(IndexBufferView* ibview, CommandBuffer& cb)																	final override;
		virtual void SetRenderTargets(RenderTargetView** views, uint32_t numViews, DepthStencilView* DepthRenderTarget, CommandBuffer& cb)	final override;
		virtual void WaitForCommandBufer(CommandBuffer& cb)																						final override;	
	};
}