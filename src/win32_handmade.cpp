#define VC_EXTRALEAN
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdint.h>

struct win32_offscreen_buffer
{
    BITMAPINFO Info;
    void* Memory;
    int Width;
    int Height;
    int Pitch;
    int BytesPerPixel;
};

static bool GlobalRunning;
static win32_offscreen_buffer GlobalBackbuffer;

struct win32_window_dimensions
{
    int Width;
    int Height;
};

win32_window_dimensions GetWindowDimension(HWND Window)
{
    win32_window_dimensions Dimensions = {};
    RECT ClientRect;
    GetClientRect(Window, &ClientRect);
    Dimensions.Width = ClientRect.right - ClientRect.left;
    Dimensions.Height = ClientRect.bottom - ClientRect.top;
    return Dimensions;
}

static void
RenderWierdGradient(win32_offscreen_buffer Buffer, int XOffset, int YOffset)
{
    uint8_t *Row = (uint8_t*)Buffer.Memory;

    for (int Y = 0; Y < Buffer.Height; Y += 1)
    {
        uint32_t *Pixel = (uint32_t*)Row;
        for (int X = 0; X < Buffer.Width; X += 1)
        { 
            uint8_t Blue = (uint8_t)(X + XOffset);
            uint8_t Green = (uint8_t)(Y + YOffset);
            
            *Pixel++ = ((Green << 8) | Blue); 
        }
        Row += Buffer.Pitch;
    }
}

static void
Win32ResizeDIBSection(win32_offscreen_buffer *Buffer, int Width, int Height)
{
    if (Buffer->Memory)
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

    int BitmapMemorySize = Buffer->BytesPerPixel * (Width * Height);
    Buffer->Memory = VirtualAlloc(0, BitmapMemorySize, MEM_COMMIT, PAGE_READWRITE);
    Buffer->Pitch = Buffer->Width * Buffer->BytesPerPixel;

    RenderWierdGradient(GlobalBackbuffer, 0, 0);
}

static void
Win32DisplayBufferInWindow(
    HDC DeviceContext,
    win32_offscreen_buffer Buffer,
    int X,
    int Y,
    int Width,
    int Height
)
{
    // @TODO: Aspect ratio correction
    StretchDIBits(
        DeviceContext,
        0, 0, Width, Height,
        0, 0, Buffer.Width, Buffer.Height,
        Buffer.Memory,
        &Buffer.Info,
        DIB_RGB_COLORS,
        SRCCOPY 
    );
}

LRESULT CALLBACK
MainWindowCallback(
  HWND Window,
  UINT Message,
  WPARAM WParam,
  LPARAM LParam 
)
{
    LRESULT Result = 0;

    switch(Message)
    {
        case WM_SIZE:
        {
        } break;

        case WM_CLOSE:
        {
            // @TODO: Handle this with a message to the user?
            GlobalRunning = false;
        } break;

        case WM_DESTROY:
        {
            // @TODO: Handle this as an error - recreate window?
            GlobalRunning = false;
        } break;

        case WM_ACTIVATEAPP:
        {
            OutputDebugStringA("WM_ACTIVATE_APP");
        } break;

        case WM_PAINT:
        {
            PAINTSTRUCT Paint;
            HDC DeviceContext = BeginPaint(Window, &Paint);
            int X = Paint.rcPaint.left;
            int Y = Paint.rcPaint.top;
            int Width = Paint.rcPaint.right - Paint.rcPaint.left;
            int Height = Paint.rcPaint.bottom - Paint.rcPaint.top;

            Win32DisplayBufferInWindow(DeviceContext, GlobalBackbuffer, X, Y, Width, Height); 
            EndPaint(Window, &Paint);
        } break;

        default:
        {
            Result = DefWindowProc(Window, Message, WParam, LParam);
        } break;
    }

    return Result;
}

int CALLBACK WinMain(
    HINSTANCE Instance,
    HINSTANCE PrevInstance,
    LPSTR CommandLine,
    int ShowCode 
)
{
    WNDCLASS WindowClass = {};
    WindowClass.style = CS_HREDRAW|CS_VREDRAW;
    WindowClass.lpfnWndProc = MainWindowCallback;
    WindowClass.hInstance = Instance;
    WindowClass.lpszClassName = "HandmadeHeroWindowClass";

    Win32ResizeDIBSection(&GlobalBackbuffer, 1280, 720);

    if (RegisterClass(&WindowClass))
    {
        HWND Window = CreateWindowExA(
            0,
            WindowClass.lpszClassName,
            "Handmade Hero",
            WS_OVERLAPPEDWINDOW | WS_VISIBLE,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            0,
            0,
            Instance,
            0 
        );

        if (Window)
        {
            GlobalRunning = true;

            int XOffset = 0;
            int YOffset = 0;

            MSG Message;

            while (GlobalRunning)
            {
                while(PeekMessage(&Message, 0, 0, 0, PM_REMOVE))
                {
                    if (Message.message == WM_QUIT)
                    {
                        GlobalRunning = false;
                    }

                    TranslateMessage(&Message);
                    DispatchMessage(&Message);
                }

                RenderWierdGradient(GlobalBackbuffer, XOffset, YOffset);

                HDC DeviceContext = GetDC(Window);
                win32_window_dimensions WindowDimensions = GetWindowDimension(Window);

                Win32DisplayBufferInWindow(
                    DeviceContext,
                    GlobalBackbuffer,
                    0,
                    0,
                    WindowDimensions.Width,
                    WindowDimensions.Height
                );
                ReleaseDC(Window, DeviceContext);
                XOffset += 1;
                YOffset += 1;
            }
        }
        else
        {
            // @TODO: Logging
        }
    }
    else
    {
        // @TODO: Logging
    }

    return 0;
}
