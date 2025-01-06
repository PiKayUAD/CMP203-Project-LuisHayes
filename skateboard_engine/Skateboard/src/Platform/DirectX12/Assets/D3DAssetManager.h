#pragma once
#include "Platform/DirectX12/D3D.h"
#include "Skateboard/Assets/AssetManager.h"
#include "Platform/DirectX12/Memory/D3DDescriptorTable.h"

namespace Skateboard
{
	class D3DAssetManager : public AssetManager
	{
	public:
		D3DAssetManager();
		virtual ~D3DAssetManager() final override {}

	private:
		virtual const Texture* LoadTextureImpl(const wchar_t* filename, const std::string_view& textureTag, TextureDimension_ resType);

		virtual Mesh* LoadModelImpl(const wchar_t* filename, const std::string_view& modelTag, const BufferLayout& bufferLayout,  const std::string_view& obj_name = "", bool flipNormals = false);

		virtual Animation* LoadAnimationImpl(const wchar_t* filename, const std::string_view& animationTag);

		virtual SkinnedMesh* LoadSkeletalMeshImpl(const wchar_t* filename, const std::string_view& meshTag, bool flipNormals = false);

		virtual Skeleton* LoadSkeletonImpl(const wchar_t* filename, const std::string_view& skeletonTag);

		virtual Texture* CreateTextureFromDataImpl(const std::string_view& textureTag, const uint32_t& width, const uint32_t& height, void* data);

		virtual ImFont* LoadFontImpl(const wchar_t* filename, uint32_t sizeInPixels, const std::string_view& modelTag);
	};
}