#include <windows.h>
#include <stdint.h>


/***********************************
 * 
 *          Global Defines 
 * 
 ***********************************/


#define internal static
#define local_persist static
#define global_variable static

typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

struct win32_offscreen_buffer
{
    BITMAPINFO Info;
    void *Memory;
    int Width;
    int Height;
    int Pitch;
    int BytesPerPixel;
};
// TODO: This is global for now
global_variable bool Running;
global_variable win32_offscreen_buffer GlobalBackbuffer;

internal void RenderWeirdGradient(win32_offscreen_buffer Buffer, int XOffset, int YOffset)
{
    // TODO: Let's see what the optimizer does
    int Width = Buffer.Width;
    int Height = Buffer.Height;
    uint8 *Row = (uint8 *)Buffer.Memory;
    for(int Y = 0; Y < Buffer.Height; ++Y)
    {
        uint32 *Pixel = (uint32 *)Row;
        for(int X = 0; X < Buffer.Width; ++X)
        {
            // Pixel in memory: RR GG BB xx
            // LITTLE ENDIAN ARCHITECTURE
            // LOADS FIRST BYTE IN LOWEST PART OF MEMORY
            // All windows bitmaps have the pixel in memory as:
            // BB GG RR xx
        
            uint8 Blue = (X + XOffset);
            uint8 Green = (Y + YOffset);

            *Pixel++ = ((Green << 8) | Blue);
        }
        Row += Buffer.Pitch;
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
    Buffer->BytesPerPixel = 4;

    Buffer->Info.bmiHeader.biSize = sizeof(Buffer->Info.bmiHeader);
    Buffer->Info.bmiHeader.biWidth = Buffer->Width;
    Buffer->Info.bmiHeader.biHeight = -Buffer->Height;
    Buffer->Info.bmiHeader.biPlanes = 1;
    Buffer->Info.bmiHeader.biBitCount = 32;
    Buffer->Info.bmiHeader.biCompression = BI_RGB;

    // NOTE: Thank you to Chris Hecker of Spy Party fame
    // for clarifying the deal with StretchDIBits and BitBlt!
    int BitmapMemorySize = (Buffer->Width*Buffer->Height)*Buffer->BytesPerPixel;
    Buffer->Memory = VirtualAlloc(0, BitmapMemorySize, MEM_COMMIT, PAGE_READWRITE);

    // TODO: Probably clear this to black 

    Buffer->Pitch = Width*Buffer->BytesPerPixel;
}

internal void Win32DisplayBufferInWindow(HDC DeviceContext, RECT ClientRect, win32_offscreen_buffer Buffer, int X, int Y, int Width, int Height)
{
    int WindowWidth = ClientRect.right - ClientRect.left;
    int WindowHeight = ClientRect.bottom - ClientRect.top;
    StretchDIBits(DeviceContext,
                    /*
                    X, Y, Width, Height,
                    X, Y, Width, Height,
                    */
                    0, 0, Buffer.Width, Buffer.Height,
                    0, 0, WindowWidth, WindowHeight,
                    Buffer.Memory,
                    &Buffer.Info,
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
            RECT ClientRect;
            GetClientRect(Window, &ClientRect);
            int Width = ClientRect.right - ClientRect.left;
            int Height = ClientRect.bottom - ClientRect.top;
            Win32ResizeDIBSection(&GlobalBackbuffer, Width, Height);
        } break;
        case WM_DESTROY:
        {
            // TODO: Handle this with an error - recreate window?
            Running = false;
        } break;
        case WM_CLOSE:
        {
            // TODO: Handle this with a message to the user?
            Running = false;
        } break;
        case WM_ACTIVATEAPP:
        {
            OutputDebugString("WM_ACTIVEAPP\n");
        } break;
        case WM_PAINT:
        {
            PAINTSTRUCT Paint;
            HDC DeviceContext = BeginPaint(Window, &Paint);
            int X = Paint.rcPaint.left;
            int Y = Paint.rcPaint.top;
            int Height = Paint.rcPaint.bottom - Paint.rcPaint.top;
            int Width = Paint.rcPaint.right - Paint.rcPaint.left;
            RECT ClientRect;
            GetClientRect(Window, &ClientRect);

            Win32DisplayBufferInWindow(DeviceContext, ClientRect, GlobalBackbuffer, X, Y, Width, Height);
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


int CALLBACK WinMain(HINSTANCE Instance,
                     HINSTANCE PrevInstance,
                     LPSTR CommandLine,
                     int ShowCode)
{
    WNDCLASS WindowClass = {};
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
                int XOffset = 0;
                int YOffset = 0;
                Running = true;
                while(Running)
                {
                    MSG Message;
                    while(PeekMessage(&Message, 0, 0, 0, PM_REMOVE))
                    {
                        if(Message.message == WM_QUIT)
                        {
                            Running = false;
                        }
                        TranslateMessage(&Message);
                        DispatchMessage(&Message);
                    }
                    RenderWeirdGradient(GlobalBackbuffer, XOffset, YOffset);

                    HDC DeviceContext = GetDC(Window);
                    RECT ClientRect;
                    GetClientRect(Window, &ClientRect);
                    int WindowWidth = ClientRect.right - ClientRect.left;
                    int WindowHeight = ClientRect.bottom - ClientRect.top;
                    Win32DisplayBufferInWindow(DeviceContext, ClientRect, GlobalBackbuffer, 0, 0, WindowWidth, WindowHeight);
                    ReleaseDC(Window, DeviceContext);
                    ++XOffset;
                    YOffset += 2;
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

