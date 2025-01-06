#include "sktbdpch.h"
#include "AudioFile.h"
#include "AudioRenderer.h"

#ifdef SKTBD_PLATFORM_PLAYSTATION
#include "Platforms/Playstation/PlaystationAudioInstance.h"
#endif // DEBUG

namespace Skateboard
{

	AudioFile* AudioFile::Create()
	{
		switch (AudioRenderer::GetAPI())
		{
			
#ifdef SKTBD_PLATFORM_PLAYSTATION
			case AudioRendererAPI_::AudioRendererAPI_PS5:
				return new PlaystationAudioInstance;
#endif
			default:
			case AudioRendererAPI_::AudioRendererAPI_None:
			return nullptr;
			[[fallthrough]];
		}
		
	}

	int AudioFile::ReadAudioFile(char* path)
	{
		return 0;
	}

}




