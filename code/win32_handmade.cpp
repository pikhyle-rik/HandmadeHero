/*
    THIS IS NOT A FINAL PLATFORM LAYER!!!
    TODO:
    - Saved game locations
    - Getting a handle to our own executable file
    - Asset loading path
    - Threading (launch a thread)
    - Raw input support for multiple keyboards
    - Sleep/timeBeginPeriod
    - ClipCuror() (multimonitor support)
    - Fullscreen support
    - WM_SETCURSOR (control cursor visability)
    - QueryCancelAutoplay
    - WM_ACTIVATEAPP (for when we are not the active application)
    - Blit speed improvements (bitBlt)
    - Hardware acceleration
    - GetKeyboardLayout (for French keyboards, international WASD support)
    * Just a partial list *
*/
#include <stdint.h>
#include <math.h>

#include <windows.h>
#include <stdio.h>
#include <xinput.h>
#include <dsound.h>
#include <malloc.h>
#include "handmade.cpp"
//

/***********************************
 *                                 *
 *          Global Defines         *
 *                                 *
 ***********************************/
// These defines are now made in handmade.h

/***********************************
 *                                 *
 *        Type Definitions         *
 *                                 *
 ***********************************/
// These definitions are now in handmade.h

/***********************************
 *                                 *
 *       Struct Definitions        *
 *                                 *
 ***********************************/

struct win32_offscreen_buffer
{
    BITMAPINFO Info;
    void *Memory;
    int Width;
    int Height;
    int Pitch;
};

struct win32_window_dimension
{
    int Width;
    int Height;
};

/*************************************
 *                                   *
 *              STUB FUNCTIONS       *
 *                                   *
 ************************************/

X_INPUT_GET_STATE(XInputGetStateStub)
{
    return(ERROR_DEVICE_NOT_CONNECTED);
}

X_INPUT_SET_STATE(XInputSetStateStub)
{
    return(ERROR_DEVICE_NOT_CONNECTED);
}

// TODO: This is global for now
global_variable bool32 GlobalRunning;
global_variable win32_offscreen_buffer GlobalBackbuffer;
global_variable LPDIRECTSOUNDBUFFER GlobalSecondaryBuffer;
global_variable x_input_get_state *XInputGetState_ = XInputGetStateStub;
global_variable x_input_set_state *XInputSetState_ = XInputSetStateStub;
global_variable int XOffset = 0;
global_variable int YOffset = 0;
#define XInputGetState XInputGetState_
#define XInputSetState XInputSetState_

internal void Win32LoadXInput(void)
{
    
    HMODULE XInputLibrary = LoadLibrary("xinput1_4.dll");
    if(!XInputLibrary)
    {
        XInputLibrary = LoadLibraryA("xinput9_1_0.dll");
    }
    if(!XInputLibrary)
    {
        XInputLibrary = LoadLibraryA("xinput1_3.dll");
    }
    if(XInputLibrary)
    {
        XInputGetState = (x_input_get_state *)GetProcAddress(XInputLibrary, "XInputGetState");
        if(!XInputGetState) {XInputGetState = XInputGetStateStub;}
        XInputSetState = (x_input_set_state *)GetProcAddress(XInputLibrary, "XInputSetState");
        if(!XInputSetState) {XInputSetState = XInputSetStateStub;}

    }
}

internal void Win32InitDSound(HWND Window, int32 SamplesPerSecond, int32 BufferSize)
{
    // Note: Load the library
    HMODULE DSoundLibrary = LoadLibraryA("dsound.dll");
    if(DSoundLibrary)
    {
        // Note: Get a DirectSound object
        direct_sound_create *DirectSoundCreate = (direct_sound_create *)GetProcAddress(DSoundLibrary, "DirectSoundCreate");
        LPDIRECTSOUND DirectSound;
        if(DirectSoundCreate && SUCCEEDED(DirectSoundCreate(0, &DirectSound, 0)))
        {
            WAVEFORMATEX WaveFormat = {};
            WaveFormat.wFormatTag = WAVE_FORMAT_PCM;
            WaveFormat.nChannels = 2;
            WaveFormat.nSamplesPerSec = SamplesPerSecond;
            WaveFormat.wBitsPerSample = 16;
            WaveFormat.nBlockAlign = (WaveFormat.nChannels*WaveFormat.wBitsPerSample) / 8;
            WaveFormat.nAvgBytesPerSec = WaveFormat.nSamplesPerSec*WaveFormat.nBlockAlign;
            WaveFormat.cbSize = 0;
            if(SUCCEEDED(DirectSound->SetCooperativeLevel(Window, DSSCL_PRIORITY)))
            {
                DSBUFFERDESC BufferDescription = {};
                BufferDescription.dwSize = sizeof(BufferDescription);
                BufferDescription.dwFlags = DSBCAPS_PRIMARYBUFFER;
                // Create a primary buffer
                LPDIRECTSOUNDBUFFER PrimaryBuffer;
                if(SUCCEEDED(DirectSound->CreateSoundBuffer(&BufferDescription, &PrimaryBuffer, 0)))
                {
                    if(SUCCEEDED(PrimaryBuffer->SetFormat(&WaveFormat)))
                    {
                        OutputDebugStringA("Primary Buffer Format Set\n");
                    }
                    else
                    {
                        // TODO: Diagnostic
                    }
                }    
                else
                {
                    // TODO: Diagnostic
                }
            }
            else
            {
                // TODO: Diagnostic
            }
            DSBUFFERDESC BufferDescription = {};
            BufferDescription.dwSize = sizeof(BufferDescription);
            BufferDescription.dwFlags = 0;
            BufferDescription.dwBufferBytes = BufferSize;
            BufferDescription.lpwfxFormat = &WaveFormat;
            if(SUCCEEDED(DirectSound->CreateSoundBuffer(&BufferDescription, &GlobalSecondaryBuffer, 0)))
            {

                OutputDebugStringA("Secondary buffer created successfully!\n");
            }

            // Note: "Create" a secondary buffer
            // Note: Start it playing
        }
        else
        {
            // TODO: Diagnostic
        }
    }
    else
    {
        // TODO: Diagnostic
    }
}

internal win32_window_dimension Win32GetWindowDimension(HWND Window)
{
    win32_window_dimension Result;

    RECT ClientRect;
    GetClientRect(Window, &ClientRect);
    Result.Width = ClientRect.right - ClientRect.left;
    Result.Height = ClientRect.bottom - ClientRect.top;

    return(Result);
}



internal void Win32ResizeDIBSection(win32_offscreen_buffer *Buffer, int Width, int Height)
{
    // TODO: Bulletproof this
    // Maybe don't free first, free after, then free first if that fails


    if(Buffer->Memory)
    {
        VirtualFree(Buffer->Memory, 0, MEM_RELEASE);
    }

    Buffer->Width = Width;
    Buffer->Height = Height;
    int BytesPerPixel = 4;

    Buffer->Info.bmiHeader.biSize = sizeof(Buffer->Info.bmiHeader);
    Buffer->Info.bmiHeader.biWidth = Buffer->Width;
    Buffer->Info.bmiHeader.biHeight = -Buffer->Height;
    Buffer->Info.bmiHeader.biPlanes = 1;
    Buffer->Info.bmiHeader.biBitCount = 32;
    Buffer->Info.bmiHeader.biCompression = BI_RGB;

    // NOTE: Thank you to Chris Hecker of Spy Party fame
    // for clarifying the deal with StretchDIBits and BitBlt!
    int BitmapMemorySize = (Buffer->Width*Buffer->Height)*BytesPerPixel;
    Buffer->Memory = VirtualAlloc(0, BitmapMemorySize, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);

    // TODO: Probably clear this to black 

    Buffer->Pitch = Width*BytesPerPixel;
}

internal void Win32DisplayBufferInWindow(win32_offscreen_buffer *Buffer, HDC DeviceContext, int WindowWidth, int WindowHeight)
{
    // TODO: Aspect ratio correction
    StretchDIBits(DeviceContext,
                    /*
                    X, Y, Width, Height,
                    X, Y, Width, Height,
                    */
                    0, 0, WindowWidth, WindowHeight,
                    0, 0, Buffer->Width, Buffer->Height,
                    Buffer->Memory,
                    &Buffer->Info,
                    DIB_RGB_COLORS,
                    SRCCOPY);

}

LRESULT CALLBACK Win32MainWindowCallback(HWND Window, UINT Message, WPARAM WParam, LPARAM LParam)
{
    LRESULT Result = 0;
    switch(Message)
    {
        case WM_SIZE:
        {
        } break;
        case WM_DESTROY:
        {
            // TODO: Handle this with an error - recreate window?
            GlobalRunning = false;
        } break;
        case WM_CLOSE:
        {
            // TODO: Handle this with a message to the user?
            GlobalRunning = false;
        } break;
        case WM_ACTIVATEAPP:
        {
            OutputDebugString("WM_ACTIVEAPP\n");
        } break;

        case WM_SYSKEYDOWN:
        case WM_SYSKEYUP:
        case WM_KEYDOWN:
        case WM_KEYUP:
        {
            uint32 VKCode = WParam;
            bool32 WasDown = ((LParam & (1 << 30)) != 0);
            bool32 IsDown = ((LParam & (1 << 31)) == 0);
            if(WasDown != IsDown)
            {
                if(VKCode == 'W')
                {
                }
                else if(VKCode == 'A')
                {
                }
                else if(VKCode == 'S')
                {
                }
                else if(VKCode == 'D')
                {
                }
                else if(VKCode == 'Q')
                {
                }
                else if(VKCode == 'E')
                {
                }
                else if(VKCode == VK_ESCAPE)
                {
                    OutputDebugStringA("ESCAPE:");
                    if(IsDown)
                    {
                        OutputDebugStringA("Is Down ");
                    }
                    if(WasDown)
                    {
                        OutputDebugStringA("Was Down ");
                    }
                    OutputDebugStringA("\n");
                }
                else if(VKCode == VK_SPACE)
                {
                }
                else if(VKCode == VK_UP)
                {
                    YOffset -= 10;
                }
                else if(VKCode == VK_DOWN)
                {
                    YOffset += 10;
                }
                else if(VKCode == VK_LEFT)
                {
                    XOffset += 10;
                }
                else if(VKCode == VK_RIGHT)
                {
                    XOffset -= 10;
                }
            }

            bool32 AltKeyWasDown = ((LParam & (1 << 29)) != 0);
            if((VKCode == VK_F4) && AltKeyWasDown)
            {
                GlobalRunning = false;
            }
        } break;

        case WM_SETCURSOR:
        {
            //SetCursor(0);
        } break;

        case WM_PAINT:
        {
            PAINTSTRUCT Paint;
            HDC DeviceContext = BeginPaint(Window, &Paint);

            win32_window_dimension Dimension = Win32GetWindowDimension(Window);

            Win32DisplayBufferInWindow(&GlobalBackbuffer, DeviceContext, Dimension.Width, Dimension.Height);
            EndPaint(Window, &Paint);
        } break;
        default:
        {
            // OutputDebugString("default\n");
            Result = DefWindowProcA(Window, Message, WParam, LParam);
        } break;
    }
    return(Result);
}

/***********************
 *    Main Function    *
 ***********************/
struct win32_sound_output
{
    int SamplesPerSecond; 
    int ToneHz;
    int16 ToneVolume;
    uint32 RunningSampleIndex;
    int WavePeriod;
    // int HalfWavePeriod = WavePeriod/2;
    int BytesPerSample;
    int SecondaryBufferSize;
    real32 tSine;
    int LatencySampleCount;
};

internal void
Win32ClearBuffer(win32_sound_output *SoundOutput)
{
	VOID *Region1;
    DWORD Region1Size;
    VOID *Region2;
    DWORD Region2Size;
	if(SUCCEEDED(GlobalSecondaryBuffer->Lock(0, SoundOutput->SecondaryBufferSize,
											 &Region1, &Region1Size,
											 &Region2, &Region2Size, 0)))
	{
		uint8 *DestSample = (uint8 *)Region1;
		for(DWORD ByteIndex = 0;
			ByteIndex < Region1Size;
			++ByteIndex)
		{
			*DestSample++ = 0;
		}
		DestSample = (uint8 *)Region2;
		for(DWORD ByteIndex = 0;
			ByteIndex < Region2Size;
			++ByteIndex)
		{
			*DestSample++ = 0;
		}
		GlobalSecondaryBuffer->Unlock(Region1, Region1Size, Region2, Region2Size);
	}
}

void Win32FillSoundBuffer(win32_sound_output *SoundOutput, DWORD ByteToLock, DWORD BytesToWrite,
						  game_sound_output_buffer *Source)
{
    VOID *Region1;
    DWORD Region1Size;
    VOID *Region2;
    DWORD Region2Size;
    
    if(SUCCEEDED(GlobalSecondaryBuffer->Lock(
        ByteToLock,
        BytesToWrite,
        &Region1, &Region1Size,
        &Region2, &Region2Size,
        0)))
    {
        // TODO: Assert that Region1Size/Region2Size is valid
        // TODO: Collapse these two loops
        DWORD Region1SampleCount = Region1Size/SoundOutput->BytesPerSample;
        int16 *DestSample = (int16 *)Region1;
		int16 *SourceSample = Source->Samples;
        for(DWORD SampleIndex = 0; SampleIndex < Region1SampleCount; ++SampleIndex)
        {
            *DestSample++ = *SourceSample++;
            *DestSample++ = *SourceSample++;                  
            ++SoundOutput->RunningSampleIndex;
        }
        DWORD Region2SampleCount = Region2Size/SoundOutput->BytesPerSample;
        DestSample = (int16 *)Region2;
        for(DWORD SampleIndex = 0; SampleIndex < Region2SampleCount; ++SampleIndex)
        {
            *DestSample++ = *SourceSample++;
            *DestSample++ = *SourceSample++;                  
            ++SoundOutput->RunningSampleIndex;
        }
        GlobalSecondaryBuffer->Unlock(Region1, Region1Size, Region2, Region2Size);
    }
}

int CALLBACK WinMain(HINSTANCE Instance,
                     HINSTANCE PrevInstance,
                     LPSTR CommandLine,
                     int ShowCode)
{
    LARGE_INTEGER PerfCountFrequencyResult;
    QueryPerformanceFrequency(&PerfCountFrequencyResult);
    int64 PerfCountFrequency = PerfCountFrequencyResult.QuadPart;
    Win32LoadXInput();

    WNDCLASS WindowClass = {};

    Win32ResizeDIBSection(&GlobalBackbuffer, 640, 480);
    WindowClass.style = CS_HREDRAW|CS_VREDRAW;
    WindowClass.lpfnWndProc = Win32MainWindowCallback;
    WindowClass.hInstance = Instance;
    WindowClass.lpszClassName = "HandmadeHeroWindowClass";

    if(RegisterClass(&WindowClass))
    {
        HWND Window = CreateWindowEx(0,
                      WindowClass.lpszClassName,
                      "Handmade Hero",
                      WS_OVERLAPPEDWINDOW|WS_VISIBLE,
                      CW_USEDEFAULT,
                      CW_USEDEFAULT,
                      CW_USEDEFAULT,
                      CW_USEDEFAULT,
                      0,
                      0,
                      Instance,
                      0);
            if(Window)
            {
                HDC DeviceContext = GetDC(Window);
                // NOTE: Graphics Test
                
                // NOTE: Sound Test
                win32_sound_output SoundOutput = {};

                SoundOutput.SamplesPerSecond = 48000;
				SoundOutput.ToneHz = 900;
                SoundOutput.RunningSampleIndex = 0;
                SoundOutput.BytesPerSample = sizeof(int16)*2;
                SoundOutput.SecondaryBufferSize = SoundOutput.SamplesPerSecond*SoundOutput.BytesPerSample;
                SoundOutput.LatencySampleCount = SoundOutput.SamplesPerSecond / 15;
                Win32InitDSound(Window, SoundOutput.SamplesPerSecond, SoundOutput.SecondaryBufferSize);
                Win32ClearBuffer(&SoundOutput);
                GlobalSecondaryBuffer->Play(0, 0, DSBPLAY_LOOPING);
				
                GlobalRunning = true;
				int16 *Samples = (int16 *)VirtualAlloc(0, SoundOutput.SecondaryBufferSize, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
				
                LARGE_INTEGER LastCounter;
                QueryPerformanceCounter(&LastCounter);
                uint64 LastCycleCount = __rdtsc();
                while(GlobalRunning)
                {
                    
                    MSG Message;
                    while(PeekMessage(&Message, 0, 0, 0, PM_REMOVE))
                    {
                        if(Message.message == WM_QUIT)
                        {
                            GlobalRunning = false;
                        }
                        TranslateMessage(&Message);
                        DispatchMessage(&Message);
                    }

                    // TODO: Should we poll this more frequently
                    for(DWORD ControllerIndex = 0; ControllerIndex < XUSER_MAX_COUNT; ++ControllerIndex)
                    {
                        XINPUT_STATE ControllerState;
                        if(XInputGetState(ControllerIndex, &ControllerState) == ERROR_SUCCESS)
                        {
                            //NOTE: Controller plugged in
                            // TODO: See if ControllerState.dwPacketNumber increments to rapidly
                            XINPUT_GAMEPAD *Pad = &ControllerState.Gamepad;

                            bool32 Up = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_UP);
                            bool32 Down = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_DOWN);
                            bool32 Left = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_LEFT);
                            bool32 Right = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_RIGHT);
                            bool32 Start = (Pad->wButtons & XINPUT_GAMEPAD_START);
                            bool32 Back = (Pad->wButtons & XINPUT_GAMEPAD_BACK);
                            bool32 LShoulder = (Pad->wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER);
                            bool32 RShoulder = (Pad->wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER);
                            bool32 AButton = (Pad->wButtons & XINPUT_GAMEPAD_A);
                            bool32 BButton = (Pad->wButtons & XINPUT_GAMEPAD_B);
                            bool32 XButton = (Pad->wButtons & XINPUT_GAMEPAD_X);
                            bool32 YButton = (Pad->wButtons & XINPUT_GAMEPAD_Y);

                            int16 StickX = Pad->sThumbLX;
                            int16 StickY = Pad->sThumbLY;
                            // Just to mess around with manipulating the ToneHz while running
                            
                            if(AButton)
                            {
                                SoundOutput.ToneHz = 1507;
                                SoundOutput.WavePeriod = SoundOutput.SamplesPerSecond/SoundOutput.ToneHz;

                            }
                            if(BButton)
                            {
                                SoundOutput.ToneHz = 754;
                                SoundOutput.WavePeriod = SoundOutput.SamplesPerSecond/SoundOutput.ToneHz;
                            }
                            
                            if(Up)
                            {
                                SoundOutput.ToneHz = 598;
                                SoundOutput.WavePeriod = SoundOutput.SamplesPerSecond/SoundOutput.ToneHz;
                            }
                            if(Down)
                            {
                                SoundOutput.ToneHz = 797;
                                SoundOutput.WavePeriod = SoundOutput.SamplesPerSecond/SoundOutput.ToneHz;   
                            }
                            /*
                            SoundOutput.ToneHz = 1800 + (int)(900.0f*((real32)StickY / 30000.0f));
                            SoundOutput.WavePeriod = SoundOutput.SamplesPerSecond/SoundOutput.ToneHz;
                            XOffset += StickX / 1024;
                            YOffset += StickY / 1024;
                            */
                            if(Back)
                            {
                                GlobalRunning = false;
                            }
                        }
                        else
                        {
                            // NOTE: Controller not available
                        }
                    }
					DWORD ByteToLock;
					DWORD TargetCursor;
					DWORD BytesToWrite;
					DWORD PlayCursor;
                    DWORD WriteCursor;
					bool32 SoundIsValid = false;
					// TODO: Tighten Up sound logic so that we know where we
					// should be writing to and can anticipate the time spent
					// in the game update.
					if(SUCCEEDED(GlobalSecondaryBuffer->GetCurrentPosition(
								&PlayCursor, &WriteCursor)))
					{
						ByteToLock = (SoundOutput.RunningSampleIndex*SoundOutput.BytesPerSample) % 
										SoundOutput.SecondaryBufferSize;
                        TargetCursor = (
						(PlayCursor + (SoundOutput.LatencySampleCount*SoundOutput.BytesPerSample)) %
                                                SoundOutput.SecondaryBufferSize);
                        if(ByteToLock > TargetCursor)
                        {
                            BytesToWrite = (SoundOutput.SecondaryBufferSize - ByteToLock);
                            BytesToWrite += TargetCursor;
                        }
                        else
                        {
                            BytesToWrite = TargetCursor - ByteToLock;
                        }
						SoundIsValid = true;
					}
					
					
					game_sound_output_buffer SoundBuffer = {};
					SoundBuffer.SamplesPerSecond = SoundOutput.SamplesPerSecond;
					SoundBuffer.SampleCount = BytesToWrite / SoundOutput.BytesPerSample;
					SoundBuffer.Samples = Samples;
					
                    game_offscreen_buffer Buffer = {};
                    Buffer.Memory = GlobalBackbuffer.Memory;
                    Buffer.Width = GlobalBackbuffer.Width;
                    Buffer.Height = GlobalBackbuffer.Height;
                    Buffer.Pitch = GlobalBackbuffer.Pitch;
					
                    GameUpdateAndRender(&Buffer, XOffset, YOffset, &SoundBuffer, SoundOutput.ToneHz);
                    // NOTE: DirectSound output test
                    
                    if(SoundIsValid)
                    {
                        Win32FillSoundBuffer(&SoundOutput, ByteToLock, BytesToWrite, &SoundBuffer);                        
                    }
                    
                    
                    win32_window_dimension Dimension = Win32GetWindowDimension(Window);
                    Win32DisplayBufferInWindow(&GlobalBackbuffer, DeviceContext, Dimension.Width, Dimension.Height);

                    uint64 EndCycleCount = __rdtsc();
                    LARGE_INTEGER EndCounter;
                    QueryPerformanceCounter(&EndCounter);

                    uint64 CyclesElapsed = EndCycleCount - LastCycleCount;
                    int64 CounterElapsed = EndCounter.QuadPart - LastCounter.QuadPart;
                    real64 MSPerFrame = (((1000*(real64)CounterElapsed) / (real64)PerfCountFrequency));
                    real64 FPS = (real64)PerfCountFrequency / (real64)CounterElapsed;
                    real64 MCPF = ((real64)CyclesElapsed / (1000 * 1000));
                    real64 ClockSpeed = (real64)((FPS*MCPF)/1000.0f);
#if 0
                    char Buffer[512];
                    sprintf(Buffer, "%.02fms/f, %.02ff/s, %.02fmc/f, %.02fGHz\n", MSPerFrame, FPS, MCPF, ClockSpeed);
                    OutputDebugStringA(Buffer);
#endif
                    LastCounter = EndCounter;
                    LastCycleCount = EndCycleCount;
                }
            }
            else
            {
                // TODO
            }
    }
    else
    {
        // TODO
    }

    return(0);
}

