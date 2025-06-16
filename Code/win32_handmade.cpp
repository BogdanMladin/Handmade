#include "handmade.cpp"

#define DIRECT_SOUND_CREATE(name)                                              \
  HRESULT WINAPI name(LPCGUID pcGuidDevice, LPDIRECTSOUND *ppDS,               \
                      LPUNKNOWN pUnkOuter);
typedef DIRECT_SOUND_CREATE(direct_sound_create);

static bool Running;
static int64_t GlobalPerfCountFrequency;
static LPDIRECTSOUNDBUFFER GlobalSecondaryBuffer;

static debug_read_file_result DEBUGPlatformReadEntireFile(char *Filename) {
  debug_read_file_result Result = {};

  HANDLE FileHandle = CreateFileA(Filename, GENERIC_READ, FILE_SHARE_READ, 0,
                                  OPEN_EXISTING, 0, 0);
  if (FileHandle != INVALID_HANDLE_VALUE) {
    LARGE_INTEGER FileSize;
    if (GetFileSizeEx(FileHandle, &FileSize)) {
      uint32_t FileSize32 = SafeTruncateUint64(FileSize.QuadPart);
      Result.Contents =
          VirtualAlloc(0, FileSize32, MEM_RESERVE | MEM_COMMIT,
                       PAGE_READWRITE); // Virtual alloc is debug code (only
                                        // used for large allocations)
      if (Result.Contents) {            // use HeapAlloc for ship code
        DWORD BytesRead;
        if (ReadFile(FileHandle, Result.Contents, FileSize32, &BytesRead, 0) &&
            FileSize32 == BytesRead) {
          Result.ContentSize = FileSize32;
        } else {
          DEBUGPlatformFreeFileMemory(Result.Contents);
          Result.Contents = 0;
        }

      } else {
      } // problem
      CloseHandle(FileHandle);
    } else {
    } // problem

  } else {
  } // problem
  return (Result);
}
static void DEBUGPlatformFreeFileMemory(void *Memory) {
  if (Memory) {
    VirtualFree(Memory, 0, MEM_RELEASE);
  }
}
static float DEBUGPlatformWriteEntireFile(char *Filename, uint32_t MemorySize,
                                          void *Memory) {

  char Result = false;

  HANDLE FileHandle =
      CreateFileA(Filename, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, 0, 0);
  if (FileHandle != INVALID_HANDLE_VALUE) {

    DWORD BytesWritten;
    if (WriteFile(FileHandle, Memory, MemorySize, &BytesWritten, 0)) {
      Result = (BytesWritten == MemorySize);
    } else {
      // problem
    }

    CloseHandle(FileHandle);
  } else {
  } // problem
  return (Result);
}

struct win32_Window_Dimension {
  int Width;
  int Height;
};

struct win32_sound_output {
  int SamplesPerSecond;
  uint32_t RunningSampleIndex;
  int BytesPerSample;
  int ToneVolume;
  DWORD SecondaryBufferSize;
  int LatencySampleCount;
  float tsine;
};

struct win32_debug_time_marker {
  DWORD PlayCursor;
  DWORD WriteCursor;
};

static void Win32ClearBuffer(win32_sound_output *SoundOutput) {
  VOID *Region1;
  DWORD Region1Size;
  VOID *Region2;
  DWORD Region2Size;

  if (SUCCEEDED(GlobalSecondaryBuffer->Lock(0, SoundOutput->SecondaryBufferSize,
                                            &Region1, &Region1Size, &Region2,
                                            &Region2Size, 0))) {
    uint8_t *DestSample = (uint8_t *)Region1;
    for (DWORD ByteIndex = 0; ByteIndex < Region1Size; ++ByteIndex) {

      *DestSample++ = 0;
    }

    DestSample = (uint8_t *)Region2;
    for (DWORD ByteIndex = 0; ByteIndex < Region2Size; ++ByteIndex) {

      *DestSample++ = 0;
    }
    GlobalSecondaryBuffer->Unlock(Region1, Region1Size, Region1, Region2Size);
  }
}

static void Win32ProcessKeyboardMessage(game_button_state *NewState,
                                        char IsDown) {
  Assert(NewState->EndedDown != IsDown);
  NewState->EndedDown = IsDown;
  ++NewState->HalfTransitionCount;
}

static void
Win32ProcessPendingMessages(game_controller_input *KeyboardController) {
  MSG Message;

  while (PeekMessageA(&Message, NULL, 0, 0, PM_REMOVE)) {
    if (Message.message == WM_QUIT) {
      Running = false;
    }

    switch (Message.message) {
    case WM_SYSKEYDOWN:
    case WM_SYSKEYUP:
    case WM_KEYDOWN:
    case WM_KEYUP: {
      uint32_t VKCode = (uint32_t)Message.wParam;
      // float WasDown = ((Message.lParam % (1 << 30)) !=
      // 0);??????????????????????????????????? float IsDown = ((Message.lParam
      // % (1 << 31)) == 0);idk y put theese hhere ???????????????????????

      char WasDown = ((Message.lParam & (1 << 30)) != 0);
      char IsDown = ((Message.lParam & (1 << 31)) == 0);

      if (IsDown != WasDown) {
        if (VKCode == 'W') {
          Win32ProcessKeyboardMessage(&KeyboardController->MoveUp, IsDown);
        } else if (VKCode == 'A') {
          Win32ProcessKeyboardMessage(&KeyboardController->MoveLeft, IsDown);
        } else if (VKCode == 'S') {
          Win32ProcessKeyboardMessage(&KeyboardController->MoveDown, IsDown);
        } else if (VKCode == 'D') {
          Win32ProcessKeyboardMessage(&KeyboardController->MoveRight, IsDown);
        } else if (VKCode == 'Q') {
          Win32ProcessKeyboardMessage(&KeyboardController->LeftShoulder,
                                      IsDown);
        } else if (VKCode == 'E') {
          Win32ProcessKeyboardMessage(&KeyboardController->RightShoulder,
                                      IsDown);
        } else if (VKCode == VK_UP) {
          Win32ProcessKeyboardMessage(&KeyboardController->ActionUp, IsDown);
        } else if (VKCode == VK_LEFT) {
          Win32ProcessKeyboardMessage(&KeyboardController->ActionLeft, IsDown);
        } else if (VKCode == VK_DOWN) {
          Win32ProcessKeyboardMessage(&KeyboardController->ActionDown, IsDown);
        } else if (VKCode == VK_RIGHT) {
          Win32ProcessKeyboardMessage(&KeyboardController->ActionRight, IsDown);
        } else if (VKCode == VK_ESCAPE) {
          OutputDebugString("ESCAPE: ");
          if (IsDown) {
            OutputDebugString("IsDown");
          }
          if (WasDown) {
            OutputDebugString("WAS DOWN");
          }
          OutputDebugString("\n");
        } else if (VKCode == VK_SPACE) {
        }
      }
      break;
    }

    default: {
      TranslateMessage(&Message);
      DispatchMessage(&Message);

    } break;
    }
  }
}

static void Win32FillSoundBuffer(win32_sound_output *SoundOutput,
                                 DWORD BytesToLock, DWORD BytesToWrite,
                                 game_sound_output_buffer *SourceBuffer) {
  VOID *Region1;
  DWORD Region1Size;
  VOID *Region2;
  DWORD Region2Size;

  if (SUCCEEDED(GlobalSecondaryBuffer->Lock(BytesToLock, BytesToWrite, &Region1,
                                            &Region1Size, &Region2,
                                            &Region2Size, 0))) {

    DWORD Region1SampleCount = Region1Size / SoundOutput->BytesPerSample;
    int16_t *DestSample = (int16_t *)Region1;
    int16_t *SourceSample = SourceBuffer->Samples;
    for (DWORD Sampleindex = 0; Sampleindex < Region1SampleCount;
         ++Sampleindex) {

      *DestSample++ = *SourceSample++;
      *DestSample++ = *SourceSample++;

      ++SoundOutput->RunningSampleIndex;
    }

    DWORD Region2SampleCount = Region2Size / SoundOutput->BytesPerSample;
    DestSample = (int16_t *)Region2;
    for (DWORD Sampleindex = 0; Sampleindex < Region2SampleCount;
         ++Sampleindex) {
      *DestSample++ = *SourceSample++;
      *DestSample++ = *SourceSample++;

      ++SoundOutput->RunningSampleIndex;
    }

    GlobalSecondaryBuffer->Unlock(Region1, Region1Size, Region1, Region2Size);
  } else {
    // Diagnostic
  }
}

static void win32InitDSound(HWND WindowHandle, uint32_t SamplesPerSecond,
                            uint32_t BufferSize) {
  // LOAD Library
  HMODULE DSoundLibrary = LoadLibrary("dsound.dll");
  if (DSoundLibrary) {
    // Get a DirectSound Object
    direct_sound_create *DirectSoundCreate =
        (direct_sound_create *)GetProcAddress(DSoundLibrary,
                                              "DirectSoundCreate");

    LPDIRECTSOUND DirectSound;
    if (DirectSoundCreate && SUCCEEDED(DirectSoundCreate(0, &DirectSound, 0))) {
      WAVEFORMATEX WaveFormat = {};
      WaveFormat.wFormatTag = WAVE_FORMAT_PCM;
      WaveFormat.nChannels = 2;
      WaveFormat.nSamplesPerSec = SamplesPerSecond;
      WaveFormat.wBitsPerSample = 16;
      WaveFormat.nBlockAlign =
          (WaveFormat.nChannels * WaveFormat.wBitsPerSample) / 8;
      WaveFormat.nAvgBytesPerSec =
          WaveFormat.nSamplesPerSec * WaveFormat.nBlockAlign;
      WaveFormat.cbSize;

      if (SUCCEEDED(
              DirectSound->SetCooperativeLevel(WindowHandle, DSSCL_PRIORITY))) {

        DSBUFFERDESC BufferDescription = {};
        BufferDescription.dwSize = sizeof(BufferDescription);
        BufferDescription.dwFlags = DSBCAPS_PRIMARYBUFFER;

        // Create a Primary Buffer

        LPDIRECTSOUNDBUFFER PrimaryBuffer;

        if (SUCCEEDED(DirectSound->CreateSoundBuffer(&BufferDescription,
                                                     &PrimaryBuffer, 0))) {

          if (SUCCEEDED(PrimaryBuffer->SetFormat(&WaveFormat))) {
            // We have set the format!!!
            OutputDebugString("Primary buffer set");
          } else {
            // Diagnostic
          }
        } else {
          // Diagnostic
        }
        BufferDescription = {};
        BufferDescription.dwSize = sizeof(BufferDescription);
        BufferDescription.dwFlags = 0;
        BufferDescription.dwBufferBytes = BufferSize;
        BufferDescription.lpwfxFormat = &WaveFormat;
        // Create a Secondary Buffer

        if (SUCCEEDED(DirectSound->CreateSoundBuffer(
                &BufferDescription, &GlobalSecondaryBuffer, 0))) {

          OutputDebugStringA("Secondary buffer created succesfully!");

        } else {
          // Diagnostic
        }
        //  ( *)GetProcAddress(DSoundLibrary, "Create")

      } else {
        // Diagnostic
      }

    } else {
      // Diagnostic
    }

  } else {
    // Diagnostic
  }
}

win32_Window_Dimension win32GetWindowDimension(HWND WindowHandle) {
  win32_Window_Dimension Result;

  RECT ClientRect;
  GetClientRect(WindowHandle, &ClientRect);
  Result.Width = ClientRect.right - ClientRect.left;
  Result.Height = ClientRect.bottom - ClientRect.top;

  return (Result);
}

struct win32_offscreeen_buffer {
  BITMAPINFO Info;
  void *Memory;
  int Width;
  int Height;
  int Pitch;
  int BytesPerPixel;
};
static win32_offscreeen_buffer GlobalBackbuffer;

static void RenderWeirdGradient(win32_offscreeen_buffer Buffer, int XOffset,
                                int YOffset) {

  uint8_t *Row =
      (uint8_t *)Buffer.Memory; // unsigned char == uint8 == 8 bytes unsigned
  for (int y = 0; y < Buffer.Height; ++y) {

    uint32_t *Pixel = (uint32_t *)Row;
    for (int x = 0; x < Buffer.Width; ++x) {

      uint8_t Blue = (uint8_t)(x + XOffset);
      uint8_t Green = (uint8_t)(y + YOffset);

      *Pixel++ = ((Green << 8) | Blue);
    }
    Row += Buffer.Pitch;
  }
}

static void win32resizeDIBsection(win32_offscreeen_buffer *Buffer, int Width,
                                  int Height) {

  if (Buffer->Memory) {
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
  Buffer->Info.bmiHeader.biSizeImage = 0;
  Buffer->Info.bmiHeader.biXPelsPerMeter = 0;
  Buffer->Info.bmiHeader.biYPelsPerMeter = 0;
  Buffer->Info.bmiHeader.biClrUsed = 0;
  Buffer->Info.bmiHeader.biClrImportant = 0;

  int BitmapMemorySize =
      (Buffer->Width * Buffer->Height) * Buffer->BytesPerPixel;
  // int BitmapMemorySize = 1000;
  Buffer->Memory = VirtualAlloc(0, BitmapMemorySize, MEM_RESERVE | MEM_COMMIT,
                                PAGE_READWRITE);

  Buffer->Pitch = Buffer->Width * Buffer->BytesPerPixel;

  RenderWeirdGradient(GlobalBackbuffer, 0, 0);
}

static void win32DisplayBufferInWindow(win32_offscreeen_buffer *Buffer,
                                       HDC DeviceContext, int WindowWidth,
                                       int WindowHeight) {
  StretchDIBits(DeviceContext,
                /*
                x, y, Width, Height,
                x, y, Width, Height,
                */
                0, 0, WindowWidth, WindowHeight, 0, 0, Buffer->Width,
                Buffer->Height, Buffer->Memory, &Buffer->Info, DIB_RGB_COLORS,
                SRCCOPY);
}

LRESULT CALLBACK MainWindowCallback(HWND Window, UINT Message, WPARAM WParam,
                                    LPARAM LParam) {
  LRESULT Result = 0;

  switch (Message) {
  case WM_SIZE: {

    break;
  }
  case WM_DESTROY: {
    Running = false;
    OutputDebugString("WM_DESTROY\n");
    break;
  }
  case WM_CLOSE: {
    Running = false;
    OutputDebugString("WM_CLOSE\n");
    break;
  }
  case WM_SYSKEYDOWN:
  case WM_SYSKEYUP:
  case WM_KEYDOWN:
  case WM_KEYUP: {
    Assert(!"Keyboard input came in through a non-dispatch message!");

    break;
  }
  case WM_ACTIVATEAPP: {
    OutputDebugString("WM_ACTIVATEAPP\n");
    break;
  }
  case WM_PAINT: {
    OutputDebugString("WM_PAINT\n");
    PAINTSTRUCT Paint;
    HDC DeviceContext = BeginPaint(Window, &Paint);

    int x = Paint.rcPaint.left;
    int y = Paint.rcPaint.top;
    int Width = Paint.rcPaint.right - Paint.rcPaint.left;
    int Height = Paint.rcPaint.bottom - Paint.rcPaint.top;

    win32_Window_Dimension WindowDimension = win32GetWindowDimension(Window);

    win32DisplayBufferInWindow(&GlobalBackbuffer, DeviceContext,
                               WindowDimension.Width, WindowDimension.Height);

    EndPaint(Window, &Paint);
    break;
  }
  default: {
    Result = DefWindowProc(Window, Message, WParam, LParam);
    break;
  }
  }
  return (Result);
}

inline LARGE_INTEGER win32GetWallClock() {
  LARGE_INTEGER Result;
  QueryPerformanceCounter(&Result);
  return (Result);
}

inline float win32GetSecondsElapsed(LARGE_INTEGER Start, LARGE_INTEGER End) {
  float Result = ((float)(End.QuadPart - Start.QuadPart) /
                  (float)GlobalPerfCountFrequency);
  return (Result);
}

static void win32DebugDrawVertical(win32_offscreeen_buffer *Backbuffer, int X,
                                   int Top, int Bottom, uint32_t Color) {
  uint8_t *Pixel = ((uint8_t *)Backbuffer->Memory +
                    X * Backbuffer->BytesPerPixel + Top * Backbuffer->Pitch);
  for (int Y = Top; Y < Bottom; ++Y) {
    *(uint32_t *)Pixel = Color;
    Pixel += Backbuffer->Pitch;
  }
}

inline void win32DrawSoundBufferMarker(win32_offscreeen_buffer *Backbuffer,
                                       win32_sound_output *SoundOutput, float C,
                                       int PadX, int Top, int Bottom,
                                       DWORD Value, uint32_t Color) {
  Assert(Value < SoundOutput->SecondaryBufferSize);
  float XReal32 = C * ((float)Value);
  int X = PadX + (int)XReal32;
  win32DebugDrawVertical(Backbuffer, X, Top, Bottom, Color);
}

static void win32DebugSyncDisplay(win32_offscreeen_buffer *Backbuffer,
                                  int MarkerCount,
                                  win32_debug_time_marker *Markers,
                                  win32_sound_output *SoundOutput,
                                  float TargetSecondsPerFrame) {
  int PadX = 16;
  int PadY = 16;

  int Top = PadY;
  int Bottom = Backbuffer->Height - PadY;
  float C = (float)(Backbuffer->Width - 2 * PadX) /
            (float)SoundOutput->SecondaryBufferSize;
  for (int MarkerIndex = 0; MarkerIndex < MarkerCount; ++MarkerIndex) {
    win32_debug_time_marker *ThisMarker = &Markers[MarkerIndex];
    win32DrawSoundBufferMarker(Backbuffer, SoundOutput, C, PadX, Top, Bottom,
                               ThisMarker->PlayCursor, 0xFFFFFFFF);
    win32DrawSoundBufferMarker(Backbuffer, SoundOutput, C, PadX, Top, Bottom,
                               ThisMarker->WriteCursor, 0xFFFF0000);
  }
}

int CALLBACK WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                     LPSTR lpCmdLine, int nShowCmd) {
  LARGE_INTEGER PerfCountFrequencyResult;
  QueryPerformanceFrequency(&PerfCountFrequencyResult);
  GlobalPerfCountFrequency = PerfCountFrequencyResult.QuadPart;

  // This sets the windows scheduler granularity to 1ms so Sleep() can be more
  // granular -> D18:53:20
  UINT DesiredSchedulerMs = 1;
  char SleepIsGranular =
      (timeBeginPeriod(DesiredSchedulerMs) == TIMERR_NOERROR);
  WNDCLASS WindowClass = {};

  win32resizeDIBsection(&GlobalBackbuffer, 1280, 720);

  WindowClass.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
  WindowClass.lpfnWndProc = MainWindowCallback;
  WindowClass.hInstance = hInstance;
  // HICON     hIcon;
  WindowClass.lpszClassName = "HandmadeWindowClass";

// TODO how do we reliably query this on windows
#define MonitorRefreshHertz 60
#define GameUpdateHz (MonitorRefreshHertz / 2)
  float TargetSecondsPerFrame = 1.0f / (float)GameUpdateHz;

  RegisterClass(&WindowClass);

  HWND WindowHandle = CreateWindowEx(
      0, WindowClass.lpszClassName, "Handmade",
      WS_OVERLAPPEDWINDOW | WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT,
      CW_USEDEFAULT, CW_USEDEFAULT, 0, 0, hInstance, 0);

  if (WindowHandle != NULL) {

    HDC DeviceContext = GetDC(WindowHandle);

    win32_sound_output SoundOutput = {};

    SoundOutput.SamplesPerSecond = 48000;
    SoundOutput.RunningSampleIndex = 0;
    SoundOutput.BytesPerSample = sizeof(int16_t) * 2;
    SoundOutput.SecondaryBufferSize =
        SoundOutput.SamplesPerSecond * SoundOutput.BytesPerSample;
    SoundOutput.LatencySampleCount =
        3 * (SoundOutput.SamplesPerSecond / GameUpdateHz);

    win32InitDSound(WindowHandle, SoundOutput.SamplesPerSecond,
                    SoundOutput.SecondaryBufferSize);
    Win32ClearBuffer(&SoundOutput);
    GlobalSecondaryBuffer->Play(0, 0, DSBPLAY_LOOPING);
#if 0 // This tests the PlayCursor/WriteCursor update frequency, on my machine
      // -> 480
        while(true)
        {
            DWORD PlayCursor;
            DWORD WriteCursor;
            GlobalSecondaryBuffer->GetCurrentPosition(&PlayCursor, &WriteCursor);
            char TextBuffer[256];
            sprintf_s(TextBuffer, sizeof(TextBuffer), "PC%u WC%u\n", PlayCursor, WriteCursor);
            OutputDebugStringA(TextBuffer);
        }
#endif

    int16_t *Samples =
        (int16_t *)VirtualAlloc(0, SoundOutput.SecondaryBufferSize,
                                MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);

#if HANDMADE_INTERNAL
    LPVOID BaseAddress = (LPVOID)Terabytes((uint64_t)1);
#else
    LPVOID BaseAddress = 0;
#endif

    game_memory GameMemory = {};
    GameMemory.PermanentStorageSize = Megabytes(64);
    GameMemory.TransientStorageSize = Gigabytes(1);
    uint64_t TotalSize =
        GameMemory.PermanentStorageSize + GameMemory.TransientStorageSize;

    GameMemory.PermanentStorage = VirtualAlloc(
        BaseAddress, TotalSize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    GameMemory.TransientStorage = ((uint8_t *)GameMemory.PermanentStorage +
                                   GameMemory.TransientStorageSize);

    if (Samples && GameMemory.PermanentStorage && GameMemory.TransientStorage) {
      game_input Input[2] = {};
      game_input *OldInput = &Input[1];
      game_input *NewInput = &Input[0];

      LARGE_INTEGER LastCounter = win32GetWallClock();

      int DebugTimeMarkerIndex = 0;
      win32_debug_time_marker DebugTimeMarkers[GameUpdateHz / 2] = {};

      DWORD LastPlayCursor = 0;
      DWORD LastWriteCursor = 0;
      char SoundIsValid = false;
      DWORD AudioLatencyBytes = 0;
      float AudioLatencySeconds = 0;

      uint64_t LastCycleCount = __rdtsc();
      Running = true;
      while (Running) {

        game_controller_input *OldKeyboardController =
            GetController(OldInput, 0);
        game_controller_input *NewKeyboardController =
            GetController(NewInput, 0);
        game_controller_input ZeroController = {};
        *NewKeyboardController = ZeroController;
        NewKeyboardController->IsConnected = true;

        for (int ButtonIndex = 0;
             ButtonIndex < ArrayCount(NewKeyboardController->Buttons);
             ++ButtonIndex) {
          NewKeyboardController->Buttons[ButtonIndex].EndedDown =
              OldKeyboardController->Buttons[ButtonIndex].EndedDown;
        }

        Win32ProcessPendingMessages(NewKeyboardController);

        /* NOTE
         * Here is how sound output computation works:
         *
         * We define a safety value that is the number of samples that we think
         * our game update loop may vary by (let's say up to 2ms)
         *
         * When we wake up to write audio, we will look and see what the play
         * cursor position is and we will forecast ahead what we think the play
         * cursor will be after on the next frame boundary.
         *
         * We will then look to see if the write cursor is before that by at
         * least our safety value. If it is, the target fill position is that
         * frame boundary plus one frame. This gives us perfect audio sync in
         * the case of a card that has low enough latency.
         *
         * If the write cursor is after that safety margin, then we assume we
         * can never sync the audio perfectly, so we will write one frame's
         * worth of audio plus the safety margin's worth of guard samples.
         */

        DWORD BytesToWrite = 0;
        DWORD TargetCursor = 0;
        DWORD BytesToLock = 0;
        if (SoundIsValid) {
          BytesToLock =
              (SoundOutput.RunningSampleIndex * SoundOutput.BytesPerSample) %
              SoundOutput.SecondaryBufferSize;
          TargetCursor = (LastPlayCursor + (SoundOutput.LatencySampleCount *
                                            SoundOutput.BytesPerSample)) %
                         SoundOutput.SecondaryBufferSize;

          if (BytesToLock > TargetCursor) {
            BytesToWrite = (SoundOutput.SecondaryBufferSize - BytesToLock);
            BytesToWrite += TargetCursor;
          } else {
            BytesToWrite = TargetCursor - BytesToLock;
          }
        }

        game_sound_output_buffer SoundBuffer = {};
        SoundBuffer.SamplesPerSecond = SoundOutput.SamplesPerSecond;
        SoundBuffer.SampleCountToOutput =
            BytesToWrite / SoundOutput.BytesPerSample;
        SoundBuffer.Samples = Samples;

        game_offscreen_buffer Buffer = {};
        Buffer.Memory = GlobalBackbuffer.Memory;
        Buffer.Width = GlobalBackbuffer.Width;
        Buffer.Height = GlobalBackbuffer.Height;
        Buffer.Pitch = GlobalBackbuffer.Pitch;

        GameUpdateAndRender(
            &GameMemory, NewInput, &Buffer,
            &SoundBuffer); // NOTE(Bob):check declaration when u can

        if (SoundIsValid) {
          // #if HANDMADE_INTERNAL
          DWORD PlayCursor;
          DWORD WriteCursor;
          GlobalSecondaryBuffer->GetCurrentPosition(&PlayCursor, &WriteCursor);

          DWORD UnwrappedWriteCursor = WriteCursor;
          if (UnwrappedWriteCursor < PlayCursor) {
            UnwrappedWriteCursor += SoundOutput.SecondaryBufferSize;
          }
          AudioLatencyBytes = UnwrappedWriteCursor - PlayCursor;
          AudioLatencySeconds =
              (((float)AudioLatencyBytes / (float)SoundOutput.BytesPerSample) /
               (float)SoundOutput.SamplesPerSecond);

          char TextBuffer[256];
          sprintf_s(TextBuffer, sizeof(TextBuffer),
                    "LPC%u BTL%u TC%u BTW%u - PC%u WC%u DELTA%u (%fs)/n",
                    LastPlayCursor, BytesToLock, TargetCursor, BytesToWrite,
                    PlayCursor, WriteCursor, AudioLatencyBytes,
                    AudioLatencySeconds);
          OutputDebugStringA(TextBuffer);
          // #endif
          Win32FillSoundBuffer(&SoundOutput, BytesToLock, BytesToWrite,
                               &SoundBuffer);
        }

        // these device contexts shouldnt be here
        ReleaseDC(WindowHandle, DeviceContext);

        float TestSecondsElapsedPerFrame =
            win32GetSecondsElapsed(LastCounter, win32GetWallClock());

        LARGE_INTEGER WorkCounter = win32GetWallClock();
        float WorkSecondsElapsed =
            win32GetSecondsElapsed(LastCounter, WorkCounter);

        float SecondsElapsedPerFrame = WorkSecondsElapsed;
        if (SecondsElapsedPerFrame < TargetSecondsPerFrame) {
          if (SleepIsGranular) {
            DWORD SleepMs = (DWORD)(1000.0f * (TargetSecondsPerFrame -
                                               SecondsElapsedPerFrame));
            if (SleepMs > 0) // i added the -1, something weird happening with
                             // the sleep calculations
            {
              Sleep(SleepMs); // same here
            }
          }

          TestSecondsElapsedPerFrame =
              win32GetSecondsElapsed(LastCounter, win32GetWallClock());
          // Assert(TestSecondsElapsedPerFrame <= TargetSecondsPerFrame);

          // while (SecondsElapsedPerFrame < TargetSecondsPerFrame)
          //{
          //     SecondsElapsedPerFrame = win32GetSecondsElapsed(LastCounter,
          //     win32GetWallClock());
          // }
        } else {
          // missed frame rate
          // log
        }
        LARGE_INTEGER EndCounter = win32GetWallClock();
        float MSPerFrame =
            1000.0f * win32GetSecondsElapsed(LastCounter, EndCounter);
        LastCounter = EndCounter;

        win32_Window_Dimension WindowDimension =
            win32GetWindowDimension(WindowHandle);

#if HANDMADE_INTERNAL
        win32DebugSyncDisplay(&GlobalBackbuffer, ArrayCount(DebugTimeMarkers),
                              DebugTimeMarkers, &SoundOutput,
                              TargetSecondsPerFrame);
#endif
        win32DisplayBufferInWindow(&GlobalBackbuffer, DeviceContext,
                                   WindowDimension.Width,
                                   WindowDimension.Height); //!

        DWORD PlayCursor;
        DWORD WriteCursor;
        if ((GlobalSecondaryBuffer->GetCurrentPosition(
                &PlayCursor, &WriteCursor)) == DS_OK) {
          LastWriteCursor = WriteCursor;
          LastPlayCursor = PlayCursor;
          if (!SoundIsValid) {
            SoundOutput.RunningSampleIndex =
                WriteCursor / SoundOutput.BytesPerSample;
            SoundIsValid = true;
          }
        } else {
          SoundIsValid = false;
        }

#if HANDMADE_INTERNAL
        {
          Assert(DebugTimeMarkerIndex < ArrayCount(DebugTimeMarkers))
              win32_debug_time_marker *Marker =
                  &DebugTimeMarkers[DebugTimeMarkerIndex++];
          if (DebugTimeMarkerIndex == ArrayCount(DebugTimeMarkers)) {
            DebugTimeMarkerIndex = 0;
          }
          Marker->PlayCursor = PlayCursor;
          Marker->WriteCursor = WriteCursor;
        }
#endif

        game_input *Temp = NewInput;
        NewInput = OldInput;
        OldInput = Temp;

        uint64_t EndCycleCount = __rdtsc();
        uint64_t CyclesElapsed = EndCycleCount - LastCycleCount;
        LastCycleCount = EndCycleCount;
        // #if 0
        float FPS = 0.0f;
        float MCPF = (float)(CyclesElapsed / (1000.0f * 1000.0f));

        char tempBuffer[256];
        sprintf_s(tempBuffer, sizeof(tempBuffer),
                  "%f.02 ms/f; %f.02 f/s; %f.02 mc/f\n", MSPerFrame, FPS, MCPF);
        OutputDebugString(tempBuffer);
        // #endif
      }

    } else {
      // problem
    }

  } else {
    // problem
  }
}
