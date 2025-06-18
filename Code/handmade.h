/*
    NOTE:

    HANDMADE_INTERNAL:
     0 - Build for public release
     1 - Build for developer only

    HANDMADE_SLOW:
     0 - Slow code not allowed
     1 - Slow code allowed


*/

#include <dsound.h>
#include <malloc.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <windows.h>
#include <wingdi.h>

#if HANDMADE_SLOW
#define Assert(Expression)                                                     \
  if (!(Expression)) {                                                         \
    *(int *)0 = 0;                                                             \
  }
#else
#define Assert(Expression)
#endif

// Utilities
#define Kilobytes(Value) ((Value) * 1024LL)
#define Megabytes(Value) (Kilobytes(Value) * 1024LL)
#define Gigabytes(Value) (Megabytes(Value) * 1024LL)
#define Terabytes(Value) (Gigabytes(Value) * 1024LL)
#define Pi32 3.14159265359f

#define ArrayCount(Array) (sizeof(Array) / sizeof((Array)[0]))

inline uint32_t SafeTruncateUint64(uint16_t Value) {
  Assert(Value <= 0xFFFFFFFF);
  uint32_t Result = (uint32_t)Value;
  return (Result);
}

/*
    Services Platform to Game
*/

// #if HANDMADE_INTERNAL // Commented this to make the Lsp work
/*
    Theese should not be used while shipping, they are blocking and the write
   doesn't protect against lost data, see d15-1:00:09
*/
struct debug_read_file_result {
  uint32_t ContentSize;
  void *Contents;
};

static debug_read_file_result DEBUGPlatformReadEntireFile(char *Filename);
static void DEBUGPlatformFreeFileMemory(void *Memory);
static float DEBUGPlatformWriteEntireFile(char *Filename, uint32_t MemorySize,
                                          void *Memory);
// #endif // HANDMADE_INTERNAL

/*
    Services Game to Platform
*/

struct game_offscreen_buffer {
  void *Memory;
  int Width;
  int Height;
  int Pitch;
  int BytesPerPixel;
};

struct game_sound_output_buffer {
  int SamplesPerSecond;
  int SampleCountToOutput;
  int16_t *Samples;
};

struct game_button_state {
  int HalfTransitionCount;
  char EndedDown;
};

struct game_controller_input {
  char IsAnalog;
  char IsConnected;

  float StartX;
  float StartY;

  float MinX;
  float MinY;

  float MaxX;
  float MaxY;

  float EndX;
  float EndY;

  union {
    game_button_state Buttons[6];
    struct {
      game_button_state MoveUp;
      game_button_state MoveDown;
      game_button_state MoveLeft;
      game_button_state MoveRight;

      game_button_state ActionUp;
      game_button_state ActionDown;
      game_button_state ActionLeft;
      game_button_state ActionRight;

      game_button_state LeftShoulder;
      game_button_state RightShoulder;

      game_button_state Terminator; // All buttons must be added above this line
    };
  };
};

struct game_memory {
  char IsInitialised;
  uint64_t PermanentStorageSize;
  void *PermanentStorage; // REQUIRED to be cleared to 0 at startup
  uint64_t TransientStorageSize;
  void *TransientStorage; // REQUIRED to be cleared to 0 at startup
};

struct game_state {
  int ToneHz;
  int GreenOffset;
  int BlueOffset;
};

struct game_input {
  game_controller_input Controllers[1];
};
inline game_controller_input *GetController(game_input *Input,
                                            int ControllerIndex) {
  Assert(ControllerIndex < ArrayCount(Input->Controllers));
  game_controller_input *Result = &Input->Controllers[ControllerIndex];
  return Result;
}

static void GameUpdateAndRender(game_memory *Memory, game_input *Input,
                                game_offscreen_buffer *Buffer);

static void GameGetSoundSamples(game_memory *Memory,
                                game_sound_output_buffer *SoundBuffer);
