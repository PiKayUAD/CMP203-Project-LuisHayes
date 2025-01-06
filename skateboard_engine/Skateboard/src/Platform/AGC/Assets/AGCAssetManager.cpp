#include "sktbdpch.h"
#include "AGCAssetManager.h"
#include "Platform/AGC/AGCGraphicsContext.h"
#include "Platform/AGC/AGCF.h"

#include "Skateboard/Renderer/InternalFormats.h"
#include "Skateboard/Utilities/StringConverters.h"
#include "Skateboard/Renderer/Model/Model.h"

#include "Platform/AGC/Model/Utils.h"
#include "Platform/AGC/AGCBuffer.h"
#include "Platform/AGC/Animation/AGCAnimation.h"

// Include helper files for unloading pack files
#include "Platform/Playstation/model-loading/pack_file.h"
#include "Platform/Playstation/model-loading/error_codes.h"
#include "Platform/Playstation/model-loading/array.h"
#include "Platform/Playstation/model-loading/material_models.h"

#include <edgeanim.h>
#include <edge/libedgeanimtool/libedgeanimtool_skeleton.h> // Include these to parse the EdgeAnimSkeleton header
#include <edge/libedgeanimtool/libedgeanimtool_animation.h>// and EdgeAnimAnimation respectively. 

//#pragma comment(lib, "libSceEdgeAnimTool.a")
#include  "Skateboard/Renderer/Pipeline.h"

#include "Skateboard/ImGui/backends/Playstation/imgui_impl_ps.h"

#define SKTBD_LOG_COMPONENT "ASSET_MANAGER_IMPL"
#include "Skateboard/Log.h"

namespace Skateboard
{
	AGCAssetManager::AGCAssetManager()
	{
		//loading default textures would be a nice thing to do?
		std::filesystem::path p(SanitizeFilePath("assets/textures/whitePixel.gnf"));
		if (std::filesystem::exists(p))
			LoadTextureImpl(L"assets/textures/whitePixel.gnf", DefaultTexKey, TextureDimension_Texture2D);
		else
			LoadTextureImpl(L"assets/whitePixel.gnf", DefaultTexKey, TextureDimension_Texture2D);
	}

	Texture* AGCAssetManager::CreateTextureFromDataImpl(const std::string_view& textureTag, const uint32_t& width, const uint32_t& height, void* data)
	{
		// Need to find an ideal way of creating simple textures by using raw data pointers through AGC

		//std::wstring filename = L"PixelTexture";
		//
		//// To get a Core::Texture out of the binary blob, we can simply use a translate.
		//sce::Agc::Core::Texture tex;
		//uint64_t* data2 = new uint64_t(0xffffffffffffffff);
		//tex.setDataAddress(data2);
		//tex.setBorderColorTableSwizzle(sce::Agc::Core::Texture::BorderColorSwizzle::kRGBA);
		//tex.setFormat(sce::Agc::Core::TypedFormat::k8_8_8_8Srgb);
		//tex.setPrtDefaultColor(sce::Agc::Core::Texture::PrtDefaultColor::k1111);
		//tex.setWidth(width);
		//tex.setHeight(height);
		//tex.setType(sce::Agc::Core::Texture::Type::k2d);
		//tex.setMetadataAddress(data2);


		//// Create a new texture based on the data extrapolated from the file.
		//TextureDesc desc = {};
		//desc.Width = tex.getWidth();
		//desc.Height = tex.getHeight();
		//desc.Type = AGCTextureTypeToSkateboard(tex.getType());
		//std::unique_ptr<AGCTexture> texture(new AGCTexture(filename, desc, tex));

		//// Store the texture id for later use.
		//uint32_t id = m_AvailableIndices.back();
		//m_IdToTag[id] = textureTag;
		//m_TagToId[textureTag] = id;
		//m_AvailableIndices.pop_back();

		//// Move texture into map for memory management.
		//m_Textures.emplace(textureTag, std::move(texture));
		//return m_Textures[textureTag].get();
		return nullptr;
	}

	ImFont* AGCAssetManager::LoadFontImpl(const wchar_t* filename,  uint32_t sizeInPixels, const std::string_view& modelTag)
	{
		std::filesystem::path path(SanitizeFilePath(filename));
		SKTBD_CORE_ASSERT(std::filesystem::exists(path), "Font file does not exist!");

		std::string string = path.string();
		auto font = ImGui_PS::addFontOTF(string.c_str(), sizeInPixels);

		m_Fonts.emplace(modelTag,font);

		return font;
	}

	Texture* AGCAssetManager::LoadTextureImpl(const wchar_t* filename, const std::string_view& textureTag, TextureDimension_ resType)
	{
		if (m_Textures.find(textureTag) != m_Textures.end())
			return m_Textures[textureTag].get();
		// The filename is converted to chars and we verify that the extension is '.gnf' since
		// the playstation textures are in that format.
		std::filesystem::path path(SanitizeFilePath(filename));
		if (path.has_extension())
			path.replace_extension(".gnf");
		SKTBD_CORE_ASSERT(std::filesystem::exists(path), "Texture file does not exist!");
		std::string pathStr = path.string();

		// For simplicity, this is done with regular kernel file I/O and not with APR.
		SceKernelStat s = {};
		int32_t error, fileID;
		fileID = sceKernelOpen(path.c_str(), SCE_KERNEL_O_RDONLY, 0);
		SCE_AGC_ASSERT_MSG(fileID > 0, "Unable to open texture %s", pathStr.c_str());
		error = sceKernelFstat(fileID, &s);
		SCE_AGC_ASSERT_MSG(error == SCE_OK, "Unable to stat texture %s", pathStr.c_str());

		
		
		// Memory is allocated based on the maximum alignment for tiled resources, to allow this code to load all textures.
		// In actual application, determining the correct alignment in advance can save memory.
		const sce::Agc::SizeAlign sAlign = { (size_t)s.st_size, sce::Agc::Alignment::kMaxTiledAlignment };

		auto memoryHandle = gAGCContext->GetMemAllocator()->TypedAlignedAllocate<uint8_t, sce::Agc::Alignment::kMaxTiledAlignment>(sAlign.m_size);
		void* pPoolData = memoryHandle.data;
		sce::Gnf::GnfFileV5* gnf = (sce::Gnf::GnfFileV5*)pPoolData;

		// Read the file into the allocated memory and close the file once completed
		size_t bytes = sceKernelRead(fileID, gnf, s.st_size);
		SCE_AGC_ASSERT_MSG(bytes == s.st_size, "Unable to read texture %s", pathStr.c_str());
		sceKernelClose(fileID);

		// To get a Core::Texture out of the binary blob, we can simply use a translate.
		sce::Agc::Core::Texture tex;
		error = sce::Agc::Core::translate(&tex, gnf);
		SCE_AGC_ASSERT_MSG(error == SCE_OK, "Unable to decode texture %s", pathStr.c_str());

#ifndef SKTBD_SHIP
		sce::Agc::Core::registerResource(&tex, "%s", pathStr.c_str());
#endif // !SKTBD_SHIP

		// Create a new texture based on the data extrapolated from the file.
		TextureDesc desc = {};
		desc.Width		= tex.getWidth();
		desc.Height		= tex.getHeight();
		desc.Depth		= tex.getDepth();
		desc.Dimension  = AGCTextureDimensionToSkateboard(tex.getType());
		//std::unique_ptr<AGCTexture> texture(new AGCTexture(filename, desc, tex, memoryHandle));

		// Store the texture id for later use.
		uint32_t id = m_AvailableIndices.back();
		m_IdToTag[id] = textureTag;
		m_TagToId[textureTag] = id;
		m_AvailableIndices.pop_back();

		// Move texture into map for memory management.
		std::string name = path.filename().string();
		//m_Textures.emplace(textureTag, std::move(texture));
		return m_Textures[textureTag].get();
	}


	Mesh* AGCAssetManager::LoadModelImpl(const wchar_t* filename, const std::string_view& modelTag, bool flipNormals)
	{
		PackFile::Package packFile = LoadPackage(filename);
		// Ensure that this package is valid
		SCE_AGC_ASSERT_MSG(packFile.gpu_data_size != 0, "");

		std::unique_ptr<Mesh> model = std::make_unique<Mesh>();

		// The header gives the information about how many meshes and textures there will be in total.
		// Resize the vectors so that it is easier to reference (this is lazy, for a fully fledged game avoid).
		std::vector<Primitive> meshes;
		meshes.resize(packFile.header.meshes_count);

		 std::vector<Texture*> textures;
		 textures.resize(packFile.header.texture_count);

		/* ----- 1. Load all textures ----- */
		if (textures.size())
		{
			for (uint32_t i = 0u; i < packFile.textures.size(); ++i)
			{
				const PackFile::TextureRef& tex = packFile.textures[i];
				std::string path = "assets/models/";
				path.append(tex.path.data());
				auto wpath = ToWString(path);
				textures[i] = LoadTextureImpl(wpath.c_str(), modelTag, TextureDimension_Texture2D);
				model->AddTexture(textures[i]);
			}
		}
		else {
			// Add default texture
			model->AddTexture(GetTextureImpl(DefaultTexKey));
		}

		// The nodes represent a collection of meshes transformed in space
		// The goal will be to loop through all nodes and find all their meshes
		for (const PackFile::Node& node : packFile.nodes)
		{
			/* ----- 2. Get the texture index for the mesh ----- */
			uint32_t nodeTextureIndex = 0u;
			// Only look for a texture index if any texture is provided
			if (packFile.header.texture_count)
			{
				// Multiple materials may be applied to the same mesh(es).
				// But we are only interested in the one that contains our texture index!
				for (size_t i = 0u, albedoFound = false; i < node.material_index.size() && !albedoFound; ++i)
				{
					const uint16_t& matIndex = node.material_index[i];
					const PackFile::Material& mat = packFile.materials[matIndex];
					for (const PackFile::MaterialProperty& prop : mat.properties)
					{
						// Check for albedo. If PBR, you can get the normal map, etc. using the corresponding MaterialPropertiesSemantic
						if (prop.semantic == PackFile::MaterialPropertiesSemantic::e_mps_albedo &&
							PackFile::MaterialPropertyType::get_data_type(prop.type) == PackFile::MaterialPropertyType::e_mpt_texture)
						{
							const PackFile::TextureProperties* texProp = (PackFile::TextureProperties*)prop.value_ptr;
							nodeTextureIndex = texProp->tx_idx;

							// Break here because we are not looking for anything else than albedo
							albedoFound = true;
							break;
						}
					}
				}
			}

			for (const uint16_t& meshIndex : node.meshes)
			{
				const PackFile::Mesh& packMesh = packFile.meshes[meshIndex];
				Primitive& currentMesh = meshes[meshIndex];

				// Assign the texture index
				currentMesh.TextureIndex = nodeTextureIndex;

				const uint32_t vertexCount = packMesh.vertex_count;
				const uint32_t indexCount = packMesh.index_count;
				std::vector<VertexType> vertices(vertexCount);
				std::vector<uint32_t> indices(indexCount);

				/* ----- 3. Get the indices of the mesh ----- */
				// The indices may be supplied in 8, 16 or 32 bits formats.
				// We need to handle proper conversion to 32 bits indices to stay consistent with our pipeline
				const PackFile::Buffer iBuff = packFile.buffers[packMesh.index_buffer];
				char* iBufferPtr = (char*)packFile.gpu_data + iBuff.offset;

				SCE_AGC_ASSERT(packMesh.index_elem_size <= 32);	// This is in bits. We will use the buffer stride for equivalent in bytes.
				for (uint32_t i = 0u; i < iBuff.elem_count; ++i)
				{
					memcpy(&indices[i], iBufferPtr, iBuff.stride); //Implicit type conversion
					iBufferPtr += iBuff.stride;
				}

				/* ----- 4. Get the vertices of the mesh ----- */
				// The vertex attributes define the vertex type. Like the indices, the packfile converter
				// may shrink the data down to occupy less space, so we need to handle proper conversion here
				// as well. Note that our implementation does not cover all types, but should be good enough
				// for a wide variety of models using the converter tool. Feel free to improve our code.
				for (const PackFile::VertexAttribute& attribute : packMesh.attributes)
				{
					// Retrieve the memory location of the first attribute of this type in the buffer
					const PackFile::Buffer bufferDesc = packFile.buffers[packMesh.vertex_buffers[attribute.vertex_buffer_index]];
					char* bufferPtr = (char*)packFile.gpu_data + bufferDesc.offset + attribute.offset;

					switch (attribute.semantic)
					{
					case PackFile::VertexSemantic::e_vx_position:
						SCE_AGC_ASSERT(attribute.format == PackFile::VertexAttribFormat::e_float32_3);
						// No conversion required
						for (uint32_t i = 0; i < bufferDesc.elem_count; ++i)
						{
							memcpy(&vertices[i].position, bufferPtr, sizeof(float3));
							if (node.parent_node_index >= 0)
							{
								MatrixMul(vertices[i].position, packFile.nodes[node.parent_node_index].matrix);
							}
							MatrixMul(vertices[i].position, node.matrix);
							bufferPtr += bufferDesc.stride;
						}
						break;
					case PackFile::VertexSemantic::e_vx_uv_channel:
						SCE_AGC_ASSERT(attribute.format == PackFile::VertexAttribFormat::e_float16_2);
						// Convert the 16 bits float type to a 32 bit float
						for (uint32_t i = 0; i < bufferDesc.elem_count; ++i)
						{
							glm::uint16 data[2];
							memcpy(&data, bufferPtr, sizeof(glm::uint16)*2);
							vertices[i].uv = { halfToFloat(data[0]), halfToFloat(data[1])};
							bufferPtr += bufferDesc.stride;
						}
						break;
					case PackFile::VertexSemantic::e_vx_normal:
						SCE_AGC_ASSERT(attribute.format == PackFile::VertexAttribFormat::e_sn_int16_4);
						// Convert the 16 bits signed integer type to a 32 bit float
						for (uint32_t i = 0; i < bufferDesc.elem_count; ++i)
						{
							glm::int16 data[4];
							memcpy(&data, bufferPtr, sizeof(glm::int16)*4);
							vertices[i].normal = { snorm16ToFloat(data[0]), snorm16ToFloat(data[1]), snorm16ToFloat(data[2])};
							if (node.parent_node_index >= 0)
							{
								MatrixMul3x3(vertices[i].normal, packFile.nodes[node.parent_node_index].matrix);
							}
							MatrixMul3x3(vertices[i].normal, node.matrix);
							Normalize(vertices[i].normal);
							if(flipNormals) 
								vertices[i].normal *= -1.0f;
							bufferPtr += bufferDesc.stride;
						}
						break;
					case PackFile::VertexSemantic::e_vx_tangent:
						SCE_AGC_ASSERT(attribute.format == PackFile::VertexAttribFormat::e_sn_int16_4);
						// Convert the 16 bits signed integer type to a 32 bit float
						for (uint32_t i = 0; i < bufferDesc.elem_count; ++i)
						{
							glm::int16 data[4];
							memcpy(&data, bufferPtr, sizeof(glm::int16) * 4);
							vertices[i].tangent = { snorm16ToFloat(data[0]), snorm16ToFloat(data[1]), snorm16ToFloat(data[2]) };
							if (node.parent_node_index >= 0)
							{
								MatrixMul3x3(vertices[i].tangent, packFile.nodes[node.parent_node_index].matrix);
							}
							MatrixMul3x3(vertices[i].tangent, node.matrix);
							Normalize(vertices[i].tangent);

							// If we have the tangent vector, we can compute the bitangent vector for completeness.
							vertices[i].bitangent = glm::cross(vertices[i].normal, vertices[i].tangent);
							Normalize(vertices[i].bitangent);

							bufferPtr += bufferDesc.stride;
						}
						break;
					default:
						continue;
					}
				}

				/* ----- 5. Create the vertex and index buffers on the PlayStation 5 memory ----- */

				// We'll create PS5 resource buffers
				std::wstring debugName = L"Mesh " + std::to_wstring(meshIndex) + L" VB";
				//currentMesh.VertexResource = VertexBuffer::Create(debugName, vertices.data(), static_cast<uint32_t>(vertices.size()));

				// Additionally, lets store the vertex data in system memory for CPU access
				//memcpy(currentMesh.Vertices.data(), vertices.data(), vertices.size() * sizeof(VertexType));


				// We'll create PS5 index buffers
				debugName = L"Mesh " + std::to_wstring(meshIndex) + L" IB";
				//currentMesh.IndexResource = IndexBuffer::Create(debugName, indices.data(), static_cast<uint32_t>(indices.size()));

				// We'll do the same for the indices.
				//memcpy(currentMesh.Indices.data(), indices.data(), indices.size() * sizeof(uint32_t));

				model->AddMesh(currentMesh);

			}
		}

		// Store the model into the map
		m_Models.emplace(modelTag, std::move(model));


		return m_Models[modelTag].get();
	}

	PackFile::Package AGCAssetManager::LoadPackage(const wchar_t* filename)
	{
		// Obtain a PackFile::Package from the pack file so we can parse the data
		PackFile::LoaderOptions options = {};
		PackFile::Package pack = {};

		std::filesystem::path path(SanitizeFilePath(filename));
		path.replace_extension(".pack");


		//std::string path = ToString();
		uint32_t err = PackFile::load_package(path.c_str(), pack, options);
		
		if (err != SCE_OK)
		{
			SKTBD_ASSERT(!err, "Invalid filepath");
		}

		return pack;
	}

	SkinnedMesh* AGCAssetManager::LoadSkeletalMeshImpl(const wchar_t* filename, const std::string_view& meshTag, bool flipNormals)
	{
		PackFile::Package packFile = LoadPackage(filename);
		// Ensure that this package is valid
		SCE_AGC_ASSERT_MSG(packFile.gpu_data_size != 0, "");

		std::unique_ptr<Mesh> model = std::make_unique<Mesh>();

		// The header gives the information about how many meshes and textures there will be in total.
		// Resize the vectors so that it is easier to reference (this is lazy, for a fully fledged game avoid).
		std::vector<SkinnedMesh> meshes;
		meshes.resize(packFile.header.meshes_count);

		std::vector<Texture*> textures;
		textures.resize(packFile.header.texture_count);


		// The nodes represent a collection of meshes transformed in space
		// The goal will be to loop through all nodes and find all their meshes
		for (const PackFile::Node& node : packFile.nodes)
		{
			/* ----- 2. Get the texture index for the mesh ----- */
			uint32_t nodeTextureIndex = 0u;
			// Only look for a texture index if any texture is provided
			if (packFile.header.texture_count)
			{
				// Multiple materials may be applied to the same mesh(es).
				// But we are only interested in the one that contains our texture index!
				for (size_t i = 0u, albedoFound = false; i < node.material_index.size() && !albedoFound; ++i)
				{
					const uint16_t& matIndex = node.material_index[i];
					const PackFile::Material& mat = packFile.materials[matIndex];
					for (const PackFile::MaterialProperty& prop : mat.properties)
					{
						// Check for albedo. If PBR, you can get the normal map, etc. using the corresponding MaterialPropertiesSemantic
						if (prop.semantic == PackFile::MaterialPropertiesSemantic::e_mps_albedo &&
							PackFile::MaterialPropertyType::get_data_type(prop.type) == PackFile::MaterialPropertyType::e_mpt_texture)
						{
							const PackFile::TextureProperties* texProp = (PackFile::TextureProperties*)prop.value_ptr;
							nodeTextureIndex = texProp->tx_idx;

							// Break here because we are not looking for anything else than albedo
							albedoFound = true;
							break;
						}
					}
				}
			}

			// Go through all the mesh nodes
			for (const uint16_t& meshIndex : node.meshes)
			{
				const PackFile::Mesh& packMesh = packFile.meshes[meshIndex];
				SkinnedMesh& currentMesh = meshes[meshIndex];

				// Assign the texture index
				currentMesh.TextureIndex = nodeTextureIndex;

				const uint32_t vertexCount = packMesh.vertex_count;
				const uint32_t indexCount = packMesh.index_count;
				std::vector<SkinnedVertexType> vertices(vertexCount);
				std::vector<uint32_t> indices(indexCount);

				/* ----- 3. Get the indices of the mesh ----- */
				const PackFile::Buffer iBuff = packFile.buffers[packMesh.index_buffer];
				char* iBufferPtr = (char*)packFile.gpu_data + iBuff.offset;

				SCE_AGC_ASSERT(packMesh.index_elem_size <= 32);	// This is in bits. We will use the buffer stride for equivalent in bytes.
				for (uint32_t i = 0u; i < iBuff.elem_count; ++i)
				{
					memcpy(&indices[i], iBufferPtr, iBuff.stride); //Implicit type conversion
					iBufferPtr += iBuff.stride;
				}
				 
				/* ----- 4. Get the vertices of the mesh ----- */
				for (const PackFile::VertexAttribute& attribute : packMesh.attributes)
				{

					// Retrieve the memory location of the first attribute of this type in the buffer
					const PackFile::Buffer bufferDesc = packFile.buffers[packMesh.vertex_buffers[attribute.vertex_buffer_index]];
					char* bufferPtr = (char*)packFile.gpu_data + bufferDesc.offset + attribute.offset;

					switch (attribute.semantic)
					{
					case PackFile::VertexSemantic::e_vx_position:
						SCE_AGC_ASSERT(attribute.format == PackFile::VertexAttribFormat::e_float32_3);
						// No conversion required
						for (uint32_t i = 0; i < bufferDesc.elem_count; ++i)
						{
							memcpy(&vertices[i].position, bufferPtr, sizeof(float3));
							if (node.parent_node_index >= 0)
							{
								MatrixMul(vertices[i].position, packFile.nodes[node.parent_node_index].matrix);
							}
							MatrixMul(vertices[i].position, node.matrix);
							bufferPtr += bufferDesc.stride;
						}
						break;
					case PackFile::VertexSemantic::e_vx_uv_channel:
						SCE_AGC_ASSERT(attribute.format == PackFile::VertexAttribFormat::e_float16_2);
						// Convert the 16 bits float type to a 32 bit float
						for (uint32_t i = 0; i < bufferDesc.elem_count; ++i)
						{
							glm::uint16 data[2];
							memcpy(&data, bufferPtr, sizeof(glm::uint16)*2);
							vertices[i].uv = { halfToFloat(data[0]), halfToFloat(data[1])};
							bufferPtr += bufferDesc.stride;
						}
						break;
					case PackFile::VertexSemantic::e_vx_normal:
						SCE_AGC_ASSERT(attribute.format == PackFile::VertexAttribFormat::e_sn_int16_4);
						// Convert the 16 bits signed integer type to a 32 bit float
						for (uint32_t i = 0; i < bufferDesc.elem_count; ++i)
						{
							glm::int16 data[4];
							memcpy(&data, bufferPtr, sizeof(glm::int16)*4);
							vertices[i].normal = { snorm16ToFloat(data[0]), snorm16ToFloat(data[1]), snorm16ToFloat(data[2])};
							if (node.parent_node_index >= 0)
							{
								MatrixMul3x3(vertices[i].normal, packFile.nodes[node.parent_node_index].matrix);
							}
							MatrixMul3x3(vertices[i].normal, node.matrix);
							Normalize(vertices[i].normal);
							if (flipNormals)
								vertices[i].normal *= -1.0f;
							bufferPtr += bufferDesc.stride;
						}
						break;
					case PackFile::VertexSemantic::e_vx_tangent:
						SCE_AGC_ASSERT(attribute.format == PackFile::VertexAttribFormat::e_sn_int16_4);
						// Convert the 16 bits signed integer type to a 32 bit float
						for (uint32_t i = 0; i < bufferDesc.elem_count; ++i)
						{
							glm::int16 data[4];
							memcpy(&data, bufferPtr, sizeof(glm::int16)*4);
							vertices[i].tangent = { snorm16ToFloat(data[0]), snorm16ToFloat(data[1]), snorm16ToFloat(data[2])};
							if (node.parent_node_index >= 0)
							{
								MatrixMul3x3(vertices[i].tangent, packFile.nodes[node.parent_node_index].matrix);
							}
							MatrixMul3x3(vertices[i].tangent, node.matrix);
							Normalize(vertices[i].tangent);

							// If we have the tangent vector, we can compute the bitangent vector for completeness.
							vertices[i].bitangent = glm::cross(vertices[i].normal, vertices[i].tangent);
							Normalize(vertices[i].bitangent);

							bufferPtr += bufferDesc.stride;
						}
						break;
					case PackFile::VertexSemantic::e_vx_bone_indices:
						SCE_AGC_ASSERT(attribute.format == PackFile::VertexAttribFormat::e_sn_int16_4);
						// Convert the 16 bits signed integer type to a 32 bit float
						for (uint32_t i = 0; i < bufferDesc.elem_count; ++i)
						{
							glm::int16 data[4];
							memcpy(&data[0], bufferPtr, sizeof(int16_t) * 4);

							vertices[i].blendIndices = { data[0], data[1], data[2], data[3]};

							bufferPtr += bufferDesc.stride;
						}
						break;
					case PackFile::VertexSemantic::e_vx_bone_weights:
						SCE_AGC_ASSERT(attribute.format == PackFile::VertexAttribFormat::e_sn_int16_4);
						// Convert the 16 bits signed integer type to a 32 bit float
						for (uint32_t i = 0; i < bufferDesc.elem_count; ++i)
						{
							glm::int16 data[4];
							memcpy(&data, bufferPtr, sizeof(glm::int16) * 4);

							vertices[i].blendWeights = { snorm16ToFloat(data[0]), snorm16ToFloat(data[1]), snorm16ToFloat(data[2]), snorm16ToFloat(data[3])};

							bufferPtr += bufferDesc.stride;
						}
						break;
					default:
						continue;
					}
				}

				// Create VBs and IBs

				// We'll create PS5 resource buffers
				std::wstring debugName = L"Mesh " + std::to_wstring(meshIndex) + L" VB";
				//currentMesh.VertexResource = VertexBuffer::Create(debugName, vertices.data(), static_cast<uint32_t>(vertices.size()), BufferLayout::GetDefaultSkinnedMeshLayout());

				// We'll create PS5 index buffers
				debugName = L"Mesh " + std::to_wstring(meshIndex) + L" IB";
				//currentMesh.IndexResource = IndexBuffer::Create(debugName, indices.data(), static_cast<uint32_t>(indices.size()));

				//Skeleton skeleton;

				///* ----- 5. Get the skeleton of the mesh ----- */
				//for (const PackFile::Skeleton& packSkeleton : packFile.skeletons)
				//{
				//	// Store the name of the skeleton
				//	skeleton.SetName(packSkeleton.name.data());

				//	for (auto pose : packSkeleton.bind_pose)
				//	{
				//		// convert the joint matrix to glm::mat4x4
				//		Joint joint;
				//		joint.InverseBindPose = glm::make_mat4x4(pose.values);
				//		skeleton.AddJoint(joint);	
				//	}
	
				//}

				std::unique_ptr<SkinnedMesh> skinnedMesh;
				skinnedMesh = std::make_unique<SkinnedMesh>(currentMesh);
				m_SkinnedMesh[meshTag] = std::move(skinnedMesh);
			}
		}

		return m_SkinnedMesh[meshTag].get();
	}

	Skeleton* AGCAssetManager::LoadSkeletonImpl(const wchar_t* filename, const std::string_view& skeletonTag)
	{
		std::filesystem::path path(SanitizeFilePath(filename));

		SKTBD_CORE_ASSERT(std::filesystem::exists(path), "Skeleton file does not exist!");
		std::string pathStr = path.string();

		// For simplicity, this is done with regular kernel file I/O and not with APR.
		SceKernelStat s = {};
		int32_t error, fileID;
		fileID = sceKernelOpen(path.c_str(), SCE_KERNEL_O_RDONLY, 0);
		SCE_AGC_ASSERT_MSG(fileID > 0, "Unable to open skeleton %s", pathStr.c_str());
		error = sceKernelFstat(fileID, &s);
		SCE_AGC_ASSERT_MSG(error == SCE_OK, "Unable to stat skeleton %s", pathStr.c_str());

		// EdgeAnimSkeleton is 16-byte aligned
		const sce::Agc::SizeAlign sAlign = { (uint64_t)s.st_size, sizeof(int16_t) };
		auto handle = gAGCContext->GetMemAllocator()->TypedAlignedAllocate<EdgeAnimSkeleton, sizeof(int16_t)>(s.st_size);

		// Read the file into the allocated memory and close the file once completed
		size_t bytes = sceKernelRead(fileID, handle.data, s.st_size);
		SCE_AGC_ASSERT_MSG(bytes == s.st_size, "Unable to read skeleton %s", pathStr.c_str());
		sceKernelClose(fileID);

		// TODO: Make our own file format for skeletons, mesh and animations.

		// Parse the runtime skeleton 
		EdgeAnimSkeleton* edgeAnimSkeleton = (EdgeAnimSkeleton*)handle.data;
		Edge::Tools::Skeleton edgeSkeleton;
		Edge::Tools::ExtractSkeleton(edgeAnimSkeleton, edgeSkeleton);

		// Create a new skeleton and store it within the map
		std::unique_ptr<Skeleton> skeleton(new Skeleton());

		// Store the name of the skeleton
		skeleton->SetName(std::string(skeletonTag));

		std::vector<Joint>& joints = const_cast<std::vector<Joint>&>(skeleton->GetJoints());

		// Extract each joint from the skeleton, and store the name, pose and parent index.
		for (int32_t i = 0; i < edgeSkeleton.m_numJoints; ++i) {
			Joint joint;
			joint.Name = std::to_string(edgeSkeleton.m_jointNameHashes[i]);
			joint.ParentId = edgeSkeleton.m_parentIndices[i];
			auto basePose = edgeSkeleton.m_basePose[i];

			joint.InverseBindPose.Translation.x = basePose.m_translation.m_data[0];
			joint.InverseBindPose.Translation.y = basePose.m_translation.m_data[1];
			joint.InverseBindPose.Translation.z = basePose.m_translation.m_data[2];

			joint.InverseBindPose.Rotation = glm::quat(basePose.m_rotation.m_data[0], basePose.m_rotation.m_data[1],
				basePose.m_rotation.m_data[2], basePose.m_rotation.m_data[3]);

			joint.InverseBindPose.Scale.x = basePose.m_scale.m_data[0];
			joint.InverseBindPose.Scale.y = basePose.m_scale.m_data[1];
			joint.InverseBindPose.Scale.z = basePose.m_scale.m_data[2];

			skeleton->AddJoint(joint);
		}
		
		m_Skeleton[skeletonTag] = std::move(skeleton);
		return m_Skeleton[skeletonTag].get();
	}

	

	Animation* AGCAssetManager::LoadAnimationImpl(const wchar_t* filename, const std::string_view& animationTag)
	{
		std::filesystem::path path(SanitizeFilePath(filename));

		SKTBD_CORE_ASSERT(std::filesystem::exists(path), "Animation file does not exist!");
		std::string pathStr = path.string();

		// For simplicity, this is done with regular kernel file I/O and not with APR.
		SceKernelStat s = {};
		int32_t error, fileID;
		fileID = sceKernelOpen(path.c_str(), SCE_KERNEL_O_RDONLY, 0);
		SCE_AGC_ASSERT_MSG(fileID > 0, "Unable to open animation %s", pathStr.c_str());
		error = sceKernelFstat(fileID, &s);
		SCE_AGC_ASSERT_MSG(error == SCE_OK, "Unable to stat animation %s", pathStr.c_str());

		// EdgeAnimAnimation is 16-byte aligned
		const sce::Agc::SizeAlign sAlign = { (uint64_t)s.st_size, sizeof(int16_t) };
		auto handle = gAGCContext->GetMemAllocator()->TypedAlignedAllocate<EdgeAnimAnimation,sizeof(int16_t)>(s.st_size);

		// Read the file into the allocated memory and close the file once completed
		size_t bytes = sceKernelRead(fileID, handle.data, s.st_size);
		SCE_AGC_ASSERT_MSG(bytes == s.st_size, "Unable to read animation %s", pathStr.c_str());
		sceKernelClose(fileID);

		std::unique_ptr<Animation> animation(new AGCAnimation(handle));

		m_Animations[animationTag] = std::move(animation);

		return nullptr;
	}


	
}
