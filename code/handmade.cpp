#include "handmade.h"

internal void
GameOutputSound(game_sound_output_buffer *SoundBuffer, int ToneHz)
{
	local_persist real32 tSine;
	int16 ToneVolume = 3000;
	int WavePeriod = SoundBuffer->SamplesPerSecond/ToneHz;
	
	int16 *SampleOut = SoundBuffer->Samples;
	for(int SampleIndex = 0; 
		SampleIndex < SoundBuffer->SampleCount; 
		++SampleIndex)
	{
		real32 SineValue = sinf(tSine);
		int16 SampleValue = (int16)(SineValue * ToneVolume);
		*SampleOut++ = SampleValue;
		*SampleOut++ = SampleValue;                  

		tSine += 2.0f*Pi32*1.0f/(real32)WavePeriod;
	}
}

internal void 
RenderWeirdGradient(game_offscreen_buffer *Buffer, int BlueOffset, int GreenOffset)
{
    // TODO: Let's see what the optimizer does
    uint8 *Row = (uint8 *)Buffer->Memory;
    for(int Y = 0; Y < Buffer->Height; ++Y)
    {
        uint32 *Pixel = (uint32 *)Row;
        for(int X = 0; X < Buffer->Width; ++X)
        {
            // Pixel in memory: RR GG BB xx
            // LITTLE ENDIAN ARCHITECTURE
            // LOADS FIRST BYTE IN LOWEST PART OF MEMORY
            // All windows bitmaps have the pixel in memory as:
            // BB GG RR xx
        
            uint8 Blue = (X + BlueOffset);
            uint8 Green = (Y + GreenOffset);
            

            *Pixel++ = ((Green << 8) | Blue);
        }
        Row += Buffer->Pitch;
    }
}

internal void 
GameUpdateAndRender(game_input *Input, game_offscreen_buffer *Buffer, game_sound_output_buffer *SoundBuffer)
{
    local_persist int GreenOffset = 0;
    local_persist int BlueOffset = 0;
    local_persist int ToneHz = 900;

    game_controller_input *Input0 = &Input->Controllers[0];
    if(Input0->IsAnalog)
    {
        // NOTE: Use analog movement tuning
        BlueOffset += (int)(4.0f*(Input0->EndX));
        ToneHz = 900 + (int)(128.0f*(Input0->EndY));
        
    }
    else
    {
        // NOTE: Use digital movement tuning
    }

    if(Input0->Down.EndedDown)
    {
        GreenOffset -= 1;
    }
    if(Input0->Up.EndedDown)
    {
        GreenOffset += 1;
    }
    if(Input0->Back.EndedDown)
    {
        GlobalRunning = false;
    }

    

	// TODO: Allow sample offsets here for more robust platform options
	GameOutputSound(SoundBuffer, ToneHz);
    RenderWeirdGradient(Buffer, BlueOffset, GreenOffset);
}