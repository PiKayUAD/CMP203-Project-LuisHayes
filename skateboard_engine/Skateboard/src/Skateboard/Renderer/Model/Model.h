#pragma once
#include <map>
#include "Span.h"
#include "Skateboard/Renderer/CommonResources.h"
#include "Skateboard/Renderer/Materials/Material.h"
#include "Skateboard/Mathematics.h"
#include "Skateboard/Memory/VirtualAllocator.h"

using namespace Skateboard::MemoryUtils;

namespace Skateboard
{
    using Texture = SingleResource<TextureView>;

    struct MaterialData
    {
        float4 Albedo;
        int AlbedoMapIndex;
        int3 padding;
        float3 Fresnel;
        float Metallic;
        float3 Specular;
        float Roughness;
    };

	class PerspectiveCamera;

    #define CULL_FLAG 0x1
    #define MESHLET_FLAG 0x2

    struct Attribute
    {
        enum EType : uint32_t
        {
            Position,
            Normal,
            TexCoord,
            Tangent,
            Bitangent,
            Count
        };

        EType    Type;
        uint32_t Offset;
    };

    struct Subset
    {
        uint32_t Offset;
        uint32_t Count;
    };

    __declspec(align(256u))
    struct MeshInfo
    {
        uint32_t IndexSize;
        uint32_t MeshletCount;

        uint32_t LastMeshletVertCount;
        uint32_t LastMeshletPrimCount;
    };

    struct Meshlet
    {
        uint32_t VertCount;
        uint32_t VertOffset;
        uint32_t PrimCount;
        uint32_t PrimOffset;
    };

    struct PackedTriangle
    {
        uint32_t i0 : 10;
        uint32_t i1 : 10;
        uint32_t i2 : 10;
    };

    struct CullData
    {
        float4  BoundingSphere;   // xyz = center, w = radius
        uint8_t NormalCone[4];    // xyz = axis, w = -cos(a + 90)
        float   ApexOffset;       // apex = center - axis * offset
    };

    __declspec(align(256u))
	struct MeshletCullPass
    {
        float4x4 View;
        float4x4 ViewProj;
        float4 Planes[6];

        float3 ViewPosition;
        uint32_t HighlightedIndex;

        float3 CullViewPosition;
        uint32_t SelectedIndex;

        uint32_t DrawMeshlets;
    };

    struct MeshInstance
    {
        float4x4 WorldTransform;
        float4x4 WorldInvTransform;
        float Scale;
        uint32_t Flags;
    };

    struct PrimitiveSuballocationSingleBuffer
    {
        BlockAllocator* ParentAllocator;
        VirtualAllocation PrimitiveData;

        //Release the Allocations on Deletion of the Primitive
        ~PrimitiveSuballocationSingleBuffer()
        {
            if (ParentAllocator)
            {
                ParentAllocator->FreeAllocation(PrimitiveData);
            }
        }
    };

    struct PrimitiveBuffers
    {
        SingleResource<Buffer> VertexBuffer;
        SingleResource<Buffer> IndexBuffer;
    };


    struct Primitive
    {
        Primitive()
            :
                Layout(BufferLayout::GetDefaultLayout())
//            ,   Vertices()
//            ,   VertexCount(0)
//            ,   IndexSubsets()
//            ,   Indices()
//            ,   IndexSize(0)
//            ,   IndexCount(0)
            , VertexBuffer()
            , IndexBuffer()
            ,   TextureIndices(0)
        {}
        Primitive(const Primitive& rhs)
            :
                Layout(rhs.Layout)
 //           ,   Vertices(rhs.Vertices)
 //           ,   VertexCount(rhs.VertexCount)
 //           ,   IndexSubsets(rhs.IndexSubsets)
 //           ,   Indices(rhs.Indices)
 //           ,   IndexSize(rhs.IndexSize)
 //           ,   IndexCount(rhs.IndexCount)
            ,   VertexBuffer(rhs.VertexBuffer)
            ,   IndexBuffer(rhs.IndexBuffer)
            ,   TextureIndices(rhs.TextureIndices)
        {}
        auto operator=(const Primitive& rhs) noexcept -> Primitive&{
            Layout=rhs.Layout;
 //           Vertices=rhs.Vertices;
 //           VertexCount=rhs.VertexCount;
 //           IndexSubsets=rhs.IndexSubsets;
 //           Indices=rhs.Indices;
 //           IndexSize=rhs.IndexSize;
 //           IndexCount=rhs.IndexCount;
            VertexBuffer =rhs.VertexBuffer;
            IndexBuffer =rhs.IndexBuffer;
            TextureIndices =rhs.TextureIndices;
            return *this;
        }

        Primitive(Primitive&& rhs) noexcept
            :
                Layout(rhs.Layout)
 //           ,   Vertices(std::move(rhs.Vertices))
 //           ,   VertexCount(rhs.VertexCount)
 //           ,   IndexSubsets(std::move(rhs.IndexSubsets))
 //           ,   Indices(std::move(rhs.Indices))
 //           ,   IndexSize(rhs.IndexSize)
 //           ,   IndexCount(rhs.IndexCount)
            ,   VertexBuffer(std::move(rhs.VertexBuffer))
            ,   IndexBuffer(std::move(rhs.IndexBuffer))
            ,   TextureIndices(rhs.TextureIndices)
        {}
        auto operator=(Primitive&& rhs) noexcept -> Primitive& {
            Layout = rhs.Layout;
//            Vertices = std::move(rhs.Vertices);
//            VertexCount = rhs.VertexCount;
//            IndexSubsets = std::move(rhs.IndexSubsets);
//            Indices = std::move(rhs.Indices);
//            IndexSize = rhs.IndexSize;
//            IndexCount = rhs.IndexCount;
            VertexBuffer = std::move(rhs.VertexBuffer);
            IndexBuffer = std::move(rhs.IndexBuffer);
            TextureIndices = std::move(rhs.TextureIndices);
            return *this;
        }

        BufferLayout Layout;                        // Vertex data layout

//        Span<uint8_t>              Vertices;        // Raw vertices
//        size_t                   VertexCount;

//        Span<Subset>               IndexSubsets;
//        Span<uint8_t>              Indices;         // Bytes
//        size_t                     IndexSize;       // So we store the size of an index (2 or 4 bytes)
//        size_t                     IndexCount;

        std::variant<std::shared_ptr<PrimitiveSuballocationSingleBuffer>, PrimitiveBuffers> Allocations;


        VertexBufferView VertexBuffer;
        IndexBufferView IndexBuffer;
        std::vector<uint32_t> TextureIndices;
    };

	class Mesh
	{
	public:
        virtual ~Mesh() = default;
        Mesh() = default;
        explicit Mesh(const wchar_t* filename);
        explicit Mesh(const std::vector<Primitive>& meshes);

    //    void AddPrimitive(Primitive& prim) { m_Primitives.push_back(prim); }
    //    void SetPrimitive(Primitive& prim, uint32_t element);
    //    void SetPrimitive(Primitive& prim, const std::string_view& meshTag);

    //  void AddVertexStride(uint32_t stride) { m_VertexStrides.push_back(stride); }

    //  uint32_t GetMeshVertexStride(uint32_t element) const { return m_VertexStrides[element]; }
    //  const std::vector<uint32_t>& GetVertexStrides() const { return m_VertexStrides; }

        VertexBufferView* GetVertexBuffer(uint32_t element) { return &m_Primitives[element].VertexBuffer; };
    //  uint8_t* GetVertexData(uint32_t element) { return m_Primitives[element].Vertices.data(); }

        IndexBufferView* GetIndexBuffer(uint32_t element) { return &m_Primitives[element].IndexBuffer; };
    //  uint8_t* GetIndexData(uint32_t element) { return m_Primitives[element].Indices.data(); }

        VertexBufferView* GetVertexBuffer(const std::string_view& meshTag)  { return &m_Primitives[m_Tag2Element[meshTag]].VertexBuffer; };
    //  uint8_t* GetVertexData(const std::string_view& meshTag) { return m_Primitives[m_Tag2Element[meshTag]].Vertices.data(); }

        IndexBufferView* GetIndexBuffer(const std::string_view& meshTag)    { return &m_Primitives[m_Tag2Element[meshTag]].IndexBuffer;  };
    //  uint8_t* GetIndexData(const std::string_view& meshTag) { return m_Primitives[m_Tag2Element[meshTag]].Indices.data(); }

        Primitive* GetMesh(uint32_t meshElement) { return &m_Primitives[meshElement]; }
        const Primitive* GetMesh(uint32_t meshElement) const {return &m_Primitives[meshElement]; }

        Primitive* GetMesh(const std::string_view& meshTag) { return &m_Primitives[m_Tag2Element[meshTag]]; }

        uint32_t GetMeshCount() const { return static_cast<uint32_t>(m_Primitives.size()); }
        uint32_t GetTextureCount() const { return static_cast<uint32_t>(m_Textures.size()); }

        Texture* GetTexture(uint32_t textureIndex) {  return m_Textures[textureIndex]; }
        void     AddTexture(Texture* texture) { m_Textures.push_back(texture); }
        void     SetTexture(Texture* texture) { if(m_Textures.size()>0) m_Textures.back() = texture; }


        std::vector<Primitive>::iterator begin() { return m_Primitives.begin(); }
        std::vector<Primitive>::iterator end()   { return m_Primitives.end(); }

    public:
        Transform Offset;
    protected:
    //    std::vector<uint32_t>      m_VertexStrides;   // Stride of each vertex buffer
        std::vector<Texture*>      m_Textures;        // Textures used by the model
        std::vector<Primitive>     m_Primitives;
        std::map<std::string_view, uint32_t> m_Tag2Element;
        
	};

	class MeshletModel : public Mesh
	{
	public:
        virtual void SetInputLayout(const BufferLayout& inputLayout, const std::string_view& meshTag) = 0;


		static MeshletModel* Create(const wchar_t* filename);

		virtual GPUResource* GetMeshletBuffer(const std::wstring_view& meshTag)         = 0;
		virtual GPUResource* GetUniqueVertexIndices(const std::wstring_view& meshTag)   = 0;
		virtual GPUResource* GetPrimitiveIndices(const std::wstring_view& meshTag)      = 0;
        virtual GPUResource* GetMeshInfo(const std::wstring_view& meshTag)              = 0;

		virtual const std::vector<Span<uint8_t>>&   GetRawVertexData(uint32_t meshIndex = 0)		   = 0;
		virtual const Span<Meshlet>&				GetRawMeshletData(uint32_t meshIndex = 0)	       = 0;
		virtual const Span<PackedTriangle>&		    GetRawPrimitiveIndices(uint32_t meshIndex = 0)	   = 0;
		virtual const Span<uint8_t>&				GetRawUniqueVertexIndices(uint32_t meshIndex = 0)  = 0;

        virtual void SetTransform(float4x4 transform) = 0;
        virtual GPUResource* GetTransformBuffer() = 0;

        virtual GPUResource* GetModelCullData(const std::wstring_view& meshTag) = 0;
        virtual GPUResource* GetSceneCullData() = 0;

        virtual void Cull(PerspectiveCamera* camera, PerspectiveCamera* debugCamera) = 0;

	};

}
