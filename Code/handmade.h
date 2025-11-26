#if !defined(HANDMADE_H)
/*
    NOTE:

    HANDMADE_INTERNAL:
     0 - Build for public release
     1 - Build for developer only

    HANDMADE_SLOW:
     0 - Slow code not allowed
     1 - Slow code allowed
*/

#include "handmade_platform.h"

#define internal static
#define local_persist static
#define global_variable static

#define Pi32 3.14159265359f

#if HANDMADE_SLOW
#define Assert(Expression)                                                                         \
    if (!(Expression))                                                                             \
    {                                                                                              \
        *(int *)0 = 0;                                                                             \
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

inline uint32 SafeTruncateUint64(uint64 Value)
{
    Assert(Value <= 0xFFFFFFFF);
    uint32_t Result = (uint32_t)Value;
    return (Result);
}

inline game_controller_input *GetController(game_input *Input, int ControllerIndex)
{
    Assert(ControllerIndex < ArrayCount(Input->Controllers));
    game_controller_input *Result = &Input->Controllers[ControllerIndex];
    return Result;
}

struct tile_map
{
    int32 CountX;
    int32 CountY;

    real32 UpperLeftX = -30;
    real32 UpperLeftY = 0;
    real32 TileWidth = 60;
    real32 TileHeight = 60;

    uint32 *Tiles;
};

struct world
{
    int32 TileMapCountX;
    int32 TileMapCountY;

    tile_map *TileMaps;
};

struct game_state
{
    real32 PlayerX;
    real32 PlayerY;
};

#define HANDMADE_H
#endif
