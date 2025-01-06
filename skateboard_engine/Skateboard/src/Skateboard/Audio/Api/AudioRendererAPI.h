#pragma once

//#include <audio_out2.h>
//#include "Skateboard/Audio/Platforms/Playstation/WavReaderUtility.h"
#include "Skateboard/Audio/AudioFile.h"
#include "Skateboard/Audio/AudioRenderCommand.h"

namespace Skateboard
{

	class AudioRendererAPI
	{
	public:

		virtual ~AudioRendererAPI() {};

		//Windows Init pure function
		virtual void CreateContext(AudioOutputType outputType)= 0;

		//Playstation Init pure function
		//virtual void Init(SceUserServiceUserId USERID, uint16_t PORT, uint32_t PORTDATAFORMAT) = 0;

		virtual void EndScene() = 0;

		//Windows Play pure function
		virtual void LoadAudio(char* path, char* ID) = 0;
		
		//Playstation Play Audio pure function
		virtual void PlayAudio(char* ID, AudioOutputType OutputType) = 0;

		virtual void StopAudio(char* ID) = 0;

		virtual void UpdateAudioRenderer() = 0;




	protected:

	};


}