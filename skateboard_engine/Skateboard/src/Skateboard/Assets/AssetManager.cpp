#include <sktbdpch.h>
#include "AssetManager.h"
//#include "Skateboard/Renderer/Renderer.h"

#include "Skateboard/Renderer/ResourceFactory.h"

#if defined(SKTBD_PLATFORM_WINDOWS)
#include "Platform/DirectX12/Assets/D3DAssetManager.h"
#elif defined(SKTBD_PLATFORM_PLAYSTATION)
#include "Platform/AGC/Assets/AGCAssetManager.h"
#endif

#define SKTBD_LOG_COMPONENT "ASSET_MANAGER"
#include "Skateboard/Log.h"

namespace Skateboard
{
	AssetManager::AssetManager()
		: m_TextureMaxIndex(-1)
	{
		// Initialize the available indices
		m_AvailableIndices.reserve(MAX_TEXTURES);
		for (int32_t i = 0; i < MAX_TEXTURES; ++i) {
			m_AvailableIndices.push_back(i);
		}

		BufferDesc desc;
		desc.Init(MAX_VERTEX_BUFFERSIZE, ResourceAccessFlag_GpuRead);

		ResourceFactory::CreateBufferResource(m_StaticMeshVertexBuffer, L"AssetManager:VertexBuffer", desc);

		MemoryUtils::VIRTUAL_BLOCK_DESC aldesc{};
		aldesc.Size = MAX_VERTEX_BUFFERSIZE;

		MemoryUtils::CreateBlock(&aldesc,&m_VertexBufferSuballocator);

	}

	AssetManager* AssetManager::Create()
	{
#if defined SKTBD_PLATFORM_WINDOWS
			return new D3DAssetManager();
#elif defined SKTBD_PLATFORM_PLAYSTATION
			return new AGCAssetManager();
#endif
	}

	AssetManager& AssetManager::Singleton()
	{
		static std::unique_ptr<AssetManager> singleton(Create());
		return *singleton;
	}

	bool AssetManager::ReleaseTexture(uint32_t id)
	{
		return Singleton().ReleaseTexture(Singleton().m_IdToTag[id]);
	}

	bool AssetManager::ReleaseTexture(const std::string_view& name)
	{
		if (!Singleton().m_Textures.count(name))
		{
			return false;
		}

		// Do not attempt to delete the texture from memory by force!
		// Let the internal API clean up resources.
		//Singleton().m_Textures[name]->Release();
		Singleton().m_Textures.erase(name);

		// Re-add the index back to available slots.
		Singleton().m_AvailableIndices.push_back(Singleton().m_TagToId[name]);
		
		// Remove references from the textures map.
		Singleton().m_IdToTag.erase(Singleton().m_TagToId[name]);
		Singleton().m_TagToId.erase(name);

		return true;
	}

}