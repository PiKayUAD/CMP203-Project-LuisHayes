#pragma once
//#include "Skateboard/Audio/Platforms/Playstation/WavReaderUtility.h"
#include "AudioFile.h"

namespace Skateboard
{
	class AudioRendererAPI;

	class AudioRenderCommand
	{
		friend class AudioRenderer;

	public:
		~AudioRenderCommand();


		static void PlayAudio(char * ID, AudioOutputType OutputType);

		static void StopAudio(char * ID);

		static void End();

		static void CreateContext(AudioOutputType outputType);

		static void InitPlatform();

		static void LoadAudio(char* path, char* ID);

		static void UpdateAudioRenderer();
	private:

		static AudioRendererAPI* m_AudioRendererAPI;





	};

}