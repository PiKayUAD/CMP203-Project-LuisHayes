#pragma once
//#include "Platforms/Playstation/WavReaderUtility.h"
//#include <audio_out2.h>
#include "AudioFile.h"
#include "AudioRenderCommand.h"
namespace Skateboard
{
	//HARDCODED_ Current platform API
	enum class AudioRendererAPI_
	{
		AudioRendererAPI_None = 0,
		AudioRendererAPI_Windows = 1,
		AudioRendererAPI_PS5 = 2
	};


	class AudioRenderer
	{
	public:
		//Audio Renderer Functions
		static void InitPlatform();
		//static void Init(SceUserServiceUserId USERID, uint16_t PORT, uint32_t PORTDATAFORMAT);
		static void CreateContext(AudioOutputType outputType);
		static void End();
		static void LoadAudio(char* path, char* ID);
		//static void PlayAudio(uintptr_t tmpWavFileData, SceAudioOut2WaveformInfo tempinfo);
		static void PlayAudio(char* ID, AudioOutputType OutputType);
		static void StopAudio(char* ID);

		static void UpdateAudioRenderer();
		//API
		static AudioRendererAPI_ GetAPI() { return s_AudioAPI; }
	private:
		static AudioRendererAPI_ s_AudioAPI;
	};



}