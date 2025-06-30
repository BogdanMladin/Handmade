#include "handmade.h"
static void GameOutputSound(game_state *GameState, game_sound_output_buffer *SoundBuffer,
                            int ToneHz)
{

    int16_t ToneVolume = 200;
    int WavePeriod = SoundBuffer->SamplesPerSecond / ToneHz;

    int16_t *SampleOut = SoundBuffer->Samples;
    for (int Sampleindex = 0; Sampleindex < SoundBuffer->SampleCountToOutput; ++Sampleindex)
    {

        float SineValue = sinf(GameState->tSine);
        uint16_t SampleValue = (int16_t)(SineValue * ToneVolume);
        *SampleOut++ = SampleValue;
        *SampleOut++ = SampleValue;

        GameState->tSine += 2.0f * Pi32 * 1.0f / (float)WavePeriod;
        if (GameState->tSine > 2.0f * Pi32)
        {
            GameState->tSine -= 2.0f * Pi32;
        }
    }
}

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

            *Pixel++ = ((Green << 8) | Blue);
        }
        Row += Buffer->Pitch;
    }
}

extern "C" GAME_UPDATE_AND_RENDER(GameUpdateAndRender)
{
    // Assert((&Input->Controllers[0].Terminator -
    // &Input->Controllers[0].Buttons[0]) ==
    // (ArrayCount(Input->Controllers[0].Buttons)));
    Assert(sizeof(game_state) <= Memory->PermanentStorageSize);

    game_state *GameState = (game_state *)Memory->PermanentStorage;

    if (!Memory->IsInitialised)
    {
        char *Filename = __FILE__;
        debug_read_file_result File = Memory->DEBUGPlatformReadEntireFile(Filename);
        if (File.Contents)
        {
            Memory->DEBUGPlatformWriteEntireFile("w:/handmade/data/test.out", File.ContentSize,
                                                 File.Contents);
            Memory->DEBUGPlatformFreeFileMemory(File.Contents);
        }

        GameState->ToneHz = 256;
        GameState->GreenOffset = 0; // NOTE(Bogdan): this shouldn't be here
        GameState->BlueOffset = 0;  // NOTE(Bogdan): this shouldn't be here
        GameState->tSine = 0.0f;
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
            if (Controller->MoveLeft.EndedDown)
            {
                GameState->BlueOffset -= 1;
            }
            if (Controller->MoveRight.EndedDown)
            {
                GameState->BlueOffset += 1;
            }
        }
    }

    /*if (Input.AButtonEndedDown)
    {

    }*/

    // TODO: allow sample offsets heere
    RenderWeirdGradient(Buffer, GameState->BlueOffset, GameState->GreenOffset);
}

extern "C" GAME_GET_SOUND_SAMPLES(GameGetSoundSamples)
{
    game_state *GameState = (game_state *)Memory->PermanentStorage;
    GameOutputSound(GameState, SoundBuffer, GameState->ToneHz);
}
