#include <windows.h>
#include <stdint.h>
#include <xinput.h>
#include <dsound.h>
#include <math.h>

/***********************************
 *                                 *
 *          Global Defines         *
 *                                 *
 ***********************************/


#define internal static
#define local_persist static
#define global_variable static
#define X_INPUT_GET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_STATE *pState)
#define X_INPUT_SET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_VIBRATION *pVibration)
#define DIRECT_SOUND_CREATE(name) HRESULT WINAPI name(LPCGUID pcGuidDevice, LPDIRECTSOUND *ppDS, LPUNKNOWN pUnkOuter)
#define Pi32 3.14159265359f

/***********************************
 *                                 *
 *        Type Definitions         *
 *                                 *
 ***********************************/

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
    int BytesPerPixel;
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
global_variable bool GlobalRunning;
global_variable win32_offscreen_buffer GlobalBackbuffer;
global_variable LPDIRECTSOUNDBUFFER GlobalSecondaryBuffer;
global_variable x_input_get_state *XInputGetState_ = XInputGetStateStub;
global_variable x_input_set_state *XInputSetState_ = XInputSetStateStub;
#define XInputGetState XInputGetState_
#define XInputSetState XInputSetState_

internal void Win32LoadXInput(void)
{
    
    HMODULE XInputLibrary = LoadLibrary("xinput1_4.dll");
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

internal void RenderWeirdGradient(win32_offscreen_buffer *Buffer, int BlueOffset, int GreenOffset)
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
            bool WasDown = ((LParam & (1 << 30)) != 0);
            bool IsDown = ((LParam & (1 << 31)) == 0);
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
                }
                else if(VKCode == VK_DOWN)
                {
                }
                else if(VKCode == VK_LEFT)
                {
                }
                else if(VKCode == VK_RIGHT)
                {
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
            Result = DefWindowProc(Window, Message, WParam, LParam);
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

};

void Win32FillSoundBuffer(win32_sound_output *SoundOutput, DWORD ByteToLock, DWORD BytesToWrite)
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
        int16 *SampleOut = (int16 *)Region1;
        for(DWORD SampleIndex = 0; SampleIndex < Region1SampleCount; ++SampleIndex)
        {
            real32 t = 2.0f*Pi32*(real32)SoundOutput->RunningSampleIndex / (real32)SoundOutput->WavePeriod;
            real32 SineValue = sinf(t);
            int16 SampleValue = (int16)(SineValue * SoundOutput->ToneVolume);
            *SampleOut++ = SampleValue;
            *SampleOut++ = SampleValue;                  

            ++SoundOutput->RunningSampleIndex;
        }
        DWORD Region2SampleCount = Region2Size/SoundOutput->BytesPerSample;
        SampleOut = (int16 *)Region2;
        for(DWORD SampleIndex = 0; SampleIndex < Region2SampleCount; ++SampleIndex)
        {
            real32 t = 2.0f*Pi32*(real32)SoundOutput->RunningSampleIndex / (real32)SoundOutput->WavePeriod;
            real32 SineValue = sinf(t);
            int16 SampleValue = (int16)(SineValue * SoundOutput->ToneVolume);
            *SampleOut++ = SampleValue;
            *SampleOut++ = SampleValue;                             

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
                int XOffset = 0;
                int YOffset = 0;
                // NOTE: Sound Test
                win32_sound_output SoundOutput = {};

                SoundOutput.SamplesPerSecond = 48000;              
                SoundOutput.ToneHz = 256;
                SoundOutput.ToneVolume = 10000;
                SoundOutput.RunningSampleIndex = 0;
                SoundOutput.WavePeriod = SoundOutput.SamplesPerSecond/SoundOutput.ToneHz;
                // int HalfWavePeriod = WavePeriod/2;
                SoundOutput.BytesPerSample = sizeof(int16)*2;
                SoundOutput.SecondaryBufferSize = SoundOutput.SamplesPerSecond*SoundOutput.BytesPerSample;
    
                Win32InitDSound(Window, SoundOutput.SamplesPerSecond, SoundOutput.SecondaryBufferSize);
                Win32FillSoundBuffer(&SoundOutput, 0, SoundOutput.SecondaryBufferSize);
                GlobalSecondaryBuffer->Play(0, 0, DSBPLAY_LOOPING);

                GlobalRunning = true;
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

                            bool Up = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_UP);
                            bool Down = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_DOWN);
                            bool Left = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_LEFT);
                            bool Right = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_RIGHT);
                            bool Start = (Pad->wButtons & XINPUT_GAMEPAD_START);
                            bool Back = (Pad->wButtons & XINPUT_GAMEPAD_BACK);
                            bool LeftShoulder = (Pad->wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER);
                            bool RightShoulder = (Pad->wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER);
                            bool AButton = (Pad->wButtons & XINPUT_GAMEPAD_A);
                            bool BButton = (Pad->wButtons & XINPUT_GAMEPAD_B);
                            bool XButton = (Pad->wButtons & XINPUT_GAMEPAD_X);
                            bool YButton = (Pad->wButtons & XINPUT_GAMEPAD_Y);

                            int16 StickX = Pad->sThumbLX;
                            int16 StickY = Pad->sThumbLY;

                            if(AButton)
                            {
                                YOffset += 2;
                            }
                            if(BButton)
                            {
                                YOffset -= 2;
                            }
                            
                            XOffset += StickX >> 12;
                            YOffset += StickY >> 12;

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
                    
                    RenderWeirdGradient(&GlobalBackbuffer, XOffset, YOffset);

                    // NOTE: DirectSound output test
                    DWORD PlayCursor;
                    DWORD WriteCursor;
                    if(SUCCEEDED(GlobalSecondaryBuffer->GetCurrentPosition(&PlayCursor, &WriteCursor)))
                    {
                        DWORD ByteToLock = (SoundOutput.RunningSampleIndex*SoundOutput.BytesPerSample) % SoundOutput.SecondaryBufferSize;
                        DWORD BytesToWrite;

                        // TODO: Change this to using a lower latency offset from the play cursor
                        // when we actually start adding sound effects
                        if(ByteToLock > PlayCursor)
                        {
                            BytesToWrite = (SoundOutput.SecondaryBufferSize - ByteToLock);
                            BytesToWrite += PlayCursor;
                        }
                        else
                        {
                            BytesToWrite = PlayCursor - ByteToLock;
                        }
                        Win32FillSoundBuffer(&SoundOutput, ByteToLock, BytesToWrite);                        
                    }
                    
                    
                    win32_window_dimension Dimension = Win32GetWindowDimension(Window);
                    Win32DisplayBufferInWindow(&GlobalBackbuffer, DeviceContext, Dimension.Width, Dimension.Height);
                    
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

