#pragma once
#include <unordered_map>
#include "Skateboard/Assets/AssetManager.h"

struct usint2
{
	unsigned short int x, y;
};

namespace PackFile
{
	class Package;
}

namespace Skateboard
{

	class AGCAssetManager final : public AssetManager
	{
	public:
		AGCAssetManager();
		virtual ~AGCAssetManager() final override {}

		virtual Texture*		LoadTextureImpl(const wchar_t* filename, const std::string_view& textureTag, TextureDimension_ resType) final override;
		virtual Mesh*			LoadModelImpl(const wchar_t* filename, const std::string_view& modelTag, bool flipNormals = false) final override;
		virtual Animation*		LoadAnimationImpl(const wchar_t* filename, const std::string_view& animationTag) final override;
		virtual SkinnedMesh*	LoadSkeletalMeshImpl(const wchar_t* filename, const std::string_view& meshTag, bool flipNormals = false) final override;
		virtual Skeleton*		LoadSkeletonImpl(const wchar_t* filename, const std::string_view& skeletonTag) final override;

		virtual Texture* CreateTextureFromDataImpl(const std::string_view& textureTag, const uint32_t& width, const uint32_t& height, void* data) final override;

		virtual ImFont* LoadFontImpl(const wchar_t* filename, uint32_t sizeInPixels, const std::string_view& fontTag) final override;

	private:
		PackFile::Package LoadPackage(const wchar_t* filename);

		//bool LoadTextures(const PackFile::Package& package, std::vector<>);

	};
}