//#pragma once
//#include "Skateboard/Renderer/Buffer.h"
//#include "Platform/DirectX12/Memory/D3DMemoryAllocator.h"
//#include "D3D.h"
//
//
//
//namespace Skateboard
//{
//	//static uint32_t m_SRVHandleSize;
//	extern size_t m_SRVHandleSize;
//
//	//Plan 
//	//
//	//create a staging buffer helper, ring buffer that will house all upload subsections or readback subsections
//	//copy data will copy into the upload buffer or the readback buffer
//	//when buffer is created in default
//	//
//	//if buffer residence is in upload heap this staging buffer reference will not be created;
//	//
//	//
//
//	template<typename T>
//	class D3DStructuredBuffer : public StructuredBuffer<T>
//	{
//	private:
//		struct DataHandle
//		{
//			D3DAllocWrappers::D3DMemoryHandle<> m_Allocation;
//		} m_Resource;
//		
//
//	public:
//		D3DStructuredBuffer(const std::wstring& debugName, const BufferDesc& desc) : StructuredBuffer<T, FrameResource>(debugName,desc)
//		{
//			D3D12_RESOURCE_DESC1 resourceDesc = {};
//
//			//cant do subresources would need to be 3 resources BLEH
//			resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
//			resourceDesc.Alignment = 0;
//			resourceDesc.Width = Width;
//			resourceDesc.Height = resourceDesc.DepthOrArraySize = resourceDesc.MipLevels = 1;
//			resourceDesc.Format = DXGI_FORMAT_UNKNOWN;
//			resourceDesc.SampleDesc.Count = 1;
//			resourceDesc.SampleDesc.Quality = 0;
//			resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
//			resourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
//
//			//if accessed
//			if (this->AccessFlags & BufferAccessFlag_GpuWrite && this->AccessFlags & BufferAccessFlag_CpuWrite)
//			{
//				m_DefaultResource = D3DAllocWrappers::Allocate<D3D12_HEAP_TYPE_DEFAULT, FrameResource>(resourceDesc);
//				m_UploadResource = D3DAllocWrappers::Allocate<D3D12_HEAP_TYPE_UPLOAD, FrameResource>(resourceDesc);
//			}
//			else if(this->AccessFlags & BufferAccessFlag_CpuWrite)
//			{
//				m_UploadResource = D3DAllocWrappers::Allocate<D3D12_HEAP_TYPE_UPLOAD, FrameResource>(resourceDesc);
//			}
//			else if(this->AccessFlags & BufferAccessFlag_GpuWrite)
//			{
//				m_DefaultResource = D3DAllocWrappers::Allocate<D3D12_HEAP_TYPE_DEFAULT, FrameResource>(resourceDesc);
//			}
//
//			if (this->AccessFlags & BufferAccessFlag_CpuRead)
//			{
//				m_ReadbackResource = D3DAllocWrappers::Allocate<D3D12_HEAP_TYPE_READBACK, FrameResource>(resourceDesc);
//			}
//			
//		};
//
//		virtual ~D3DStructuredBuffer() final override
//		{
//			Release();
//		};
//
//		D3D12_GPU_VIRTUAL_ADDRESS GetResourceGPUVirtualAddress() const { return GetGPUVirtualAddress(); };
//		ID3D12Resource* GetResource() const { return m_Buffer.Get(); }
//
//		Microsoft::WRL::ComPtr<ID3D12Resource>& GetResourceComPtr() { return m_Buffer; }
//
//		D3DDescriptorHandle GetSRVHandle() const { return m_SRVHandle + m_SRVHandleSize; }
//		D3DDescriptorHandle GetUAVHandle() const { return m_SRVHandle; }
//
//		void CopyData(uint32_t subresourceIndex, size_t offt, size_t elem_count, const void* data) final override 
//		{
//			
//		}
//
//		void CopyData(uint32_t subresourceIndex, size_t elemIndex, const void* data) final override
//		{
//
//		}
//
//		void Release() final override
//		{
//
//		}
//
//		const void* GetDefaultDescriptor() final override
//		{
//
//		}
//
//		const void* GetResourceMemory() final override
//		{
//
//		}
//
//	private:
//		D3DDescriptorHandle m_SRVHandle;
//		
//	};
//
//	typedef D3DStructuredBuffer<uint8_t> D3DBuffer;
//
//	template<typename T>
//	class D3DConstantBuffer : ConstantBuffer<T>
//	{
//		//Inline Constant buffer becomes Root Const and doesnt need to store a data copy
//		struct DataHandle
//		{
//			union {
//				struct
//				{
//					D3DAllocWrappers::D3DMemoryHandle<FrameResource> m_Allocation;
//				} m_Allocations;
//
//				T m_InlineData;
//			};
//		};
//
//	private:
//		DataHandle m_Resource;
//
//		//DescriptorHandles;
//
//
//	public:
//		D3DConstantBuffer(const std::wstring& debugName, const BufferDesc& bufferDesc) : ConstantBuffer<T>(debugName, bufferDesc)
//		{
//			SKTBD_ASSERT(AccessFlags & BufferAccessFlag_::BufferAccessFlag_InlinedConstantBuffer ^ AccessFlags & BufferAccessFlag_::BufferAccessFlag_GpuWrite, "Const inlined buffer cant be written to by the gpu")
//
//			D3D12_RESOURCE_DESC1 resourceDesc = {};
//
//			if (AccessFlags & BufferAccessFlag_::BufferAccessFlag_InlinedConstantBuffer)
//			{
//				if (bufferDesc.pInitialDataToTransfer)
//				{
//					memcpy(&m_Resource.m_InlineData, bufferDesc.pInitialDataToTransfer, sizeof(T));
//				}
//			}
//			else
//			{
//				//cant do subresources would need to be 3 resources BLEH
//				resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
//				resourceDesc.Alignment = D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT;
//				resourceDesc.Width = this->Width;
//				resourceDesc.Height = resourceDesc.DepthOrArraySize = resourceDesc.MipLevels = 1;
//				resourceDesc.Format = DXGI_FORMAT_UNKNOWN;
//				resourceDesc.SampleDesc.Count = 1;
//				resourceDesc.SampleDesc.Quality = 0;
//				resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
//				resourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
//
//				if (this->AccessFlags & BufferAccessFlag_GpuWrite && this->AccessFlags & BufferAccessFlag_CpuWrite)
//				{
//					m_Resource.m_Allocations.m_DefaultResource = D3DAllocWrappers::Allocate<D3D12_HEAP_TYPE_DEFAULT, FrameResource>(resourceDesc);
//					m_Resource.m_Allocations.m_UploadResource = D3DAllocWrappers::Allocate<D3D12_HEAP_TYPE_UPLOAD, FrameResource>(resourceDesc);
//				}
//				else if (this->AccessFlags & BufferAccessFlag_CpuWrite)
//				{
//					m_Resource.m_Allocations.m_UploadResource = D3DAllocWrappers::Allocate<D3D12_HEAP_TYPE_UPLOAD, FrameResource>(resourceDesc);
//				}
//				else if (this->AccessFlags & BufferAccessFlag_GpuWrite)
//				{
//					m_Resource.m_Allocations.m_DefaultResource = D3DAllocWrappers::Allocate<D3D12_HEAP_TYPE_DEFAULT, FrameResource>(resourceDesc);
//				}
//
//				if (this->AccessFlags & BufferAccessFlag_CpuRead)
//				{
//					m_Resource.m_Allocations.m_ReadbackResource = D3DAllocWrappers::Allocate<D3D12_HEAP_TYPE_READBACK, FrameResource>(resourceDesc);
//				}
//			}
//			
//		}
//
//		~D3DConstantBuffer()
//		{
//			if (!(AccessFlags & BufferAccessFlag_::BufferAccessFlag_InlinedConstantBuffer))
//			{
//			
//			}
//		}
//
//		GPUSubresource GetSubresource(uint32_t subresourceIndex) final override
//		{
//		}
//
//		void CopyData(uint32_t subresourceIndex, size_t offt, size_t elem_count, const void* pData) final override
//		{
//			SKTBD_ASSERT(false, "Cant copy arrays on Constant buffers, they contain only one object!")
//		}
//
//		void CopyData(uint32_t subresourceIndex, size_t elementIndex, const void* pData) final override
//		{
//			if (this->AccessFlags & BufferAccessFlag_::BufferAccessFlag_InlinedConstantBuffer)
//			{
//				memcpy(&m_Resource.m_InlineData, pData, sizeof(T));
//			}
//			else
//			{
//				if (this->AccessFlags & BufferAccessFlag_GpuWrite && this->AccessFlags & BufferAccessFlag_CpuWrite)
//				{
//
//					m_Resource.m_Allocations.m_UploadResource.data[gD3DContext->GetCurrentFrameResourceIndex() % FrameResource]->GetResource()->Map()
//					m_Resource.m_Allocations.m_DefaultResource 
//					
//				}
//				else if (this->AccessFlags & BufferAccessFlag_CpuWrite)
//				{
//				//	m_Resource.m_Allocations.m_UploadResource = D3DAllocWrappers::Allocate<D3D12_HEAP_TYPE_UPLOAD, FrameResource>(resourceDesc);
//				}
//				else if (this->AccessFlags & BufferAccessFlag_GpuWrite)
//				{
//				//	m_Resource.m_Allocations.m_DefaultResource = D3DAllocWrappers::Allocate<D3D12_HEAP_TYPE_DEFAULT, FrameResource>(resourceDesc);
//				}
//
//				if (this->AccessFlags & BufferAccessFlag_CpuRead)
//				{
//				//	m_Resource.m_Allocations.m_ReadbackResource = D3DAllocWrappers::Allocate<D3D12_HEAP_TYPE_READBACK, FrameResource>(resourceDesc);
//				}
//			}
//		}
//		void Release() final override
//		{
//
//		}
//
//		const void* GetDefaultDescriptor() final override
//		{
//
//		}
//
//		const void* GetResourceMemory() final override
//		{
//
//		}
//	};
//
//	/*class D3DTexture final : public Texture
//	{
//	public:
//		D3DTexture(const std::wstring& debugName, const TextureDesc& desc);
//
//		virtual ~D3DTexture() final override;
//
//		void CreateSRV();
//
//		D3D12_GPU_VIRTUAL_ADDRESS GetResourceGPUVirtualAddress() const { return m_Buffer->GetGPUVirtualAddress(); };
//		ID3D12Resource* GetResource() const { return m_Buffer.Get(); }
//		ID3D12Resource* GetIntermediateResource() const { return m_IntermediateBuffer.Get(); }
//
//		Microsoft::WRL::ComPtr<ID3D12Resource>& GetResourceComPtr() { return m_Buffer; }
//		Microsoft::WRL::ComPtr<ID3D12Resource>& GetIntermediateResourceComPtr() { return m_IntermediateBuffer; }
//
//		D3DDescriptorHandle GetSRVHandle() const { return m_SRVHandle; }
//
//		void Release() final override;
//	private:
//		Microsoft::WRL::ComPtr<ID3D12Resource> m_Buffer, m_IntermediateBuffer;
//		D3DDescriptorHandle m_SRVHandle;
//	};
//
//	class D3DIndexBuffer final : public IndexBuffer
//	{
//	public:
//		D3DIndexBuffer(const std::wstring& debugName, uint32_t* indices, uint32_t count);
//		
//		virtual ~D3DIndexBuffer() final override;
//		virtual void Bind() const final override;
//		virtual void Unbind() const final override {};
//
//		const D3D12_INDEX_BUFFER_VIEW& GetView() const { return m_IndexBufferView; }
//
//		virtual D3DBuffer* GetBuffer() const final override { return m_IndexBuffer.get(); }
//
//		void Release() final override;
//	private:
//		std::unique_ptr<D3DBuffer> m_IndexBuffer;
//		D3D12_INDEX_BUFFER_VIEW m_IndexBufferView;
//	};
//
//	class D3DVertexBuffer final : public VertexBuffer
//	{
//	public:
//		D3DVertexBuffer(const std::wstring& debugName, void* vertices, uint32_t count, const BufferLayout& layout);
//
//		virtual ~D3DVertexBuffer() final override;
//
//		virtual void Bind() const final override;
//		virtual void Unbind() const final override {};
//
//		virtual D3DBuffer* GetBuffer() const final override { return m_VertexBuffer.get(); }
//
//		const D3D12_VERTEX_BUFFER_VIEW& GetView() const { return m_VertexBufferView; }
//
//		void Release() final override;
//	private:
//		std::unique_ptr<D3DBuffer> m_VertexBuffer;
//		D3D12_VERTEX_BUFFER_VIEW m_VertexBufferView;
//	};
//
//	class D3DFrameBuffer final : public FrameBuffer
//	{
//	public:
//		D3DFrameBuffer(const std::wstring& debugName, const FrameBufferDesc& desc);
//		virtual ~D3DFrameBuffer() final override;
//
//		virtual void Bind(uint32_t renderTargetIndex) const final override;
//		virtual void Unbind() const final override;
//
//
//		virtual void Resize(uint32_t newWidth, uint32_t newHeight);
//		virtual float GetAspectRatio() const { return static_cast<float>(m_Desc.Width) / m_Desc.Height; }
//
//		ID3D12Resource* GetResource() const { return m_RTResources.Get(); }
//		virtual void* GetRenderTargetAsImGuiTextureID(uint32_t renderTargetIndex) const final override;
//		virtual void* GetDepthStencilTargetAsImGuiTextureID(uint32_t renderTargetIndex) const final override;
//
//		const D3DDescriptorHandle GetSRVHandle(uint32_t colourBufferIndex) const { return m_CPUSRVHandle + colourBufferIndex * m_SRVDescriptorSize; }
//		const D3DDescriptorHandle GetRTVHandle(uint32_t colourBufferIndex) const { return m_RTVHandle + colourBufferIndex * m_RTVDescriptorSize; }
//		const D3DDescriptorHandle GetDSVHandle() const { return m_DSVHandle; }
//
//		void Release() final override;
//	private:
//		inline void BuildColourRenderTargets();
//		inline void BuildDepthStencilTargets();
//		inline void BuildDescriptors(bool allocate);
//
//	private:
//		D3D12_VIEWPORT m_ViewPort;
//		D3D12_RECT m_ScissorRect;
//		Microsoft::WRL::ComPtr<ID3D12Resource> m_RTResources;
//		Microsoft::WRL::ComPtr<ID3D12Resource> m_DSResource;
//		D3DDescriptorHandle m_CPUSRVHandle;
//		D3DDescriptorHandle m_GPUSRVHandle;
//		D3DDescriptorHandle m_RTVHandle;
//		D3DDescriptorHandle m_DSVHandle;
//		uint32_t m_SRVDescriptorSize;
//		uint32_t m_RTVDescriptorSize;
//	};*/
//
//	class D3DBottomLevelAccelerationStructure final : public BottomLevelAccelerationStructure
//	{
//	public:
//		D3DBottomLevelAccelerationStructure(const std::wstring& debugName, const BottomLevelAccelerationStructureDesc& desc);
//		virtual ~D3DBottomLevelAccelerationStructure() final override;
//
//		D3D12_GPU_VIRTUAL_ADDRESS GetGPUVirtualAddress() const { return m_Result->GetGPUVirtualAddress(); }
//		void Release() final override;
//	private:
//		Microsoft::WRL::ComPtr<ID3D12Resource> m_Scratch;			// Scratch memory used to build the acceleration structure
//		Microsoft::WRL::ComPtr<ID3D12Resource> m_Result;			// The result of the building of the acceleration structure
//	};
//
//	class D3DTopLevelAccelerationStructure final : public TopLevelAccelerationStructure
//	{
//	public:
//		D3DTopLevelAccelerationStructure(const std::wstring& debugName, const TopLevelAccelerationStructureDesc& desc);
//		virtual ~D3DTopLevelAccelerationStructure() final override;
//
//		virtual void PerformUpdate(uint32_t instanceIndex, const float4x4& transform) final override;
//
//		D3D12_GPU_VIRTUAL_ADDRESS GetGPUVirtualAddress() const { return m_Result->GetGPUVirtualAddress(); }
//		D3DDescriptorHandle GetSRVHandle() const { return m_SRVHandle; }
//		void Release() final override;
//	private:
//		Microsoft::WRL::ComPtr<ID3D12Resource> m_Scratch;			// Scratch memory used to build the acceleration structure
//		Microsoft::WRL::ComPtr<ID3D12Resource> m_Result;			// The result of the building of the acceleration structure
//		Microsoft::WRL::ComPtr<ID3D12Resource> m_Instance;			// The constant buffer of the instance
//		D3DDescriptorHandle m_SRVHandle;
//	};
//}