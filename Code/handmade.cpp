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

internal void DrawRectangele(game_offscreen_buffer *Buffer,
                             real32 RealMinX,
                             real32 RealMinY,
                             real32 RealMaxX,
                             real32 RealMaxY,
                             uint32 Color)
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
        { // Keyboard
        }
    }

    DrawRectangele(Buffer, 0.0f, 0.0f, (real32)Buffer->Width, (real32)Buffer->Height, 0x00FF00FF);
    DrawRectangele(Buffer, 10.0f, 10.0f, 30.0f, 30.0f, 0x0000FFFF);
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
