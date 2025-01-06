#pragma once
#include "Pipeline.h"
#include "Skateboard/Renderer/Api/RenderingApi.h"

namespace Skateboard
{
	// Forward declarations
	class GraphicsContext;
	class Mesh;

	class RenderCommand
	{
		friend class Application;
	public:
		static void SetViewport(Viewport n_viewport, CommandBuffer& cb = GraphicsContext::Context->GetDefaultCommandBuffer())
		{
			Api->SetViewport(n_viewport, cb);
		}


		static void SetScissor(Rect rect, CommandBuffer& cb = GraphicsContext::Context->GetDefaultCommandBuffer())
		{
			Api->SetScissor(rect,cb);
		}

		static void WaitUntilIdle()
		{
			Api->WaitUntilIdle();
		}

		static void Flush()
		{
			Api->Flush();
		}

		static void BeginCommandBuffer(CommandBuffer& cb)																																				
		{
			Api->BeginCommandBuffer(cb);
		};	

		static void EndCommandBuffer(CommandBuffer& cb)																																					
		{
			Api->EndCommandBuffer(cb);
		};

		static void SubmitCommandBuffers(CommandBuffer** cbs, uint32_t count)																																
		{
			Api->SubmitCommandBuffers(cbs, count);
		};

		static void SetInputLayoutGraphics(ShaderInputLayout* layout, CommandBuffer& cb = GraphicsContext::Context->GetDefaultCommandBuffer())
		{
			Api->SetInputLayoutGraphics(layout, cb);
		}

		static void SetInputLayoutCompute(ShaderInputLayout* layout, CommandBuffer& cb = GraphicsContext::Context->GetDefaultCommandBuffer())
		{
			Api->SetInputLayoutCompute(layout, cb);
		}

		static void SetInline32bitDataGraphics(uint32_t InputSlot, void* Data, uint32_t Size, CommandBuffer& cb = GraphicsContext::Context->GetDefaultCommandBuffer())											
		{
			Api->SetInline32bitDataGraphics(InputSlot, Data, Size, cb);
		};

		static void SetInline32bitDataCompute(uint32_t InputSlot, void* Data, uint32_t Size, CommandBuffer& cb = GraphicsContext::Context->GetDefaultCommandBuffer())
		{
			Api->SetInline32bitDataCompute(InputSlot, Data, Size, cb);
		};

		static void SetInlineResourceViewGraphics(uint32_t InputSlot, Buffer* Buffer, BufferViewDesc desc, ViewAccessType_ type , CommandBuffer& cb = GraphicsContext::Context->GetDefaultCommandBuffer())												
		{
			Api->SetInlineResourceViewGraphics(InputSlot, Buffer, desc, type, cb);
		};

		static void SetInlineResourceViewCompute(uint32_t InputSlot, Buffer* Buffer, BufferViewDesc desc, ViewAccessType_ type, CommandBuffer& cb = GraphicsContext::Context->GetDefaultCommandBuffer())
		{
			Api->SetInlineResourceViewGraphics(InputSlot,Buffer,desc, type, cb);
		};

		static void SetDescriptorTableCompute(uint32_t InputSlot, const DescriptorTable& table, CommandBuffer& cb = GraphicsContext::Context->GetDefaultCommandBuffer())										
		{
			Api->SetDescriptorTableCompute(InputSlot, table, cb);
		};

		static void SetDescriptorTableGraphics(uint32_t InputSlot, const DescriptorTable& table, CommandBuffer& cb = GraphicsContext::Context->GetDefaultCommandBuffer())
		{
			Api->SetDescriptorTableGraphics(InputSlot, table, cb);
		};

		static void SetPipelineState(const Pipeline& pipeline, CommandBuffer& cb = GraphicsContext::Context->GetDefaultCommandBuffer())						
		{
			Api->SetPipelineState(pipeline, cb);
		};

		static void SetVertexBuffer(VertexBufferView* vbviews, uint32_t numViews, CommandBuffer& cb = GraphicsContext::Context->GetDefaultCommandBuffer())												
		{
			Api->SetVertexBuffer(vbviews, numViews, cb);
		};

		static void SetPrimitiveTopology(SKTBD_PRIMITIVE_TOPOLOGY Topology, CommandBuffer& cb = GraphicsContext::Context->GetDefaultCommandBuffer())
		{
			Api->SetPrimitiveTopology(Topology, cb);
		}

		static void SetIndexBuffer(IndexBufferView* ibview, CommandBuffer& cb = GraphicsContext::Context->GetDefaultCommandBuffer())																	
		{
			Api->SetIndexBuffer(ibview, cb);
		};

		static void SetRenderTargets(RenderTargetView** views, uint32_t numViews, DepthStencilView* DepthRenderTarget, CommandBuffer& cb = GraphicsContext::Context->GetDefaultCommandBuffer())	
		{
			Api->SetRenderTargets(views, numViews, DepthRenderTarget, cb);
		};

		static void WaitForCommandBufer(CommandBuffer& cb)
		{
			Api->WaitForCommandBufer(cb);
		};

		static void Dispatch(uint32_t X_groups, uint32_t Y_groups, uint32_t Z_groups, CommandBuffer& cb = GraphicsContext::Context->GetDefaultCommandBuffer())											
		{
			Api->Dispatch(X_groups, Y_groups, Z_groups, cb);
		};

		static void Draw(uint32_t StartingVertex, uint32_t VertexCount, CommandBuffer& cb = GraphicsContext::Context->GetDefaultCommandBuffer())
		{
			Api->Draw(StartingVertex, VertexCount, cb);
		};

		static void DrawInstanced(uint32_t StartingVertex, uint32_t VertexCount, uint32_t InstanceCount, uint32_t StartingInstance, CommandBuffer& cb = GraphicsContext::Context->GetDefaultCommandBuffer())
		{
			Api->DrawInstanced(StartingVertex, VertexCount, InstanceCount, StartingInstance, cb);
		};

		static void DrawIndexed(uint32_t StratingVertex, uint32_t StartingIndex, uint32_t IndexCount, CommandBuffer& cb = GraphicsContext::Context->GetDefaultCommandBuffer())
		{
			Api->DrawIndexed(StratingVertex, StartingIndex, IndexCount, cb);
		};

		static void DrawIndexedInstanced(uint32_t StratingVertex, uint32_t StartingIndex, uint32_t StartingInstance, uint32_t IndexCount, uint32_t InstanceCount, CommandBuffer& cb = GraphicsContext::Context->GetDefaultCommandBuffer())
		{
			Api->DrawIndexedInstanced( StratingVertex,  StartingIndex,  StartingInstance,  IndexCount,  InstanceCount, cb);
		};


	private:
		static void Init();
		static RenderingApi* Api;
	};

	
}
