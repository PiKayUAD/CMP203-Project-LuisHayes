#include "sktbdpch.h"
#include "RenderCommand.h"

namespace Skateboard
{
	RenderingApi* RenderCommand::Api{ nullptr };

	void RenderCommand::Init()
	{
		Api = GraphicsContext::Context->GetAPI();
	}

}
