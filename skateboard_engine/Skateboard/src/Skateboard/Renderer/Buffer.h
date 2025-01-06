#pragma once

#include "InternalFormats.h"
#include "Skateboard/Mathematics.h"
#include "Skateboard/Scene/AABB.h"
#include "Renderer.h"

#include "GPUResource.h"

namespace Skateboard
{
	struct BufferElement
	{
		std::string SemanticName;		// Name of the semantic used in the shader (careful with different APIs!)
		ShaderDataType_ Type;			// Abstraction of the shader data type of this element
		uint32_t InputSlot;				// The input slot of this element for this semantic
		uint32_t Size;					// The size of the element (obtained automatically)
		uint32_t Offset;				// The offset of the element in the overall layout (obtained automatically)

		BufferElement() = default;
		BufferElement(const std::string& name, ShaderDataType_ type, uint32_t inputSlot = 0) :
			SemanticName(name), Type(type), InputSlot(inputSlot), Size(ShaderDataTypeSizeInBytes(type)), Offset(0)
		{
		}

		/// <summary>
		/// An shader element contains one or multiple components. For instance:
		///		- A float3 contains 3 components (3 floats)
		///		- A int2 contains 2 components (2 ints)
		///		- A float4x4 contains 16 components (4x4 floats)
		/// This function retrieves this information outside of an API context.
		/// </summary>
		/// <returns>Returns the number of components present in this element</returns>
		uint32_t GetComponentCount() const;
	};

	template<typename ... ElementType>
	struct VertexType
	{
		constexpr size_t GetStride() { return sizeof...(ElementType); }
	};

	class BufferLayout
	{
	public:
		BufferLayout() : m_Stride(0u) {};
		BufferLayout(const std::initializer_list<BufferElement>& elements) :
			v_Elements(elements),
			m_Stride(0u)
		{
			CalculateOffsetsAndStride();
		}

		/// <summary>
		/// Function to get the default layout for graphics applicaitons.
		/// Custom layouts can be created using the same pattern used in this function for simpler/more complicated layouts
		/// (for instance when using skinning). For static geometry this layout is perfect as it gives all the necessary
		/// elements to render high quality graphics!
		/// </summary>
		/// <returns>The default BufferLayout for unskinned geometry</returns>
		static BufferLayout GetDefaultLayout()
		{
			return {
				{ "POSITION", ShaderDataType_::Float3 },
				{ "TEXCOORD_0", ShaderDataType_::Float2 },
				{ "NORMAL",   ShaderDataType_::Float3 },
				{ "TANGENT",  ShaderDataType_::Float3 },
				{ "BITANGENT",ShaderDataType_::Float3 }
			};
		}


		/// <summary>
		/// Retrieves the vertex layout input for simple animated skinned mesh. This layout 
		/// can be used across all APIs.
		/// </summary>
		/// <returns>The default BufferLayout for skinned geometry</returns>
		static BufferLayout GetDefaultSkinnedMeshLayout()
		{
			return {
				{ "POSITION",		ShaderDataType_::Float3 },
				{ "TEXCOORD_0",		ShaderDataType_::Float2 },
				{ "NORMAL",			ShaderDataType_::Float3 },
				{ "TANGENT",		ShaderDataType_::Float3 },
				{ "BITANGENT",		ShaderDataType_::Float3 },
				{ "BLENDINDICES",	ShaderDataType_::Int4 },
				{ "BLENDWEIGHTS",	ShaderDataType_::Float4 },
			};
		}

		/// <summary>
		/// Retrieve the different elements in this layout as a vector.
		/// See BufferElement for an overview of what elements contain.
		/// </summary>
		/// <returns></returns>
		const std::vector<BufferElement>& GetElements() const { return v_Elements; }
		/// <summary>
		/// Retrieves the overall stride of this layout, that is how many bytes in total
		/// each vertex occupies in the vertex buffer.
		/// </summary>
		/// <returns>The overall stride, in bytes</returns>
		uint32_t GetStride() const { return m_Stride; }

		constexpr DataFormat_ GetVertexPositionFormat() const { return DataFormat_R32G32B32_FLOAT; }

		// Iterators for nice C++ functionalities
		std::vector<BufferElement>::iterator begin() { return v_Elements.begin(); }
		std::vector<BufferElement>::iterator end() { return v_Elements.end(); }
		std::vector<BufferElement>::const_iterator begin() const { return v_Elements.begin(); }
		std::vector<BufferElement>::const_iterator end() const { return v_Elements.end(); }

	private:
		// This function is called privately to calculate the internal offsets of the different elements
		// as well as the overall stride of the layout. These calculation can only be performed once the
		// layout contains all the elements that define it.
		void CalculateOffsetsAndStride();


	private:
		std::vector<BufferElement> v_Elements;
		uint32_t m_Stride;
	};

	/*typedef struct {
		DataFormat_ Format;
		bool CubeMapTarget;
		union
		{
			float4 ClearColour;
			struct {
				float Depth;
				uint8_t Stencil;
			} DepthStencil;
		};
	} RenderTargetDesc, DepthStencilTargetDesc;

	struct FrameBufferDesc
	{
		uint32_t Width, Height;
		RenderTargetDesc RenderTarget;
		DepthStencilTargetDesc DepthStencilTarget;
		uint32_t NumRenderTargets = 1u;

		uint32_t DepthFormat;
		uint32_t Compression;*/

		
	//};

	//struct IndirectArgumentBufferDesc
	//{
	//	uint32_t BaseVertexLocation;;
	//	uint32_t IndexCountPerInstance;
	//	uint32_t InstanceCount;
	//	uint32_t StartIndexLocation;
	//	uint32_t StartInstanceLocation;
	//};
	// 
	//class IndirectArgumentBuffer : public GPUResource //, public ResourceDesc<IndirectArgumentBufferDesc>
	//{
	//protected:
	//	IndirectArgumentBuffer(const std::wstring& debugName, const IndirectArgumentBufferDesc desc) : GPUResource(debugName, GPUResourceType_IndirectArgsBuffer)
	//	{};
	//public:
	//	static IndirectArgumentBuffer* Create(const std::wstring& debugName, const IndirectArgumentBufferDesc& desc);
	//	virtual void* GetIndirectArgumentBuffer() = 0;
	//};

	struct BufferDesc
	{
		//ResourceAccessFlags
		ResourceAccessFlag_ AccessFlags;
		BufferFlags_ Flags;
		uint32_t Size;
		
		void* pInitialDataToTransfer;

		void Init(uint32_t SizeInBytes, ResourceAccessFlag_ accessFlags = ResourceAccessFlag_CpuWrite | ResourceAccessFlag_GpuRead | ResourceAccessFlag_DesktopPlatformPrimaryResidenceGPU, BufferFlags_ bufferFlags = BufferFlags_NONE, void* pDataToTransfer = nullptr)
		{
			Size = SizeInBytes;
			AccessFlags = accessFlags;
			Flags = bufferFlags;
			pInitialDataToTransfer = pDataToTransfer;
		};
	};

	class MemoryResource : public GPUResource
	{
	public :
		uint32_t GetAccessFlags() { return AccessFlags; };
	protected:

		template<typename ... Args>
		MemoryResource(Args&&... args) : GPUResource(std::forward<Args>(args)...) {}

		ResourceAccessFlag_ AccessFlags;
	};

	class Buffer : public MemoryResource //, public CPUInterface
	{
	public:
		virtual ~Buffer() {};
		uint64_t Size;
		uint32_t Flags;

	protected:
		Buffer() : MemoryResource(GpuResourceType_Buffer) {};
		Buffer(const std::wstring& debugName, const BufferDesc& desc) : MemoryResource(debugName, GpuResourceType_Buffer)
		{
			Size = desc.Size;
			Flags = desc.Flags;
			AccessFlags = desc.AccessFlags;
		};
	};

	union ClearValue
	{
		glm::vec4 RenderClearValue;
		struct
		{
			float Depth;
			uint32_t Stencil;
		};
	};

	struct TextureDesc
	{
		DataFormat_ Format;
		ResourceAccessFlag_ AccessFlags;
		TextureType_ Type;
		TextureDimension_ Dimension;

		uint64_t Width;
		uint32_t Height;
		uint32_t Depth;
		uint32_t Mips;

		ClearValue Clear;
	};

	class TextureBuffer : public MemoryResource
	{
	protected:
		TextureBuffer() : MemoryResource(GpuResourceType_Texture) {};
		TextureBuffer(const std::wstring& debugName, const TextureDesc& desc) : MemoryResource(debugName, GpuResourceType_Texture) // , ResourceDesc<TextureDesc>(desc)
		{
			Format = desc.Format;
			AccessFlags = desc.AccessFlags;
			Type = desc.Type;
			Dimension = desc.Dimension;

			Width = desc.Width;
			Height = desc.Height;
			Depth = desc.Depth;
			Mips = desc.Mips;
			Clear = desc.Clear;
		};

		DataFormat_ Format;
		uint32_t AccessFlags;
		TextureType_ Type;
		TextureDimension_ Dimension;

		ClearValue Clear;
		
		uint64_t Width;
		uint32_t Height;
		uint32_t Depth;
		uint32_t Mips;

	public:
		virtual ~TextureBuffer() {};

		uint64_t GetWidth() const	{ return Width;};
		uint32_t GetHeight() const	{ return Height;};
		uint32_t GetDepth() const	{ return Depth;};
		uint32_t GetMipCount() const { return Mips; };
		glm::vec4 GetClearValue() const { return Clear.RenderClearValue; };
		float DepthClearValue() const { return Clear.Depth; };
		uint32_t StencilClearValue() const { return Clear.Stencil; };
	};

	// A render target could be a colour buffer or a depth/stencil buffer

	//class FrameBuffer : public GPUResource //, public ResourceDesc<FrameBufferDesc>
	//{
	//	DISABLE_C_M_D(FrameBuffer)
	//protected:
	//	FrameBuffer(const std::wstring& debugName, const FrameBufferDesc& desc) : GPUResource(debugName, GPUResourceType_FrameBuffer) , m_Desc(desc)//, ResourceDesc<FrameBufferDesc>(desc)
	//	{};

	//public:
	//	virtual ~FrameBuffer() {}

	//	static FrameBuffer* Create(const std::wstring& debugName, const FrameBufferDesc& desc);

	//	virtual void Bind(uint32_t renderTargetIndex = 0u) const = 0;
	//	virtual void Unbind() const = 0;

	//	virtual void Resize(uint32_t newWidth, uint32_t newHeight) = 0;
	//	virtual float GetAspectRatio() const = 0;
	//	//virtual void ClearRenderTarget(uint32_t renderTargetIndex, uint32_t value) = 0;

	//	virtual void* GetRenderTargetAsImGuiTextureID(uint32_t renderTargetIndex = 0u) const = 0;
	//	virtual void* GetDepthStencilTargetAsImGuiTextureID(uint32_t depthTargetIndex = 0u) const = 0;

	//protected:
	//	FrameBufferDesc m_Desc;
	//};

	
}
