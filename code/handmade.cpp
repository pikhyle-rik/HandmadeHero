#include "handmade.h"

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
GameUpdateAndRender(game_offscreen_buffer *Buffer, int BlueOffset, int GreenOffset)
{
    RenderWeirdGradient(Buffer, BlueOffset, GreenOffset);
}