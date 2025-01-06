#include <sktbdpch.h>
#include "D3DAssetManager.h"
#include "Skateboard/Renderer/ResourceFactory.h"
#include "Skateboard/Assets/AssetManager.h"
#include "Platform/DirectX12/D3DBuffer.h"
#include "Platform/DirectX12/D3DGraphicsContext.h"

//D3D12TK helpers
#include "ResourceUploadBatch.h"
#include "DDSTextureLoader.h"

//D3D12 MemAloc
#include "D3D12MemAlloc.h"

#include <fastgltf/core.hpp>
#include <fastgltf/types.hpp>
#include <fastgltf/glm_element_traits.hpp>
#include <fastgltf/tools.hpp>

namespace Skateboard
{
	D3DAssetManager::D3DAssetManager()
	{
	}

	const Texture* D3DAssetManager::LoadTextureImpl(const wchar_t* filename, const std::string_view& textureTag, TextureDimension_ resType)
	{
		if(m_Textures.count(textureTag) > 0) return &m_Textures[textureTag];

		std::filesystem::path path(filename);
		path.replace_extension(L".dds");

		// Check the file and load the texture using Microsoft's DDS library
		std::ifstream inputFile(path.native().c_str());
		if (inputFile.good())
		{
			if (!path.extension().wstring().compare(L".dds"))
			{

				DirectX::ResourceUploadBatch uploadBatch(gD3DContext->Device());

				uploadBatch.Begin();

				D3D12MA::Allocation* allocation;

				DirectX::CreateDDSTextureFromFile(gD3DContext->Device(),gD3DContext->GetMemoryAllocator(), uploadBatch, path.c_str(), &allocation);

				uploadBatch.End(gD3DContext->CommandQueue()).wait();

				//Plan
				// Update DDSLoader from here https://github.com/microsoft/DirectXTK12/wiki/DDSTextureLoader
				// modify to use the D3D12MA Allocator and Fill its Description
				// store D3D12MA::Allocation* in the void* memhandle as the rest of the resources that could be user created;

#ifndef SKTBD_SHIP
				//Texture result()(Resource) ? reinterpret_cast<D3D12MA::Allocation*>(Resource)->GetResource() : nullptr
				allocation->GetResource()->SetName(path.native().c_str());
#endif

				auto ds = allocation->GetResource()->GetDesc();
				TextureDesc desc = {};

				desc.AccessFlags = ResourceAccessFlag_GpuRead;
				desc.Depth = ds.DepthOrArraySize;
				desc.Dimension = resType;
				desc.Format = D3DBufferFormatToSkateboard(ds.Format);
				desc.Height = ds.Height;
				desc.Mips = ds.MipLevels;
				desc.Type = TextureType_Default;
				desc.Width = ds.Width;

				//create skateboard wrapper resource
				auto data = new SingleResource<TextureBuffer>(filename, desc);
				*static_cast<void**>(data->GetResourcePtr()) = allocation;

				TextureViewDesc view{};
				view.ArraySize = ds.DepthOrArraySize;
				view.Dimension = resType;
				view.Format = D3DBufferFormatToSkateboard(ds.Format);
				view.MipLevels = ds.MipLevels;
				view.MostDetailedMip = 0;

				//create skateboard view
				SingleResource<TextureView> textureView;
				ResourceFactory::CreateTextureView(textureView, filename, data, ViewAccessType_GpuRead, &view);
				
				m_Textures[textureTag] = textureView;

			}
			else
			{
				SKTBD_CORE_ASSERT(false, "Unsopported texture type. For now only DDS files are supported.");
				//delete result, result = nullptr;
				return nullptr;
			}
		}
		else
		{
			SKTBD_CORE_ASSERT(false, "Could not load the specified texture, texture not found.");
			//delete result, result = nullptr;
			return nullptr;
		}

		// Success
		++m_TextureMaxIndex;
		return &m_Textures[textureTag];
	}

	Mesh* D3DAssetManager::LoadModelImpl(const wchar_t* filename, const std::string_view& modelTag, const BufferLayout& Layout, const std::string_view& obj_name, bool flipNormals)
	{
		if (m_Meshes.count(modelTag) > 0) return m_Meshes[modelTag].get();
		else
		{

			auto PrimTypeToSkateboard = [](const fastgltf::PrimitiveType& type) -> SKTBD_PRIMITIVE_TOPOLOGY
				{
					switch (type)
					{
					case fastgltf::PrimitiveType::Points:		 return SKTBD_PRIMITIVE_TOPOLOGY::SKTBD_PRIMITIVE_TOPOLOGY_POINTLIST;
					case fastgltf::PrimitiveType::Lines:		 return SKTBD_PRIMITIVE_TOPOLOGY::SKTBD_PRIMITIVE_TOPOLOGY_LINELIST;
					case fastgltf::PrimitiveType::LineLoop: { SKTBD_CORE_INFO("CANNY DO LINE LOOPS HUN, ITS 2024 - gotta work with line strips"); return SKTBD_PRIMITIVE_TOPOLOGY::SKTBD_PRIMITIVE_TOPOLOGY_LINESTRIP; }
					case fastgltf::PrimitiveType::LineStrip:	 return SKTBD_PRIMITIVE_TOPOLOGY::SKTBD_PRIMITIVE_TOPOLOGY_LINESTRIP;
					case fastgltf::PrimitiveType::Triangles:	 return SKTBD_PRIMITIVE_TOPOLOGY::SKTBD_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
					case fastgltf::PrimitiveType::TriangleStrip: return SKTBD_PRIMITIVE_TOPOLOGY::SKTBD_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;
					case fastgltf::PrimitiveType::TriangleFan:	 return SKTBD_PRIMITIVE_TOPOLOGY::SKTBD_PRIMITIVE_TOPOLOGY_TRIANGLEFAN;
					default:
						break;
					}
				};

			auto LoadMeshToMemory = [&](fastgltf::Asset& ass, fastgltf::Mesh& mesh, const BufferLayout& RequiredComponents) -> Mesh*
				{
					
					std::vector<Primitive> primitives(mesh.primitives.size());
					//std::vector<Texture*> meshTextures(ass.textures.size());
					uint32_t index = 0;

					for (auto&& p : mesh.primitives) {

						auto& prim = primitives[index]; ++index;

						const fastgltf::Accessor& indexaccessor = ass.accessors[p.indicesAccessor.value()];
						const fastgltf::Accessor& vertaccessor = ass.accessors[p.findAttribute("POSITION")->accessorIndex];

						//Check Vertex and index sizes

						if(indexaccessor.count == 0 || vertaccessor.count == 0 )
						{
							SKTBD_CORE_INFO("LoadModel::{3} Loading mesh {0} within model failed, index or vertex buffer contain no vertices, indices {1}, vertices {2}", index, indexaccessor.count, vertaccessor.count, modelTag)
							continue;
						}
						else
						{
							SKTBD_CORE_WARN("LoadModel::{3} Loading mesh {0}, indices {1}, vertices {2}", index, indexaccessor.count, vertaccessor.count, modelTag)
						}

						//IndexSize
						size_t indexSize = 0;
						if (indexaccessor.componentType == fastgltf::ComponentType::UnsignedInt)
						{
							indexSize = sizeof(uint32_t);
							prim.IndexBuffer.m_Format = IndexFormat::bit32;
						}
						else
						{
							indexSize = sizeof(uint16_t);
							prim.IndexBuffer.m_Format = IndexFormat::bit16;
						}

						size_t PrimitiveSize = indexaccessor.count * indexSize + vertaccessor.count * RequiredComponents.GetStride();

						MemoryUtils::VIRTUAL_ALLOCATION_DESC desc{};
						desc.Size = PrimitiveSize;

						auto suballocation = std::make_shared<PrimitiveSuballocationSingleBuffer>();
						suballocation->ParentAllocator = m_VertexBufferSuballocator;

						uint64_t Offset;
						m_VertexBufferSuballocator->Allocate(&desc, &suballocation->PrimitiveData, &Offset);

						prim.IndexBuffer.m_IndexCount = indexaccessor.count;
						prim.IndexBuffer.m_Offset = Offset;
						prim.IndexBuffer.m_ParentResource = &m_StaticMeshVertexBuffer;

						prim.VertexBuffer.m_Offset = Offset + indexaccessor.count * indexSize;
						prim.VertexBuffer.m_ParentResource = &m_StaticMeshVertexBuffer;
						prim.VertexBuffer.m_VertexCount = vertaccessor.count;
						prim.VertexBuffer.m_VertexStride = RequiredComponents.GetStride();

						prim.Allocations = suballocation;
						prim.Layout = RequiredComponents;

						GraphicsContext::Context->CopyDataToBuffer(&m_StaticMeshVertexBuffer, Offset, PrimitiveSize, [&](void* dest)

							{
								//write indices
								if (prim.IndexBuffer.m_Format = IndexFormat::bit16)
									fastgltf::iterateAccessorWithIndex<uint16_t>(ass, indexaccessor,
										[&](std::uint16_t idx, size_t i) {
											((uint16_t*)(dest))[indexaccessor.count - i - 1] = idx;
										});
								else
									fastgltf::iterateAccessorWithIndex<std::uint32_t>(ass, indexaccessor,
										[&](std::uint32_t idx, size_t i) {
											((uint32_t*)(dest))[indexaccessor.count - i - 1] = idx;
										});

								//write vertices
								for (auto& element : RequiredComponents)
								{
									if (!p.findAttribute(element.SemanticName))
									{
										SKTBD_CORE_WARN("LoadModel::{0} Vertex info does not contain attribute {1}, skipping...",modelTag, element.SemanticName)
										continue;
									}

									fastgltf::Accessor& accessor = ass.accessors[p.findAttribute(element.SemanticName)->accessorIndex];

									switch (element.Type)
									{
									case ShaderDataType_::Bool:
										fastgltf::iterateAccessorWithIndex<uint8_t>(ass, accessor,
											[&](uint8_t value, size_t index) {
												*(uint8_t*)(&((uint8_t*)dest)[indexaccessor.count * indexSize + index * RequiredComponents.GetStride() + element.Offset]) = value;
											});
										break;
									case ShaderDataType_::Int:
										fastgltf::iterateAccessorWithIndex<int>(ass, accessor,
											[&](int value, size_t index) {
												*(int*)(&((uint8_t*)dest)[accessor.count * indexSize + index * RequiredComponents.GetStride() + element.Offset]) = value;
											});
										break;
									case ShaderDataType_::Int2:
										fastgltf::iterateAccessorWithIndex<glm::u32vec2>(ass, accessor,
											[&](glm::u32vec2 value, size_t index) {
												*(glm::u32vec2*)(&((uint8_t*)dest)[indexaccessor.count * indexSize + index * RequiredComponents.GetStride() + element.Offset]) = value;
											});
										break;
									case ShaderDataType_::Int3:
										fastgltf::iterateAccessorWithIndex<glm::u32vec3>(ass, accessor,
											[&](glm::u32vec3 value, size_t index) {
												*(glm::u32vec3*)(&((uint8_t*)dest)[indexaccessor.count * indexSize + index * RequiredComponents.GetStride() + element.Offset]) = value;
											});
										break;
									case ShaderDataType_::Int4:
										fastgltf::iterateAccessorWithIndex<glm::u32vec4>(ass, accessor,
											[&](glm::u32vec4 value, size_t index) {
												*(glm::u32vec4*)(&((uint8_t*)dest)[indexaccessor.count * indexSize + index * RequiredComponents.GetStride() + element.Offset]) = value;
											});
										break;
									case ShaderDataType_::Uint:
										fastgltf::iterateAccessorWithIndex<uint32_t>(ass, accessor,
											[&](uint32_t value, size_t index) {
												*(uint32_t*)(&((uint8_t*)dest)[indexaccessor.count * indexSize + index * RequiredComponents.GetStride() + element.Offset]) = value;
											});
										break;
									case ShaderDataType_::Uint2:
										fastgltf::iterateAccessorWithIndex<glm::u32vec2>(ass, accessor,
											[&](glm::u32vec2 value, size_t index) {
												*(glm::u32vec2*)(&((uint8_t*)dest)[indexaccessor.count * indexSize + index * RequiredComponents.GetStride() + element.Offset]) = value;
											});
										break;
									case ShaderDataType_::Uint3:
										fastgltf::iterateAccessorWithIndex<glm::u32vec3>(ass, accessor,
											[&](glm::u32vec3 value, size_t index) {
												*(glm::u32vec3*)(&((uint8_t*)dest)[indexaccessor.count * indexSize + index * RequiredComponents.GetStride() + element.Offset]) = value;
											});
										break;
									case ShaderDataType_::Uint4:
										fastgltf::iterateAccessorWithIndex<glm::u32vec4>(ass, accessor,
											[&](glm::u32vec4 value, size_t index) {
												*(glm::u32vec4*)(&((uint8_t*)dest)[indexaccessor.count * indexSize + index * RequiredComponents.GetStride() + element.Offset]) = value;
											});
										break;
									case ShaderDataType_::Float:
										fastgltf::iterateAccessorWithIndex<float>(ass, accessor,
											[&](float value, size_t index) {
												*(float*)(&((uint8_t*)dest)[indexaccessor.count * indexSize + index * RequiredComponents.GetStride() + element.Offset]) = value;
											});
										break;
									case ShaderDataType_::Float2:
										fastgltf::iterateAccessorWithIndex<glm::vec2>(ass, accessor,
											[&](glm::vec2 value, size_t index) {
												*(glm::vec2*)(&((uint8_t*)dest)[indexaccessor.count * indexSize + index * RequiredComponents.GetStride() + element.Offset]) = value;
											});
										break;
									case ShaderDataType_::Float3:
										fastgltf::iterateAccessorWithIndex<glm::vec3>(ass, accessor,
											[&](glm::vec3 value, size_t index) {
												*(glm::vec3*)(&((uint8_t*)dest)[indexaccessor.count * indexSize + index * RequiredComponents.GetStride() + element.Offset]) = value;
											});
										break;
									case ShaderDataType_::Float4:
										fastgltf::iterateAccessorWithIndex<glm::vec4>(ass, accessor,
											[&](glm::vec4 value, size_t index) {
												*(glm::vec4*)(&((uint8_t*)dest)[indexaccessor.count * indexSize + index * RequiredComponents.GetStride() + element.Offset]) = value;
											});
										break;

									default:
										break;
									}
								}
							}
						);
					};

					m_Meshes[modelTag].reset();
					m_Meshes[modelTag] = std::make_unique<Mesh>(primitives);

					return m_Meshes[modelTag].get();
			};

			fastgltf::Parser parser;

			std::filesystem::path path(filename);
			path.replace_extension(L".gltf");

			auto data = fastgltf::GltfDataBuffer::FromPath(path);
			if (data.error() != fastgltf::Error::None) {
				// The file couldn't be loaded, or the buffer could not be allocated.
				SKTBD_CORE_WARN("Asset path not found!");
				return nullptr;
			}

			auto asset = parser.loadGltf(data.get(), path.parent_path(), fastgltf::Options::None);
			if (auto error = asset.error(); error != fastgltf::Error::None) {
				// Some error occurred while reading the buffer, parsing the JSON, or validating the data.
				return nullptr;
			}

			return LoadMeshToMemory(asset.get(), asset.get().meshes[0],Layout);
			
		};
		
	}

	Animation* D3DAssetManager::LoadAnimationImpl(const wchar_t* filename, const std::string_view& animationTag)
	{
		return nullptr;
	}
	SkinnedMesh* D3DAssetManager::LoadSkeletalMeshImpl(const wchar_t* filename, const std::string_view& meshTag, bool flipNormals)
	{
		return nullptr;
	}
	Skeleton* D3DAssetManager::LoadSkeletonImpl(const wchar_t* filename, const std::string_view& skeletonTag)
	{
		return nullptr;
	}
	Texture* D3DAssetManager::CreateTextureFromDataImpl(const std::string_view& textureTag, const uint32_t& width, const uint32_t& height, void* data)
	{
		return nullptr;
	}
	ImFont* D3DAssetManager::LoadFontImpl(const wchar_t* filename, uint32_t sizeInPixels, const std::string_view& modelTag)
	{
		return nullptr;
	}
}