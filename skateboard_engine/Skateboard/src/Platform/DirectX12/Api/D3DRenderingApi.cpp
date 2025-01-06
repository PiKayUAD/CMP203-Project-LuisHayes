#include "sktbdpch.h"
#include "D3DRenderingApi.h"
#include "Platform/DirectX12/D3DGraphicsContext.h"
#include "Platform/DirectX12/D3DBuffer.h"

#include "Skateboard/Renderer/Pipeline.h"
#include "Skateboard/Scene/SceneBuilder.h"

namespace Skateboard
{

	void D3DRenderingApi::SetViewport(Viewport n_viewport, CommandBuffer& cb)
	{

	}

	void D3DRenderingApi::SetScissor(Rect n_scissor, CommandBuffer& cb)
	{

	}

	void D3DRenderingApi::Dispatch(uint32_t X_groups, uint32_t Y_groups, uint32_t Z_groups, CommandBuffer& cb)
	{
		// Validate the graphics command list before recording any further instructions.
		auto const commandList = static_cast<CommandBufferMemHandle*>(cb.GetResource())->m_Commandlist;
		SKTBD_CORE_ASSERT(commandList, "Null command list!");

		// Adds a dispatch api call to the graphics command list.
		// Please note, the kernel does not execute at this stage. Use the ExecuteCommandList() api appropriately to do so.
		commandList->Dispatch(X_groups, Y_groups, Z_groups);
	}

	void D3DRenderingApi::Draw(uint32_t StartingVertex, uint32_t VertexCount, CommandBuffer& cb)
	{
		DrawInstanced(StartingVertex, VertexCount, 1, 0, cb);
	}

	void D3DRenderingApi::DrawInstanced(uint32_t StartingVertex, uint32_t VertexCount, uint32_t InstanceCount, uint32_t StartingInstance, CommandBuffer& cb)
	{
		auto handle = static_cast<CommandBufferMemHandle*>(cb.GetResource());
		handle->m_Commandlist->DrawInstanced(VertexCount, InstanceCount, StartingVertex, StartingInstance);
	}

	void D3DRenderingApi::DrawIndexed(uint32_t StratingVertex, uint32_t StartingIndex, uint32_t IndexCount, CommandBuffer& cb)
	{
		DrawIndexedInstanced(StratingVertex, StartingIndex, 0, IndexCount, 1, cb);
	}

	void D3DRenderingApi::DrawIndexedInstanced(uint32_t StratingVertex, uint32_t StartingIndex, uint32_t StartingInstance, uint32_t IndexCount, uint32_t InstanceCount, CommandBuffer& cb)
	{
		auto handle = static_cast<CommandBufferMemHandle*>(cb.GetResource());
		handle->m_Commandlist->DrawIndexedInstanced(IndexCount, InstanceCount, StartingIndex, StratingVertex, StartingInstance);
	}

	void D3DRenderingApi::BeginCommandBuffer(CommandBuffer& cb)
	{
		gD3DContext->BeginCommandBuffer(cb);
	}

	void D3DRenderingApi::EndCommandBuffer(CommandBuffer& cb)
	{
		gD3DContext->EndCommandBuffer(cb);
	}

	void D3DRenderingApi::SubmitCommandBuffers(CommandBuffer** cb, uint32_t count)
	{
		gD3DContext->SubmitCommandBuffers(cb, count);
	}

	void D3DRenderingApi::SetInline32bitDataCompute(uint32_t InputSlot, void* Data, uint32_t size, CommandBuffer& cb)
	{
		auto handle = static_cast<CommandBufferMemHandle *>(cb.GetResource());
		handle->m_Commandlist->SetComputeRoot32BitConstants(InputSlot, size, Data, 0);
	}

	void D3DRenderingApi::SetInline32bitDataGraphics(uint32_t InputSlot, void* Data, uint32_t size, CommandBuffer& cb)
	{
		auto handle = static_cast<CommandBufferMemHandle*>(cb.GetResource());
		handle->m_Commandlist->SetGraphicsRoot32BitConstants(InputSlot, size, Data, 0);
	}


	void D3DRenderingApi::SetInlineResourceViewGraphics(uint32_t InputSlot, Buffer* Buffer, BufferViewDesc desc, ViewAccessType_ Type, CommandBuffer& cb)
	{
		auto handle = static_cast<CommandBufferMemHandle*>(cb.GetResource());
		auto addr = static_cast<D3D12MA::Allocation*>(Buffer->GetResource())->GetResource()->GetGPUVirtualAddress();

		SKTBD_CORE_ASSERT(desc.Type != BufferType_FormattedBuffer , "DX12 Doesnt Support Formatted Inline Buffers, Use Descriptor Heap Based Binding (Bindless or Descriptor Tables)")
		SKTBD_CORE_ASSERT(!(desc.Type == BufferType_StructureBuffer && desc.Flags & BufferViewFlags_AppendConsumeBuffer), "DX12 Doesnt Support AppendConsume Inline Buffers, Use Descriptor Heap Based Binding (Bindless or Descriptor Tables)")

		switch (Type)
		{
			case ViewAccessType_GpuRead			:
				handle->m_Commandlist->SetGraphicsRootShaderResourceView(InputSlot, addr + desc.Offset);
				break;
			case ViewAccessType_GpuReadWrite	:
				handle->m_Commandlist->SetGraphicsRootUnorderedAccessView(InputSlot, addr + desc.Offset);
				break;
			case ViewAccessType_ConstantBuffer	:
				handle->m_Commandlist->SetGraphicsRootConstantBufferView(InputSlot, addr + desc.Offset);
				break;
		}
	}

	void D3DRenderingApi::SetInlineResourceViewCompute(uint32_t InputSlot, Buffer* Buffer, BufferViewDesc desc, ViewAccessType_ Type, CommandBuffer& cb)
	{
		auto handle = static_cast<CommandBufferMemHandle*>(cb.GetResource());
		auto addr = static_cast<D3D12MA::Allocation*>(Buffer->GetResource())->GetResource()->GetGPUVirtualAddress();

		SKTBD_CORE_ASSERT(desc.Type == BufferType_FormattedBuffer, "DX12 Doesnt Support Formatted Inline Buffers, Use Descriptor Heap Based Binding (Bindless or Descriptor Tables)")
			SKTBD_CORE_ASSERT(desc.Type == BufferType_StructureBuffer && desc.Flags & BufferViewFlags_AppendConsumeBuffer, "DX12 Doesnt Support AppendConsume Inline Buffers, Use Descriptor Heap Based Binding (Bindless or Descriptor Tables)")

			switch (Type)
			{
			case ViewAccessType_GpuRead:
				handle->m_Commandlist->SetComputeRootShaderResourceView(InputSlot, addr + desc.Offset);
				break;
			case ViewAccessType_GpuReadWrite:
				handle->m_Commandlist->SetComputeRootUnorderedAccessView(InputSlot, addr + desc.Offset);
				break;
			case ViewAccessType_ConstantBuffer:
				handle->m_Commandlist->SetComputeRootConstantBufferView(InputSlot, addr + desc.Offset);
				break;
			}
	}

	void D3DRenderingApi::SetDescriptorTableGraphics(uint32_t InputSlot, const DescriptorTable& table, CommandBuffer& cb)
	{
		SKTBD_CORE_ASSERT(false, "DESCRIPTOR TABLES UNSUPPORTED")
		auto handle = static_cast<CommandBufferMemHandle*>(cb.GetResource());
		//	handle->m_Commandlist->SetGraphicsRootDescriptorTable(InputSlot);
	}
	
	void D3DRenderingApi::SetDescriptorTableCompute(uint32_t InputSlot, const DescriptorTable& table, CommandBuffer& cb)
	{
		SKTBD_CORE_ASSERT(false, "DESCRIPTOR TABLES UNSUPPORTED")
		auto handle = static_cast<CommandBufferMemHandle*>(cb.GetResource());
		//	handle->m_Commandlist->SetComputeRootDescriptorTable(InputSlot, static_cast<D3DDescriptorHandle*>(view.GetResource())->GetGPUPointer());
	}

	void D3DRenderingApi::SetInputLayoutCompute(ShaderInputLayout* inputLayout, CommandBuffer& cb)
	{
		auto handle = static_cast<CommandBufferMemHandle*>(cb.GetResource());
		handle->m_Commandlist->SetComputeRootSignature(static_cast<ID3D12RootSignature*>(inputLayout->GetResource()));
	}

	void D3DRenderingApi::SetInputLayoutGraphics(ShaderInputLayout* inputLayout, CommandBuffer& cb)
	{
		auto handle = static_cast<CommandBufferMemHandle*>(cb.GetResource());
		handle->m_Commandlist->SetGraphicsRootSignature(static_cast<ID3D12RootSignature*>(inputLayout->GetResource()));
	}

	void D3DRenderingApi::SetPipelineState(const Pipeline& pipeline, CommandBuffer& cb)
	{
		auto handle = static_cast<CommandBufferMemHandle*>(cb.GetResource());
		handle->m_Commandlist->SetPipelineState(static_cast<ID3D12PipelineState*>(pipeline.GetResource()));
	}

	void D3DRenderingApi::SetVertexBuffer(VertexBufferView* vbview, uint32_t numViews, CommandBuffer& cb)
	{
		auto handle = static_cast<CommandBufferMemHandle*>(cb.GetResource());
		std::vector<D3D12_VERTEX_BUFFER_VIEW> Views(numViews);

		std::transform(vbview, &vbview[numViews], Views.begin(), [](const VertexBufferView& vb) -> D3D12_VERTEX_BUFFER_VIEW
		{
				D3D12_VERTEX_BUFFER_VIEW vview = {};
				vview.BufferLocation = static_cast<D3D12MA::Allocation*>(vb.m_ParentResource->GetResource())->GetResource()->GetGPUVirtualAddress() + vb.m_Offset;
				vview.SizeInBytes = vb.m_VertexStride * vb.m_VertexCount;
				vview.StrideInBytes = vb.m_VertexStride;
				return vview;
		});

		handle->m_Commandlist->IASetVertexBuffers(0,numViews,Views.data());
	}

	void D3DRenderingApi::SetPrimitiveTopology(SKTBD_PRIMITIVE_TOPOLOGY topology, CommandBuffer& cb)
	{
		auto handle = static_cast<CommandBufferMemHandle*>(cb.GetResource());

		handle->m_Commandlist->IASetPrimitiveTopology((D3D_PRIMITIVE_TOPOLOGY)topology);
	}

	void D3DRenderingApi::SetIndexBuffer(IndexBufferView* ibview, CommandBuffer& cb)
	{
		auto handle = static_cast<CommandBufferMemHandle*>(cb.GetResource());

		if (ibview)
		{
			D3D12_INDEX_BUFFER_VIEW view = {};
			view.BufferLocation = static_cast<D3D12MA::Allocation*>(ibview->m_ParentResource->GetResource())->GetResource()->GetGPUVirtualAddress() + ibview->m_Offset;
			view.Format = (ibview->m_Format == bit32) ? DXGI_FORMAT_R32_UINT : DXGI_FORMAT_R16_UINT;
			view.SizeInBytes = ((ibview->m_Format == bit32) ? 4 : 2) * ibview->m_IndexCount;

			handle->m_Commandlist->IASetIndexBuffer(&view);
		}
		else
			handle->m_Commandlist->IASetIndexBuffer(nullptr);

	}

	void D3DRenderingApi::SetRenderTargets(RenderTargetView** views, uint32_t numViews, DepthStencilView* DepthRenderTarget, CommandBuffer& cb)
	{
		auto handle = static_cast<CommandBufferMemHandle*>(cb.GetResource());
	}

	void D3DRenderingApi::WaitForCommandBufer(CommandBuffer& cb)
	{
		auto handle = static_cast<CommandBufferMemHandle*>(cb.GetResource());
	}
}
