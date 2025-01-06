#pragma once
#include "Skateboard/Core.h"
#include "Skateboard/Renderer/CommonResources.h"
#include "Skateboard/Renderer/Model/Model.h"
#include "Skateboard/Renderer/Animation/Animator/Animator.h"
#include "Skateboard/Memory/VirtualAllocator.h"
#include "Skateboard/Utilities/StringId.h"

namespace Skateboard
{
	constexpr uint32_t MAX_TEXTURES = 2048;
	constexpr uint32_t MAX_VERTEX_BUFFERSIZE = 1024 * 1024 * 250; //250MBs

	#define DefaultTexKey "whitePixel"
	typedef StringIdTable TextureTable;
	typedef StringIdTable ModelTable;
	typedef StringIdTable AnimationTable;
	typedef StringIdTable FontTable;

	class DescriptorTable;

	class AssetManager
	{
		
	protected:
		AssetManager();
	public:
		static AssetManager* Create();
		static AssetManager& Singleton();
		virtual ~AssetManager() {}

		static const Texture* LoadTexture(const wchar_t* filename, const std::string_view& textureTag, TextureDimension_ resType = TextureDimension_Texture2D) { return Singleton().LoadTextureImpl(filename, textureTag, resType); }
		static Texture* CreateTextureFromData(const std::string_view& textureTag, const uint32_t& width, const uint32_t& height, void*  data) { return Singleton().CreateTextureFromDataImpl(textureTag, width, height, data); };
		static int32_t GetTexureMaxIndex() { return Singleton().m_TextureMaxIndex; }
		static const Texture* GetTexture(const std::string_view& name) { return Singleton().GetTextureImpl(name); }
		static Texture* GetDefaultTexture() { return Singleton().GetTextureImpl(DefaultTexKey); }

		static Texture* GetTextureBasedOnId(int32_t id)					{ return Singleton().GetTextureBasedOnIdImpl(id); }
		static uint32_t GetTextureId(const std::string_view& name)		{ return Singleton().GetTextureIdImpl(name); }

		static bool ReleaseTexture(const std::string_view& name);
		static bool ReleaseTexture(uint32_t id);

		static Mesh* LoadModel(const wchar_t* filename, const std::string_view& modelTag, const BufferLayout& ImportLayout, const std::string_view& obj_name = "", bool flipNormals = false) { return Singleton().LoadModelImpl(filename, modelTag, ImportLayout, obj_name, flipNormals); }
		static Mesh* GetModel(const std::string_view& name) { return Singleton().GetModelImpl(name); }

		static Animation* LoadAnimation(const wchar_t* filename, const std::string_view& animationTag) { return Singleton().LoadAnimationImpl(filename, animationTag); };
		static Animation* GetAnimation(const std::string_view& name) { return Singleton().GetAnimationImpl(name); };
		static Animation* GetAnimationById(uint32_t id) { return Singleton().GetAnimationByIdImpl(id); }

		static SkinnedMesh* LoadSkinnedMesh(const wchar_t* filename, const std::string_view& meshTag, bool flipNormals = false) { return Singleton().LoadSkeletalMeshImpl(filename, meshTag, flipNormals); };
		static SkinnedMesh* GetSkeletalMesh(const std::string_view& name) { return Singleton().GetSkeletalMeshImpl(name); };

		static Skeleton* LoadSkeleton(const wchar_t* filename, const std::string_view& skeletonTag) { return Singleton().LoadSkeletonImpl(filename, skeletonTag); };
		
		static Mesh* AddModelToMap(std::unique_ptr<Mesh>& model,  std::string& name) {  Singleton().m_Meshes.emplace(std::move(name), std::move(model)); return Singleton().m_Meshes[name].get(); };

		static ImFont* LoadFont(const wchar_t* filename, uint32_t sizeInPixels, const std::string_view& fonttag) { return Singleton().LoadFontImpl(filename,sizeInPixels, fonttag); };
		static ImFont* GetFont(const std::string_view& name) { return Singleton().GetFontImpl(name); };



	protected:
		// Abstracted Loaders
		virtual const Texture* LoadTextureImpl(const wchar_t* filename, const std::string_view& textureTag, TextureDimension_ resType) = 0;
		virtual Mesh* LoadModelImpl(const wchar_t* filename, const std::string_view& modelTag, const BufferLayout& Layout, const std::string_view& obj_name = "", bool flipNormals = false) = 0;
		virtual Animation* LoadAnimationImpl(const wchar_t* filename, const std::string_view& animationTag) = 0;
		
		virtual SkinnedMesh* LoadSkeletalMeshImpl(const wchar_t* filename, const std::string_view& meshTag, bool flipNormals = false) = 0;

		virtual Skeleton* LoadSkeletonImpl(const wchar_t* filename, const std::string_view& skeletonTag) = 0;

		virtual Texture* CreateTextureFromDataImpl(const std::string_view& textureTag, const uint32_t& width, const uint32_t& height, void* data) = 0;

		virtual ImFont* LoadFontImpl(const wchar_t* filename,uint32_t sizeInPixels, const std::string_view& modelTag) = 0;

	protected:
		Mesh* GetModelImpl(const std::string_view& name) {   return m_Meshes[name].get(); }

		Texture* GetTextureImpl(const std::string_view& name)		{ return &m_Textures[name]; }
		Texture* GetTextureBasedOnIdImpl(uint32_t id)				{ return &m_Textures[m_IdToTag[id]]; }
		uint32_t GetTextureIdImpl(const std::string_view& name)		{ return m_TagToId[name]; }

		Animation* GetAnimationImpl(const std::string_view& name)	{ return m_Animations[name].get(); }
		Animation* GetAnimationByIdImpl(uint32_t id)				{ return m_Animations[m_IdToTag[id]].get(); }

		SkinnedMesh* GetSkeletalMeshImpl(const std::string_view& name) { return m_SkinnedMesh[name].get(); }

		ImFont* GetFontImpl(const std::string_view& name) { return m_Fonts[name]; }

	protected:
		SingleResource<Buffer> m_StaticMeshVertexBuffer;
		MemoryUtils::BlockAllocator* m_VertexBufferSuballocator;

		int32_t m_TextureMaxIndex;

		std::unordered_map<std::string_view, std::unique_ptr<Mesh>> m_Meshes;

		std::unordered_map<std::string_view, std::unique_ptr<SkinnedMesh>> m_SkinnedMesh;
		std::unordered_map<std::string_view, std::unique_ptr<Skeleton>> m_Skeleton;

		std::unordered_map<std::string_view, std::unique_ptr<Primitive>> m_Primitives;
		std::unordered_map<std::string_view, std::unique_ptr<Animation>> m_Animations;

	//	std::unordered_map<std::string_view, SingleResource<Texture_Base>> m_TextureData;
		std::unordered_map<std::string_view, Texture> m_Textures;
		
		//doesnt need to be a smart pointer as ImGui Context holds the font atlas
		std::unordered_map<std::string_view, ImFont*> m_Fonts;

		TextureTable	m_TextureTable;
		ModelTable		m_ModelTable;
		AnimationTable	m_AnimationTable;
		FontTable		m_FontTable;

		std::unordered_map<uint32_t, std::string_view> m_IdToTag;
		std::unordered_map<std::string_view, uint32_t> m_TagToId;
		std::vector<uint32_t> m_AvailableIndices;

	};
}