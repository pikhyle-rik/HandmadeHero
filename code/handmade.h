#if !defined(HANDMADE_H)
#define HANDMADE_H

#define internal static
#define local_persist static
#define global_variable static
#define X_INPUT_GET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_STATE *pState)
#define X_INPUT_SET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_VIBRATION *pVibration)
#define DIRECT_SOUND_CREATE(name) HRESULT WINAPI name(LPCGUID pcGuidDevice, LPDIRECTSOUND *ppDS, LPUNKNOWN pUnkOuter)
#define Pi32 3.14159265359f
#define ArrayCount(Array) (sizeof(Array) / sizeof((Array)[0]))



typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;
typedef int32 bool32;

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

typedef float real32;
typedef double real64;

typedef X_INPUT_GET_STATE(x_input_get_state);
typedef X_INPUT_SET_STATE(x_input_set_state);
typedef DIRECT_SOUND_CREATE(direct_sound_create);

global_variable bool32 GlobalRunning;
/* TODO: Services that the game provides to the platform layer */

/* Note: Services that the game provides to the platform layer */

// NEEDS FOUR THINGS - TIMING, INPUT, BITMAP, SOUND
struct game_offscreen_buffer
{
    void *Memory;
    int Width;
    int Height;
    int Pitch;
};

struct game_sound_output_buffer
{
	int SamplesPerSecond;
	int SampleCount;
	int16 *Samples;
	
};

struct game_button_state
{
    int HalfTransitionCount;
    bool32 EndedDown;
};

struct game_controller_input
{
    bool32 IsAnalog;

    real32 StartX;
    real32 StartY;

    real32 MinX;
    real32 MinY;

    real32 MaxX;
    real32 MaxY;

    real32 EndX;
    real32 EndY;
    union 
    {
        game_button_state Buttons[8];
        struct
        {
            game_button_state Up;
            game_button_state Down;
            game_button_state Left;
            game_button_state Right;
            game_button_state LeftShoulder;
            game_button_state RightShoulder;
            game_button_state Start;
            game_button_state Back;
        };
    };
    

};

struct game_input
{
    game_controller_input Controllers[4];
};

internal void 
GameUpdateAndRender(game_input *Input, game_offscreen_buffer *Buffer, int BlueOffset, int GreenOffset,
					game_sound_output_buffer *SoundBuffer, int ToneHz);



#endif