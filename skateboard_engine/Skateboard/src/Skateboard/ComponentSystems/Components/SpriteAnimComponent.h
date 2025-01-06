#pragma once
#include "Skateboard\Scene\Components.h"

class SpriteAnimComponent :
    public Skateboard::BaseComponent
{
public:

    friend class SpriteAnimatorSystem;

    //controlls, like pushbuttons on a cassete tape player
    bool playing = false;
    bool paused = false;
    bool loop = false;
    bool restart = false;

    //controls the flow of the FrameIncremen, multiplying it by -1 in case of renerse so that the animation plays backwards
    bool reverse = false;

    //mirors the animation in the respective axis, NOTE: this doesnt affect the animation playback direction, we will start walking backwards, for that see reverse :3
    bool flipX = false;
    bool flipY = false;

    //bool switched_track
    int new_track = -1;

    //what is the curent frame we displaying
    float ElapsedTime = 0;
    int32_t CurrentFrame = 0;

    //how fast we are moving FPS
    float Framerate;

    struct Track
    {
        //size of the frame expressed in Pixels
        int2 FrameSize;

        //where in the spritesheet are we starting from in Pixels
        int2 StartOffset;

        //frame count in a track
        int32_t FrameCount;

        //where are we going for the next frame, expressed as direction vector  so that on increment newFrame = StartOffset + FrameSize * Frameincrement * ++currentframe
        float2 FrameIncrement;
    };

    std::vector<Track> Tracks;

private:
    float2 UvOffset = { 0,0 };
    int CurrentTrack = -1;
};

