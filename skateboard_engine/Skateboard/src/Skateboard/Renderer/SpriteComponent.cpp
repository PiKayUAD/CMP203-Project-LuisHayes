#include "sktbdpch.h"
#include "SpriteComponent.h"
#include "Skateboard/Renderer/Renderer2D.h"
#include "Skateboard/Assets/AssetManager.h"
#include "Skateboard/Memory/MemoryManager.h"
#include "entt.hpp"

namespace Skateboard
{
	const float SpriteTexDivisionFactor = 100.f;
	static uint32_t SpriteCount = 0;

	bool SpriteComponent::SetSizeByTexture()
	{
		if (p_texture)
		{
			//m_size.x = (float)p_texture->GetWidth() / SpriteTexDivisionFactor;
			//m_size.y = -(float)p_texture->GetHeight() / SpriteTexDivisionFactor;

			UpdateScaleSizeBuffer({ m_size,m_uvScale });
			return true;
		}
		return false;
	}

	void SpriteComponent::SetTexture(const std::string_view& key)
	{
		p_texture = AssetManager::GetTexture(key);
		SetSizeByTexture();
	}


	void SpriteComponent::SetColour(float4 colour)
	{
		Colour = colour;
		//MemoryManager::UploadData(ColourBufferId, 0,  &colour );
	}

	void SpriteComponent::SetSize(const float2 n_size)
	{
		m_size = n_size;
		UpdateScaleSizeBuffer({ m_size,m_uvScale });
	}

	void SpriteComponent::SetUvScale(const float2 n_scale)
	{
		m_uvScale = n_scale;
		UpdateScaleSizeBuffer({ m_size,m_uvScale });
	}

	void SpriteComponent::SetUvOffset(const float2 n_offset)
	{
		m_uvOffset = n_offset;
		//MemoryManager::UploadData(OffsetBufferId, 0, &m_uvOffset);
	}

	//helper to upload dataabout the component;
	void SpriteComponent::UpdateScaleSizeBuffer(float4 SizeAndUvScale)
	{
		//MemoryManager::UploadData(SizeBufferId, 0, &SizeAndUvScale); 
	};

	void SpriteComponent::Create(const std::wstring& fileName)
	{
		m_spriteId = SpriteCount++;
		p_texture = AssetManager::LoadTexture(fileName.c_str(), std::to_string(m_spriteId) + "_sprite");
		SetSizeByTexture();
		//ColourBufferId = MemoryManager::CreateConstantBuffer<float4>(std::wstring(L"SpriteColour_")+std::to_wstring(m_spriteId));

		//SizeBufferId = MemoryManager::CreateConstantBuffer<float4>(std::wstring(L"SpriteSize_") + std::to_wstring(m_spriteId));
		//OffsetBufferId = MemoryManager::CreateConstantBuffer<float2>(std::wstring(L"SpriteOfft_") + std::to_wstring(m_spriteId));
		SetColour(float4(1, 1, 1, 1));
		SetSize({ 1,1 });
		SetUvOffset({ 0,0 });
		SetUvScale({ 1,1 });
	}
	void SpriteComponent::Create(const std::string& texName)
	{
		m_spriteId = SpriteCount++;
		SetTexture(texName);
		//ColourBufferId = MemoryManager::CreateConstantBuffer<float4>(std::wstring(L"SpriteColour_") + std::to_wstring(m_spriteId));

		//SizeBufferId = MemoryManager::CreateConstantBuffer<float4>(std::wstring(L"SpriteSize_") + std::to_wstring(m_spriteId));
		//OffsetBufferId = MemoryManager::CreateConstantBuffer<float2>(std::wstring(L"SpriteOfft_") + std::to_wstring(m_spriteId));
		SetColour(float4(1, 1, 1, 1));
		SetSize({ 1,1 });
		SetUvOffset({ 0,0 });
		SetUvScale({ 1,1 });
		
	}

	void SpriteComponent::CreateDefault()
	{
		m_spriteId = SpriteCount++;
		//p_texture = Renderer2D::GetDefaultWhiteTexture();
		m_size.x = 1;
		m_size.y = 1;

		//ColourBufferId = MemoryManager::CreateConstantBuffer<float4>(std::wstring(L"SpriteColour_") + std::to_wstring(m_spriteId));
		//SizeBufferId = MemoryManager::CreateConstantBuffer<float4>(std::wstring(L"SpriteSize_") + std::to_wstring(m_spriteId));
		//OffsetBufferId = MemoryManager::CreateConstantBuffer<float2>(std::wstring(L"SpriteOfft_") + std::to_wstring(m_spriteId));

		SetColour(float4(1, 1, 1, 1));
		SetSize({ 1.f,1.f });
		SetUvOffset({ 0.f,0.f });
		SetUvScale({ 1,1 });
	}
}