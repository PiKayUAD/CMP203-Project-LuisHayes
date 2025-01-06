#pragma once
#include "Skateboard/Renderer/Buffer.h"
#include "Skateboard/Renderer/View.h"
#include "Skateboard/Platform.h"

namespace Skateboard
{
	class ResourceFactory
	{
		friend class GraphicsContext;

	public:
		template<typename BufferResource>
		static SKTBDR CreateBufferResource(BufferResource& resource, const std::wstring& debugName, BufferDesc& bufferDesc)
		{
			if constexpr (BufferResource::GetHandleCount() == 0) return E_INVALIDARG;
			static_assert(std::is_convertible<BufferResource*, GPUResource*>::value);
			resource = BufferResource(debugName, bufferDesc);
			resource.ForEachMemoryHandle(GraphicsContext::Context->GetBufferMemoryInitializer(bufferDesc));

#ifdef SKTBD_DEBUG
			auto regFunc = GraphicsContext::Context->RegisterMemoryResource(resource.GetType());

			if constexpr (BufferResource::GetHandleCount() > 1)
				resource.ForEachMemoryHandle([regFunc, debugName, i = 0](void** mem) mutable {regFunc(mem, debugName + std::to_wstring(i)); ++i; });
			else
				resource.ForEachMemoryHandle([regFunc, debugName](void** mem) {regFunc(mem, debugName); });
#endif // SKTBD_DEBUG

			return OKEYDOKEY;
		}

		template<typename TextureResource>
		static SKTBDR CreateTextureResource(TextureResource& resource, const std::wstring& debugName, TextureDesc& textureDesc)
		{
			static_assert(std::is_convertible<TextureResource*, GPUResource*>::value);
			resource = TextureResource(debugName, textureDesc);
			resource.ForEachMemoryHandle(GraphicsContext::Context->GetTextureMemoryInitializer(textureDesc));

#ifdef SKTBD_DEBUG
			auto regFunc = GraphicsContext::Context->RegisterMemoryResource(resource.GetType());

			if constexpr (TextureResource::GetHandleCount() > 1)
				resource.ForEachMemoryHandle([regFunc, debugName, i = 0](void** mem) mutable {regFunc(mem, debugName + std::to_wstring(i)); ++i; });
			else
				resource.ForEachMemoryHandle([regFunc, debugName](void** mem) {regFunc(mem, debugName); });
#endif // SKTBD_DEBUG

			return OKEYDOKEY;
		}


		template<typename Resource, typename View>
		static SKTBDR CreateRenderTargetView(View& view, const std::wstring& debugName, Resource* resource, RenderTargetDesc* viewDesc)
		{
			if constexpr (Resource::GetHandleCount() == View::GetHandleCount() && View::GetHandleCount() == 1)
			{
				view = SingleResource<RenderTargetView>(debugName, viewDesc);
				view.ForEachMemoryHandle(GraphicsContext::Context->CreateRenderTargetView(viewDesc));
				view.m_ParentResource = resource;
				return OKEYDOKEY;
			}
			else if constexpr (Resource::GetHandleCount() == View::GetHandleCount())
			{
				view = MultiResource<RenderTargetView, Resource::GetHandleCount()>(debugName, viewDesc);

				auto init = [i = 0, mem_handle = resource->GetResourceArray(), viewDesc](void** memHandle) mutable
				{
					GraphicsContext::Context->CreateRenderTargetView(viewDesc)(mem_handle[i], memHandle);
					i++;
				};

				view.ForEachMemoryHandle(init);
				view.m_ParentResource = resource;
				return OKEYDOKEY;
			}
			else return OOPS_RESOURCESIZEMISSMATCH;
		}

		template<typename Resource, typename View>
		static SKTBDR CreateDepthRenderTargetView(View& view, const std::wstring& debugName, Resource* resource, DepthStencilDesc* viewDesc)
		{
			if constexpr (Resource::GetHandleCount() == View::GetHandleCount() && View::GetHandleCount() == 1)
			{
				view = SingleResource<DepthStencilView>(debugName, viewDesc);
				view.ForEachMemoryHandle(GraphicsContext::Context->CreateDepthRenderTargetView(resource->GetResource(), viewDesc));
				view.m_ParentResource = resource;
				return OKEYDOKEY;
			}
			else if constexpr (Resource::GetHandleCount() == View::GetHandleCount())
			{
				view = MultiResource<DepthStencilView, Resource::GetHandleCount()>(debugName, viewDesc);

				auto init = [i = 0, mem_handle = resource->GetResourceArray(), viewDesc](void** memHandle) mutable
					{
						GraphicsContext::Context->CreateDepthRenderTargetView(mem_handle[i], viewDesc)(memHandle);
						++i;
					};

				view.ForEachMemoryHandle(init);
				view.m_ParentResource = resource;
				return OKEYDOKEY;
			}
			else return OOPS_RESOURCESIZEMISSMATCH;
		}

		template<typename Resource,typename View>
		static SKTBDR CreateTextureView(View& view, const std::wstring& debugName, Resource* resource, ViewAccessType_ AccessType, TextureViewDesc* viewDesc)
		{
			if constexpr (Resource::GetHandleCount() == View::GetHandleCount() && View::GetHandleCount() == 1)
			{
				view = SingleResource<TextureView>(debugName, AccessType, viewDesc);
				view.ForEachMemoryHandle(GraphicsContext::Context->CreateTextureView(resource->GetResource(), AccessType, viewDesc));
				view.m_ParentResource = resource;
				return OKEYDOKEY;
			}
			else if constexpr (Resource::GetHandleCount() == View::GetHandleCount())
			{
				view = MultiResource<TextureView, Resource::GetHandleCount()>(debugName, AccessType, viewDesc);

				auto init = [i = 0, mem_handle = resource->GetResourceArray(), AccessType, viewDesc](void** memHandle) mutable
					{
						GraphicsContext::Context->CreateTextureView(mem_handle[i], AccessType, viewDesc)(memHandle);
						i++;
					};

				view.ForEachMemoryHandle(init);
				view.m_ParentResource = resource;
				return OKEYDOKEY;
			}
			else return OOPS_RESOURCESIZEMISSMATCH;
		}

		template<typename Resource, typename View>
		static SKTBDR CreateBufferView(View& view,const std::wstring& debugName, Resource* resource, ViewAccessType_ AccessType, const BufferViewDesc& viewDesc)
		{
			if constexpr (Resource::GetHandleCount() == View::GetHandleCount() && View::GetHandleCount() == 1)
			{
				view = SingleResource<BufferView>(debugName, viewDesc, AccessType);
				view.ForEachMemoryHandle(GraphicsContext::Context->CreateBufferView(resource->GetResource(), AccessType, viewDesc));
				view.m_ParentResource = resource;
				return OKEYDOKEY;
			}
			else if constexpr (Resource::GetHandleCount() == View::GetHandleCount())
			{
				view = MultiResource<BufferView, Resource::GetHandleCount()>(debugName, viewDesc, AccessType);

				auto init = [i = 0, mem_handle = resource->GetResourceArray(), viewDesc, AccessType](void** memHandle) mutable
					{
						GraphicsContext::Context->CreateBufferView(mem_handle[i], AccessType, viewDesc)(memHandle);
						i++;
					};

				view.ForEachMemoryHandle(init);
				view.m_ParentResource = resource;
				return OKEYDOKEY;
			}
			else return OOPS_RESOURCESIZEMISSMATCH;
		}
	};
};