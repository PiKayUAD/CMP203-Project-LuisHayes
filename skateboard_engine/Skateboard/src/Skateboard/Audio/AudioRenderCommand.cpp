#include "sktbdpch.h"
#include "AudioRenderCommand.h"
//#include "AudioRenderCommand.h"
#include "Api/AudioRendererAPI.h"

#ifdef SKTBD_PLATFORM_PLAYSTATION
#include "Platforms/Playstation/PlaystationAudioRendererAPI.h"
#endif // SKATEBOARD_PLATFORM_PLAYSTATION



namespace Skateboard
{
	AudioRendererAPI* AudioRenderCommand::m_AudioRendererAPI{ nullptr };

	AudioRenderCommand::~AudioRenderCommand()
	{
		delete m_AudioRendererAPI;
		m_AudioRendererAPI = nullptr;
	}

	void AudioRenderCommand::InitPlatform()
	{
#if defined(SKTBD_PLATFORM_WINDOWS)
		// TODO: Need to make this switch based on selected api.
		// Note: Hard coded for now.
		//AudioRendererAPI = new D3DRenderingApi();
#elif defined(SKTBD_PLATFORM_PLAYSTATION)
		
		m_AudioRendererAPI = new PlaystationAudioRendererAPI();
#endif
	}

	void AudioRenderCommand::UpdateAudioRenderer()
	{
		m_AudioRendererAPI->UpdateAudioRenderer();
	}

	void AudioRenderCommand::LoadAudio(char* path, char* ID)
	{
		m_AudioRendererAPI->LoadAudio(path, ID);
	}

	void AudioRenderCommand::PlayAudio(char* ID,AudioOutputType OutputType)
	{
		m_AudioRendererAPI->PlayAudio(ID, OutputType);
	}

	void AudioRenderCommand::StopAudio(char* ID)
	{
		m_AudioRendererAPI->StopAudio(ID);
	}

	void AudioRenderCommand::End()
	{
	}

	void AudioRenderCommand::CreateContext(AudioOutputType outputType)
	{
		m_AudioRendererAPI->CreateContext(outputType);
	}


}