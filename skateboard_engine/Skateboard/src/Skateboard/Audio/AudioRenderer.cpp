
#include "AudioRenderer.h"
#include "AudioRenderCommand.h"
#include "sktbdpch.h"
#include "AudioFile.h"
namespace Skateboard
{
	// Hard coded for now, different API depending on platform application is run on
	#if defined(SKTBD_PLATFORM_WINDOWS)
		AudioRendererAPI_ AudioRenderer::s_AudioAPI = AudioRendererAPI_::AudioRendererAPI_Windows;
	#elif defined(SKTBD_PLATFORM_PLAYSTATION)
		AudioRendererAPI_ AudioRenderer::s_AudioAPI = AudioRendererAPI_::AudioRendererAPI_PS5;
	#endif
		
	void AudioRenderer::InitPlatform()
	{
		AudioRenderCommand::InitPlatform();
	}
	
	void AudioRenderer::CreateContext(AudioOutputType outputType)
	{
		AudioRenderCommand::CreateContext(outputType);
	}

	void AudioRenderer::End()
	{
		AudioRenderCommand::End();
	}

	void AudioRenderer::LoadAudio(char* path, char* ID)
	{
		AudioRenderCommand::LoadAudio(path, ID);
	}

	void AudioRenderer::PlayAudio(char *ID, AudioOutputType OutputType)
	{
		AudioRenderCommand::PlayAudio(ID, OutputType);
	}

	void AudioRenderer::StopAudio(char* ID)
	{
		AudioRenderCommand::StopAudio(ID);
	}

	void AudioRenderer::UpdateAudioRenderer()
	{
		AudioRenderCommand::UpdateAudioRenderer();
	}


}