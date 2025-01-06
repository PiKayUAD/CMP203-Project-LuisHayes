#include "sktbdpch.h"
#include "Components.h"
#include "Entity.h"
#include "Skateboard/Memory/MemoryManager.h"
#include "Skateboard/Utilities/StringConverters.h"

namespace Skateboard {

	

	TransformComponent::~TransformComponent()
	{
		// TODO: Memory Manager Releate Resource
	}

	void TransformComponent::OnComponentAdded(Entity* entity)
	{
		auto& tagComponent = ECS::GetRegistry().get<TagComponent>(*entity);
		std::wstring debugName = L"WorldBuffer-" + ToWString(tagComponent.tag);
		//BufferId = MemoryManager::CreateConstantBuffer<WorldBuffer2D>(debugName);	// Not very happy with this approach as additional 16 bytes is being sent to the GPU -- not horrible but find a better solution.
	}

	// TODO : When the static mesh component is removed, we need to also remove it from the rendering list

	MaterialComponent::~MaterialComponent()
	{
		// TODO: Memory Manager Releate Resource
	}

	void MaterialComponent::OnComponentAdded(Entity* E)
	{
		auto& tagComponent = ECS::GetRegistry().get<TagComponent>(*E);
		std::wstring debugName = L"MaterialBuffer-" + ToWString(tagComponent.tag);
		//BufferId = MemoryManager::CreateStructuredBuffer<MaterialData>(debugName, 1);
			
	}

};