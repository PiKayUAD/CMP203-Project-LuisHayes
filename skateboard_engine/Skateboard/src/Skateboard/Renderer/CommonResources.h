#pragma once

#include "Buffer.h"
#include "View.h"
#include "Pipeline.h"

namespace Skateboard
{
	// COMMON USECASES Static Resources 
	
	class FrameBuffer
	{
		//static InitAS

		MultiResource<RenderTargetView> m_RTV;
		MultiResource<DepthStencilView> m_DSV;

	protected:
		MultiResource<TextureBuffer> m_RTVData;
		MultiResource<TextureBuffer> m_DSVData;
	};

	using VertexBuffer = VertexBufferView;
	using IndexBuffer = IndexBufferView;

	using Texture = SingleResource<TextureView>;
	//	using StructuredBuffer = SingleResource<StructuredBufferView>;
	using TLAS = SingleResource<TopLevelAccelerationStructureView>;
	using BLAS = SingleResource<BottomLevelAccelerationStructureView>;
}