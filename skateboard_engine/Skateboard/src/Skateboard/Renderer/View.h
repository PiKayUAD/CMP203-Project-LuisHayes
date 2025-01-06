#pragma once
#include "sktbdpch.h"
#include "Skateboard/Renderer/GPUResource.h"

namespace Skateboard
{
	enum BufferType_
	{
		BufferType_ConstantBuffer,
		BufferType_ByteBuffer,
		BufferType_StructureBuffer,
		BufferType_FormattedBuffer,
	};

	enum BufferViewFlags_
	{
		BufferViewFlags_NONE,
		BufferViewFlags_AppendConsumeBuffer = 1,
	};

	struct BufferViewDesc
	{
		BufferType_ Type;
		uint64_t Offset;
		uint32_t ElementSize;
		uint32_t ElementCount;
		DataFormat_ Format;
		BufferViewFlags_ Flags;

		template<typename T>
		void InitAsStructuredBuffer(uint32_t offset, uint32_t elementCount, bool appendconsume = false) {
			Type = BufferType_StructureBuffer;
			Offset = offset;
			ElementCount = elementCount;
			ElementSize = sizeof(T);
			Format = DataFormat_UNKNOWN;
			Flags = (appendconsume) ? BufferViewFlags_AppendConsumeBuffer : BufferViewFlags_NONE;
		}

		template<typename T>
		void InitAsConstantBuffer(uint32_t offset) {
			Type = BufferType_ConstantBuffer;
			Offset = ROUND_UP(offset, GraphicsConstants::CONSTANT_BUFFER_ALIGNMENT);
			ElementCount = 1;
			ElementSize = ROUND_UP(sizeof(T), GraphicsConstants::CONSTANT_BUFFER_ALIGNMENT);
			Format = DataFormat_UNKNOWN;
			Flags = BufferViewFlags_NONE;
		}

		void InitAsByteBuffer(uint32_t offset, uint32_t elementCount) {
			Type = BufferType_ByteBuffer;
			Offset = offset;
			ElementCount = elementCount;
			ElementSize = 1;
			Format = DataFormat_UNKNOWN;
			Flags = BufferViewFlags_NONE;
		}

		void InitAsFormatedBuffer(uint32_t offset, uint32_t elementCount,  DataFormat_ format) {
			Type = BufferType_ByteBuffer;
			Offset = offset;
			ElementCount = elementCount;
			ElementSize = 0;
			Format = format;
			Flags = BufferViewFlags_NONE;
		}
	};

	struct TextureViewDesc
	{
		TextureDimension_ Dimension;
		DataFormat_ Format;
		uint32_t MostDetailedMip;
		uint32_t MipLevels;
		float ResourceMinLodClamp;

		//for array
		uint32_t  FirstArraySlice;
		uint32_t  ArraySize;
		uint32_t  PlaneSlice;

		//for uav
		uint32_t MipSlice;
	};

	struct RenderTargetDesc
	{
		TextureDimension_ Dimension;
		DataFormat_ Format;
		uint32_t MostDetailedMip;
		uint32_t MipLevels;
		float ResourceMinLodClamp;

		uint32_t MipSlice;

		//for array
		uint32_t  FirstArraySlice;
		uint32_t  ArraySize;
		uint32_t  PlaneSlice;

		uint32_t  FirstWSlice;
		uint32_t  Width;

		union
		{
			glm::int4 DepthClearValue;
			glm::vec4 RenderClearValue;
		};
	};

	enum DRT_Flags_
	{
		DRT_Flags_ReadOnly_Depth = 0,
		DRT_Flags_ReadOnly_Stencil = 1,
	};

	struct DepthStencilDesc
	{
		TextureDimension_ Dimension;
		DataFormat_ Format;
		uint32_t MipLevels;
		float ResourceMinLodClamp;

		uint32_t MipSlice;

		//for array
		uint32_t  FirstArraySlice;
		uint32_t  ArraySize;
		uint32_t  PlaneSlice;

		DRT_Flags_ Flags;
	};

	class ShaderView : public GPUResource
	{
	public:
		virtual ~ShaderView() {};
		ViewAccessType_ GetViewType() const { return m_Access; };
		virtual MemoryResource* GetMemoryResource() const = 0;
		virtual uint64_t GetOffset() const = 0;
	protected:
		ShaderView() : GPUResource(GpuResourceType_ShaderView) {};
		ShaderView(const std::wstring& debugName, ViewAccessType_ accessType) : m_Access(accessType), GPUResource(debugName, GpuResourceType_ShaderView)
		{
		};
		
		ViewAccessType_ m_Access;
	};
	
	class BufferView : public ShaderView
	{
	public:
		virtual ~BufferView() {};
	
		BufferType_ GetBufferViewType() const { return Type; };
		uint32_t GetSize() const { return Size; };
		uint64_t GetOffset() const { return Offset; }
		virtual MemoryResource* GetMemoryResource() const { return m_ParentResource; };

	protected:
		BufferView() : ShaderView() {}
		BufferView(const std::wstring& debugName, const BufferViewDesc& desc, ViewAccessType_ access) : ShaderView(debugName, access)
		{
			Type = desc.Type;
			Offset = desc.Offset;
			Size = desc.ElementSize * desc.ElementCount;
		}

		BufferType_ Type;
		uint64_t Offset;
		uint32_t Size;

		Buffer* m_ParentResource;
	};

	class TextureView : public ShaderView
	{
	protected:
		TextureView() : ShaderView() {};

		TextureView(const std::wstring& debugName, ViewAccessType_ Access, TextureViewDesc* desc) : ShaderView(debugName, Access) {};

		TextureView(const std::wstring& debugName, ViewAccessType_ Access, const TextureViewDesc& desc) : ShaderView(debugName, Access)
		{
			Format = desc.Format;
			Dimension = desc.Dimension;
			MostDetailedMip = desc.MostDetailedMip;
			ResourceMinLodClamp = desc.ResourceMinLodClamp;
			MipSlice = desc.MipSlice;
			FirstArraySlice = desc.FirstArraySlice;
			ArraySize = desc.ArraySize;
			PlaneSlice = desc.PlaneSlice;
		};

	public:
		uint64_t GetWidth()    const { return (m_ParentResource) ? m_ParentResource->GetWidth()    : 0; }
		uint32_t GetDepth()    const { return (m_ParentResource) ? m_ParentResource->GetDepth()    : 0; }
		uint32_t GetHeight()   const { return (m_ParentResource) ? m_ParentResource->GetHeight()   : 0; }
		uint32_t GetMipCount() const { return (m_ParentResource) ? m_ParentResource->GetMipCount() : 0; }

		virtual MemoryResource* GetMemoryResource() const { return m_ParentResource; };
		virtual uint64_t GetOffset() const { return 0; };
	protected:
		TextureDimension_ Dimension;
		DataFormat_ Format;
		uint32_t MostDetailedMip;
		float ResourceMinLodClamp;

		uint32_t MipSlice;

		//for array
		uint32_t  FirstArraySlice;
		uint32_t  ArraySize;
		uint32_t  PlaneSlice;

		TextureBuffer* m_ParentResource;
	};

	class RenderTargetView : public GPUResource
	{
	public:
		virtual ~RenderTargetView() {}

		uint64_t GetWidth()    const { return (m_ParentResource) ? m_ParentResource->GetWidth() : 0; }
		uint32_t GetDepth()    const { return (m_ParentResource) ? m_ParentResource->GetDepth() : 0; }
		uint32_t GetHeight()   const { return (m_ParentResource) ? m_ParentResource->GetHeight() : 0; }
		uint32_t GetMipCount() const { return (m_ParentResource) ? m_ParentResource->GetMipCount() : 0; }
		const DataFormat_ GetFormat() const { return Format; }
		const glm::vec4 GetClearValue() const { return (m_ParentResource) ? m_ParentResource->GetClearValue() : GRAPHICS_FRAMEBUFFER_DEFAULT_CLEAR_COLOUR; }

	protected:
		RenderTargetView() : GPUResource(GPUResourceType_::GpuResourceType_RenderTargetView) {};
		RenderTargetView(const std::wstring& name, RenderTargetDesc* desc) : GPUResource(name,GPUResourceType_::GpuResourceType_RenderTargetView)
		{
			if (desc)
			{
				Format = desc->Format;
			};
		}
	protected:
		DataFormat_ Format;
		TextureBuffer* m_ParentResource;
	};

	class DepthStencilView : public GPUResource
	{
	public:
		virtual ~DepthStencilView() {}

		const DataFormat_ GetFormat() const { return Format; };
		const DRT_Flags_ GetFlags() const { return Flags; };

		const float GetDepthClearValue() const { return(m_ParentResource) ? m_ParentResource->DepthClearValue() : GRAPHICS_DEPTH_DEFAULT_CLEAR_COLOUR; };
		const uint32_t GetStencilClearValue() const { return(m_ParentResource) ? m_ParentResource->StencilClearValue() : GRAPHICS_STENCIL_DEFAULT_CLEAR_COLOUR; };
	
		
		uint64_t GetWidth()    const { return (m_ParentResource) ? m_ParentResource->GetWidth() : 0; }
		uint32_t GetDepth()    const { return (m_ParentResource) ? m_ParentResource->GetDepth() : 0; }
		uint32_t GetHeight()   const { return (m_ParentResource) ? m_ParentResource->GetHeight() : 0; }
		uint32_t GetMipCount() const { return (m_ParentResource) ? m_ParentResource->GetMipCount() : 0; }


	protected:
		DepthStencilView() : GPUResource(GpuResourceType_DepthStencilView) {};
		DepthStencilView(const std::wstring& name, DepthStencilDesc* desc) : GPUResource(name,GpuResourceType_DepthStencilView)
		{
			if (desc)
			{
				Format = desc->Format;
				Flags = desc->Flags;
			};
		};

		DataFormat_ Format;
		DRT_Flags_ Flags;

		TextureBuffer* m_ParentResource;
	};

	enum IndexFormat
	{
		bit32,
		bit16
	};

	//=========================== Index and Vertex Buffers ====================================
	// as they are a hint for the 

	struct IndexBufferView
	{
		IndexFormat m_Format;
		uint32_t m_Offset;
		uint32_t m_IndexCount;

		Buffer* m_ParentResource;
	};

	enum SKTBD_PRIMITIVE_TOPOLOGY
	{
		SKTBD_PRIMITIVE_TOPOLOGY_UNDEFINED = 0,
		SKTBD_PRIMITIVE_TOPOLOGY_POINTLIST = 1,
		SKTBD_PRIMITIVE_TOPOLOGY_LINELIST = 2,
		SKTBD_PRIMITIVE_TOPOLOGY_LINESTRIP = 3,
		SKTBD_PRIMITIVE_TOPOLOGY_TRIANGLELIST = 4,
		SKTBD_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP = 5,
		SKTBD_PRIMITIVE_TOPOLOGY_TRIANGLEFAN,
		SKTBD_PRIMITIVE_TOPOLOGY_LINELIST_ADJ = 10,
		SKTBD_PRIMITIVE_TOPOLOGY_LINESTRIP_ADJ = 11,
		SKTBD_PRIMITIVE_TOPOLOGY_TRIANGLELIST_ADJ = 12,
		SKTBD_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP_ADJ = 13,
		SKTBD_PRIMITIVE_TOPOLOGY_1_CONTROL_POINT_PATCHLIST = 33,
		SKTBD_PRIMITIVE_TOPOLOGY_2_CONTROL_POINT_PATCHLIST = 34,
		SKTBD_PRIMITIVE_TOPOLOGY_3_CONTROL_POINT_PATCHLIST = 35,
		SKTBD_PRIMITIVE_TOPOLOGY_4_CONTROL_POINT_PATCHLIST = 36,
		SKTBD_PRIMITIVE_TOPOLOGY_5_CONTROL_POINT_PATCHLIST = 37,
		SKTBD_PRIMITIVE_TOPOLOGY_6_CONTROL_POINT_PATCHLIST = 38,
		SKTBD_PRIMITIVE_TOPOLOGY_7_CONTROL_POINT_PATCHLIST = 39,
		SKTBD_PRIMITIVE_TOPOLOGY_8_CONTROL_POINT_PATCHLIST = 40,
		SKTBD_PRIMITIVE_TOPOLOGY_9_CONTROL_POINT_PATCHLIST = 41,
		SKTBD_PRIMITIVE_TOPOLOGY_10_CONTROL_POINT_PATCHLIST = 42,
		SKTBD_PRIMITIVE_TOPOLOGY_11_CONTROL_POINT_PATCHLIST = 43,
		SKTBD_PRIMITIVE_TOPOLOGY_12_CONTROL_POINT_PATCHLIST = 44,
		SKTBD_PRIMITIVE_TOPOLOGY_13_CONTROL_POINT_PATCHLIST = 45,
		SKTBD_PRIMITIVE_TOPOLOGY_14_CONTROL_POINT_PATCHLIST = 46,
		SKTBD_PRIMITIVE_TOPOLOGY_15_CONTROL_POINT_PATCHLIST = 47,
		SKTBD_PRIMITIVE_TOPOLOGY_16_CONTROL_POINT_PATCHLIST = 48,
		SKTBD_PRIMITIVE_TOPOLOGY_17_CONTROL_POINT_PATCHLIST = 49,
		SKTBD_PRIMITIVE_TOPOLOGY_18_CONTROL_POINT_PATCHLIST = 50,
		SKTBD_PRIMITIVE_TOPOLOGY_19_CONTROL_POINT_PATCHLIST = 51,
		SKTBD_PRIMITIVE_TOPOLOGY_20_CONTROL_POINT_PATCHLIST = 52,
		SKTBD_PRIMITIVE_TOPOLOGY_21_CONTROL_POINT_PATCHLIST = 53,
		SKTBD_PRIMITIVE_TOPOLOGY_22_CONTROL_POINT_PATCHLIST = 54,
		SKTBD_PRIMITIVE_TOPOLOGY_23_CONTROL_POINT_PATCHLIST = 55,
		SKTBD_PRIMITIVE_TOPOLOGY_24_CONTROL_POINT_PATCHLIST = 56,
		SKTBD_PRIMITIVE_TOPOLOGY_25_CONTROL_POINT_PATCHLIST = 57,
		SKTBD_PRIMITIVE_TOPOLOGY_26_CONTROL_POINT_PATCHLIST = 58,
		SKTBD_PRIMITIVE_TOPOLOGY_27_CONTROL_POINT_PATCHLIST = 59,
		SKTBD_PRIMITIVE_TOPOLOGY_28_CONTROL_POINT_PATCHLIST = 60,
		SKTBD_PRIMITIVE_TOPOLOGY_29_CONTROL_POINT_PATCHLIST = 61,
		SKTBD_PRIMITIVE_TOPOLOGY_30_CONTROL_POINT_PATCHLIST = 62,
		SKTBD_PRIMITIVE_TOPOLOGY_31_CONTROL_POINT_PATCHLIST = 63,
		SKTBD_PRIMITIVE_TOPOLOGY_32_CONTROL_POINT_PATCHLIST = 64,
	};

	struct VertexBufferView 
	{
		uint32_t m_Offset;
		uint32_t m_VertexCount;
		uint32_t m_VertexStride;

		Buffer* m_ParentResource;
	};

	// =========================== BLAS STRUCTURES ==================================================================================

	struct GeometryTriangleDesc
	{
		VertexBufferView* pVertexBuffer;
		uint32_t StartVertexLocation;
		uint32_t VertexCount;
		IndexBufferView* pIndexBuffer;
		uint32_t StartIndexLocation;
		uint32_t IndexCount;
	};

	struct GeometryAABBDesc
	{
		uint64_t AABBCount;
		struct
		{
			uint64_t Offset;
			Buffer* pProceduralGeometryAABBBuffer;
		}BufferAABB;
	};

	struct BottomLevelAccelerationStructureDesc
	{
		GeometryType_ Type;
		GeometryFlags_ Flags;
		union
		{
			GeometryTriangleDesc Triangles;
			GeometryAABBDesc Procedurals;
		};
	};

	// =========================== BLAS GPU Resource ==================================================================================


	class BottomLevelAccelerationStructureView : public GPUResource 
	{
	protected:
		BottomLevelAccelerationStructureView() : GPUResource(GpuResourceType_ShaderView) {};
		BottomLevelAccelerationStructureView(const std::wstring& debugName, const BottomLevelAccelerationStructureDesc& desc) :
			GPUResource(debugName, GpuResourceType_ShaderView), m_Desc(desc) {}
	public:
		virtual ~BottomLevelAccelerationStructureView() {}

		BottomLevelAccelerationStructureDesc m_Desc;
		Buffer* m_ParentResource;
	};

	// =========================== TLAS Structres ==================================================================================

	struct TopLevelAccelerationStructureDesc
	{
		std::vector<float4x4> vTransforms;
		std::vector<uint32_t> vInstanceIndices;
		std::vector<uint32_t> vMeshIDs;
		std::vector<std::unique_ptr<BottomLevelAccelerationStructureView>>* vBLAS;
	};

	// =========================== TLAS GPU Resource ==================================================================================

	class TopLevelAccelerationStructureView : public GPUResource
	{
	protected:
		TopLevelAccelerationStructureView() : GPUResource(GpuResourceType_ShaderView) {};
		TopLevelAccelerationStructureView(const std::wstring& debugName, const TopLevelAccelerationStructureDesc& desc) :
			GPUResource(debugName, GpuResourceType_ShaderView), m_Desc(desc) {}
	public:
		virtual ~TopLevelAccelerationStructureView() {}

		virtual void PerformUpdate(uint32_t instanceIndex, const float4x4& transform) = 0;

		TopLevelAccelerationStructureDesc m_Desc;
		Buffer* m_ParentResource;
	};
}