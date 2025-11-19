#include "handmade.h"
internal void GameOutputSound(game_state *GameState,
                              game_sound_output_buffer *SoundBuffer,
                              int ToneHz)
{

    int16 ToneVolume = 200;
    int WavePeriod = SoundBuffer->SamplesPerSecond / ToneHz;

    int16 *SampleOut = SoundBuffer->Samples;
    for (int SampleIndex = 0; SampleIndex < SoundBuffer->SampleCount; ++SampleIndex)
    {

#if 0
        float SineValue = sinf(GameState->tSine);
        int16 SampleValue = (int16)(SineValue * ToneVolume);
#else
        int16 SampleValue = 0;
#endif
        *SampleOut++ = SampleValue;
        *SampleOut++ = SampleValue;

#if 0
        GameState->tSine += 2.0f * Pi32 * 1.0f / (real32)WavePeriod;
        if (GameState->tSine > 2.0f * Pi32)
        {
            GameState->tSine -= 2.0f * Pi32;
        }
#endif
    }
}

internal int32 RoundReal32ToInt32(real32 Real32)
{
    // int32 result = (int32)lrintf(Real32); //this uses intrinsic
    int32 Result = 0;
    Result = (int32)(Real32 + 0.5f);
    return Result;
}
internal uint32 RoundReal32ToUInt32(real32 Real32)
{
    // int32 result = (int32)lrintf(Real32); //this uses intrinsic
    uint32 Result = 0;
    Result = (uint32)(Real32 + 0.5f);
    return Result;
}

internal void DrawRectangele(game_offscreen_buffer *Buffer,
                             real32 RealMinX,
                             real32 RealMinY,
                             real32 RealMaxX,
                             real32 RealMaxY,
                             real32 R,
                             real32 G,
                             real32 B)
{
    int32 MinX = RoundReal32ToInt32(RealMinX);
    int32 MinY = RoundReal32ToInt32(RealMinY);
    int32 MaxX = RoundReal32ToInt32(RealMaxX);
    int32 MaxY = RoundReal32ToInt32(RealMaxY);

    if (MinX < 0)
    {
        MinX = 0;
    }
    if (MinY < 0)
    {
        MinY = 0;
    }
    if (MaxX > Buffer->Width)
    {
        MaxX = Buffer->Width;
    }
    if (MaxY > Buffer->Height)
    {
        MaxY = Buffer->Height;
    }

    uint32 Color =
        ((RoundReal32ToUInt32(R * 255.0f) << 16) | (RoundReal32ToUInt32(G * 255.0f) << 8) |
         (RoundReal32ToUInt32(B * 255.0f) << 0));

    uint8 *Row = ((uint8 *)Buffer->Memory + MinX * Buffer->BytesPerPixel + MinY * Buffer->Pitch);
    for (int Y = MinY; Y < MaxY; ++Y)
    {
        uint32 *Pixel = (uint32 *)Row;
        for (int X = MinX; X < MaxX; ++X)
        {
            *Pixel++ = Color;
        }
        Row += Buffer->Pitch;
    }
}

extern "C" GAME_UPDATE_AND_RENDER(GameUpdateAndRender)
{
    Assert((&Input->Controllers[0].Terminator - &Input->Controllers[0].Buttons[0]) ==
           (ArrayCount(Input->Controllers[0].Buttons)));
    // NOTE(Bogdan): This assert checks if the button array in the game_controller_input struct is
    // the same size as the struct it makes a union with

    Assert(sizeof(game_state) <= Memory->PermanentStorageSize);

    game_state *GameState = (game_state *)Memory->PermanentStorage;
    if (!Memory->IsInitialised)
    {
        Memory->IsInitialised = true;
    }

    for (int ControllerIndex = 0; ControllerIndex < 1 /*ArrayCount(Input->Controllers)*/;
         ++ControllerIndex)
    {
        game_controller_input *Controller = GetController(Input, ControllerIndex);
        if (Controller->IsAnalog)
        {
            // we dont do that
        }
        else
        {                           // Keyboard
            real32 dPlayerX = 0.0f; // px/s
            real32 dPlayerY = 0.0f;
            if (Controller->MoveUp.EndedDown)
            {
                dPlayerY = -1.0f;
            }
            if (Controller->MoveDown.EndedDown)
            {
                dPlayerY = 1.0f;
            }
            if (Controller->MoveLeft.EndedDown)
            {
                dPlayerX = -1.0f;
            }
            if (Controller->MoveRight.EndedDown)
            {
                dPlayerX = 1.0f;
            }

            dPlayerX *= 128.0f;
            dPlayerY *= 128.0f;
            GameState->PlayerX += Input->dTForFrame * dPlayerX;
            GameState->PlayerY += Input->dTForFrame * dPlayerY;
        }
    }

    DrawRectangele(Buffer,
                   0.0f,
                   0.0f,
                   (real32)Buffer->Width,
                   (real32)Buffer->Height,
                   1.0f,
                   0.0f,
                   0.1f);

    uint32 TileMap[9][17] = {{1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1, 1},
                             {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
                             {0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1},
                             {0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
                             {0, 1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0},
                             {0, 1, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
                             {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1},
                             {0, 1, 1, 1, 0, 0, 0, 0, 1, 0, 0, 1, 0, 1, 1, 0, 1},
                             {0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1}};
    real32 UpperLeftX = -30;
    real32 UpperLeftY = 0;
    real32 TileWidth = 60;
    real32 TileHeight = 60;

    for (int Row = 0; Row < 9; ++Row)
    {
        for (int Column = 0; Column < 17; ++Column)
        {
            uint32 TileID = TileMap[Row][Column];
            real32 Gray = 0.5f;
            if (TileID == 1)
            {
                Gray = 1.0f;
            }
            real32 MinX = UpperLeftX + ((real32)Column) * TileWidth;
            real32 MinY = UpperLeftY + ((real32)Row) * TileHeight;
            real32 MaxX = MinX + TileWidth;
            real32 MaxY = MinY + TileHeight;
            DrawRectangele(Buffer, MinX, MinY, MaxX, MaxY, Gray, Gray, Gray);
        }
    }

    real32 PlayerR = 1.0f;
    real32 PlayerG = 1.0f;
    real32 PlayerB = 0.0f;
    real32 PlayerWidth = 0.75f * TileWidth;
    real32 PlayerHeight = TileHeight;
    real32 PlayerLeft = GameState->PlayerX - 0.5f * PlayerWidth;
    real32 PlayerTop = GameState->PlayerY - PlayerHeight;

    DrawRectangele(Buffer,
                   PlayerLeft,
                   PlayerTop,
                   PlayerLeft + PlayerWidth,
                   PlayerTop + PlayerHeight,
                   PlayerR,
                   PlayerG,
                   PlayerB);
}

extern "C" GAME_GET_SOUND_SAMPLES(GameGetSoundSamples)
{
    game_state *GameState = (game_state *)Memory->PermanentStorage;
    GameOutputSound(GameState, SoundBuffer, 400);
}

/*
static void RenderWeirdGradient(game_offscreen_buffer *Buffer, int XOffset, int YOffset)
{

    uint8_t *Row = (uint8_t *)Buffer->Memory; // unsigned char == uint8 == 8 bytes unsigned
    for (int y = 0; y < Buffer->Height; ++y)
    {

        uint32_t *Pixel = (uint32_t *)Row;
        for (int x = 0; x < Buffer->Width; ++x)
        {

            uint8_t Blue = x + XOffset;
            uint8_t Green = y + YOffset;

            *Pixel++ = ((Green << 16) | Blue);
        }
        Row += Buffer->Pitch;
    }
}
*/
