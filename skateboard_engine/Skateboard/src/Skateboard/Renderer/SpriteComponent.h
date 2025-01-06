#pragma once
#include "Skateboard/Scene/Components.h"

namespace Skateboard {

	class SpriteComponent final : public  BaseComponent
	{
		friend class Renderer2D;

	public:
		// Create a default white coloured sprite. The texture can be set later.
		SpriteComponent():Colour(1) { CreateDefault(); };			
		// Load a new texture from disk and set that to the sprite currently used
		SpriteComponent( const std::wstring& fileName) :Colour(1) { Create(fileName); };	
		// Create a sprite and set texture from the asset manager. This requires you to load a texture beforehand and provide that texture name here.
		SpriteComponent( const std::string& texName) :Colour(1) { Create(texName); };		
		SpriteComponent(entt::entity entityId) : BaseComponent(entityId) { CreateDefault(); };
		SpriteComponent(const SpriteComponent&) = default;
	public:
		bool SetSizeByTexture();

		// Set texture from texture pointer but do not resize sprite.
		void SetTexture(Texture* tex) { p_texture = tex; };
		// Set texture from Asset Manager and then resize the sprite
		void SetTexture(const std::string_view& key);

		float2 GetSize() const { return m_size; };
		auto GetSpriteID() const { return m_spriteId; };
		const Texture* GetRenderTexture() const { return p_texture; };

		void SetColour(float4 colour);
		void SetSize(const float2 n_size);

		void SetUvScale(const float2 n_scale);
		void SetUvOffset(const float2 n_offset);

		const float2& GetUvScale() { return m_uvScale; };
		const float2& GetUvOffset() { return m_uvOffset; }

	public:
		float4 Colour;
		

	private:
		void Create(const std::string& fileName);
		void Create(const std::wstring& fileName);
		void CreateDefault();

		void UpdateScaleSizeBuffer(float4 SizeAndUvScale); // updates size of the sprite and size of its uv box

		float2 m_uvScale;
		float2 m_uvOffset;

		float2 m_size;
		uint32_t m_spriteId;

		const Texture* p_texture;

		uint32_t ColourBufferId;
		uint32_t SizeBufferId;
		uint32_t OffsetBufferId;

		std::vector<SpriteFrame> m_frames;
	};
}