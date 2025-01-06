#pragma once
#include "InternalFormats.h"
#include "Skateboard/Renderer/GraphicsSettingsDefines.h"

#include "Skateboard/Log.h"

namespace Skateboard
{
	// A 'GpuResource' is a resource that will be transferred to the platform GPU in some way

	class GPUResource
	{
		friend class ResourceFactory;

	public:
		GPUResource(GPUResourceType_ type) : m_Type(type), m_DebugName(L"") {};

#ifndef SKTBD_SHIP
		GPUResource(const std::wstring& debugName, GPUResourceType_ type) : m_DebugName(debugName), m_Type(type) {}
		const std::wstring& GetDebugName() const { return m_DebugName; }
		void SetDebugName(const std::wstring& n_name) { m_DebugName = n_name; };
#else
		GpuResource(const std::wstring& debugName, GPUResourceType_ type) {}
		const std::wstring& GetDebugName() const { return L""; }
		void SetDebugName(const std::wstring& n_name) {}

#endif
		GPUResourceType_ GetType() const { return m_Type; }
		virtual ~GPUResource() {};

		//returns a copy of the handle, only good for immediate use
		virtual void* GetResource() const = 0;

		//returns a pointer to the handle, allows us to modify the contents
		virtual void* GetResourcePtr() = 0;

	protected:
#ifndef SKTBD_SHIP
		std::wstring m_DebugName;
#endif // !SKTBD_SHIP
		GPUResourceType_ m_Type;

		static constexpr const uint32_t GetHandleCount() { return  0; }
	};

	//resource type
	typedef void* ResourceHandle;

	//Single Resource mixin, Holds one sigle memory handle, Resource must derive from GPUResource Class
	template<class Resource, typename ResourceMemHandleType = ResourceHandle>
	class SingleResource : public Resource
	{
	//	DISABLE_COPY(SingleResource)
		friend class ResourceFactory;
		friend Resource;
		static_assert(std::is_base_of<GPUResource, Resource>::value);
		
	public:
		template<typename... Args>
		SingleResource(Args&&... args) : Resource(std::forward<Args>(args)...) { ZeroMemory(&m_MemoryHandle, sizeof(ResourceMemHandleType)); };

		static constexpr const uint32_t GetHandleCount() { return  1; }

		void* GetResource() const override { return m_MemoryHandle; }
		void* GetResourcePtr() override { return &m_MemoryHandle; };

		void ForEachMemoryHandle(const std::function<void(ResourceMemHandleType*)>& MemFunc)
		{
			MemFunc(&m_MemoryHandle);
		};

	protected:
		ResourceMemHandleType m_MemoryHandle;
	};

	//Multiple resource mixin, Holds multiple memory handles, Resource must derive from GPUResource Class
	template<class Resource, uint32_t ResourceCount = GRAPHICS_SETTINGS_NUMFRAMERESOURCES, typename ResourceMemHandleType = void*>
	class MultiResource : public Resource
	{
	//	DISABLE_COPY(MultiResource)
		friend class ResourceFactory;
		friend Resource;
		static_assert(std::is_base_of<GPUResource, Resource>::value);
		static_assert(ResourceCount > 1);

	public:
		template<typename... Args>
		MultiResource(Args&&... args) : Resource(std::forward<Args>(args)...), m_internalCounter(0) { ZeroMemory(m_MemoryHandles.data(), sizeof(ResourceMemHandleType) * ResourceCount); };

		static constexpr uint32_t GetHandleCount() { return  ResourceCount; }

		std::array<ResourceMemHandleType, ResourceCount>& GetResourceArray()
		{
			return m_MemoryHandles;
		}

		void* GetResource() const override
		{
			return m_MemoryHandles[m_internalCounter];
		}

		void* GetResourcePtr() override 
		{
			return &m_MemoryHandles[m_internalCounter];
		};

		void SetCounter(uint32_t new_idx)
		{
			SKTBD_CORE_ASSERT(new_idx < ResourceCount, "Frame Index Out of Range for This Resource");
			m_internalCounter = new_idx;
		}

		void IncrementCounter()
		{
			m_internalCounter = (m_internalCounter + 1) % ResourceCount;
		}

		ResourceMemHandleType operator[](const uint32_t& idx) const
		{
			return m_MemoryHandles[idx];
		}

		void ForEachMemoryHandle(const std::function<void(ResourceMemHandleType*)>& MemFunc)
		{
			for (int i = 0; i < ResourceCount; i++)
			{
				MemFunc(&m_MemoryHandles[i]);
			}
		}

	protected:
		std::array<ResourceMemHandleType, ResourceCount> m_MemoryHandles;
		uint32_t m_internalCounter;
	};
};