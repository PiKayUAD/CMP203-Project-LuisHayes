#include "sktbdpch.h"
#include "SpriteAnimatorSystem.h"
#include "Skateboard/Scene/ECS.h"
#include "Skateboard/ComponentSystems/Components/SpriteAnimComponent.h"
#include "Skateboard/Renderer/SpriteComponent.h"
#include "Skateboard/Platform.h"

#define SKTBD_LOG_COMPONENT "SPRITE ANIMATOR SYSTEM"
#include "Skateboard/Log.h" 

void SpriteAnimatorSystem::Init(Skateboard::Scene* scn)
{
	//nothing to init here;
}

void SpriteAnimatorSystem::HandleInput(Skateboard::TimeManager* time, Skateboard::Scene* scn)
{
}

void SpriteAnimatorSystem::RunUpdate(Skateboard::TimeManager* time, Skateboard::Scene* scn)
{
	auto AnimatedSprites = Skateboard::ECS::GetRegistry().view<SpriteAnimComponent, Skateboard::SpriteComponent>();

	AnimatedSprites.each([time](const auto entity, SpriteAnimComponent& animateur, Skateboard::SpriteComponent& animatee)
		{
			//if the track changed updated the members of the animator
			if (animateur.new_track != animateur.CurrentTrack)
			{
				animateur.CurrentTrack = animateur.new_track;

				float2 t_size;
				t_size.x = animatee.GetRenderTexture()->GetWidth();
				t_size.y = animatee.GetRenderTexture()->GetHeight();

				float2 framesize = animateur.Tracks[animateur.CurrentTrack].FrameSize;
				float2 offset = animateur.Tracks[animateur.CurrentTrack].StartOffset;

				animateur.UvOffset = offset / t_size;
				float2 newScale = framesize / t_size;

				if(animatee.GetUvScale() != newScale)
				animatee.SetUvScale(newScale);

				animatee.SetUvOffset(animateur.UvOffset);

				animateur.ElapsedTime = 0;
			}

			//if we want to start from the beginning
			if (animateur.restart)
			{
				animateur.CurrentFrame = 0;
				animateur.restart = false;
			}

			//if we want to mirror the frame X;
			if (animateur.flipX)
			{
				animatee.SetUvScale({ -animatee.GetUvScale().x, animatee.GetUvScale().y });
				animateur.flipX = false;
			}

			//mirror Y
			if (animateur.flipY)
			{
				animatee.SetUvScale({ animatee.GetUvScale().x, -animatee.GetUvScale().y });
				animateur.flipY = false;
			}

			//if playing the animation, when we pass the elapsed time we want to update the animation offset in the sprite sheet
			if (animateur.playing && !animateur.paused)
			{
				animateur.ElapsedTime += time->DeltaTime();

				SpriteAnimComponent::Track track = animateur.Tracks[animateur.CurrentTrack];

				if (animateur.ElapsedTime > 1.f / animateur.Framerate)
				{
					animateur.ElapsedTime = 0;
					++animateur.CurrentFrame;

					if (animateur.CurrentFrame < track.FrameCount)
					{
						//SKTBD_APP_TRACE("animation frame: {}", animateur.CurrentFrame);
						animatee.SetUvOffset(animateur.UvOffset + animatee.GetUvScale() * animateur.CurrentFrame * track.FrameIncrement * ((animateur.reverse) ? -1 : 1));
					}
					else if (animateur.loop)
					{
						animateur.restart = true;
					}
					else
					{
						animateur.playing = false;
					}
				}
			}
		}
	);
}
