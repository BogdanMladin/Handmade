#include "handmade.h"

static void GameOutputSound(game_sound_output_buffer* SoundBuffer, int ToneHz)
{

    static float tsine;
    int16_t ToneVolume = 200;
    int WavePeriod = SoundBuffer->SamplesPerSecond / ToneHz;

    int16_t* SampleOut = SoundBuffer->Samples;
    for (int Sampleindex = 0; Sampleindex < SoundBuffer->SampleCountToOutput; ++Sampleindex)
    {


        float SineValue = sinf(tsine);
        uint16_t SampleValue = (int16_t)(SineValue * ToneVolume);
        *SampleOut++ = SampleValue;
        *SampleOut++ = SampleValue;

        tsine += 2.0f * Pi32 * 1.0f / (float)WavePeriod;
    }
}

static void RenderWeirdGradient(game_offscreen_buffer *Buffer, int XOffset, int YOffset) {

    uint8_t* Row = (uint8_t*)Buffer->Memory; // unsigned char == uint8 == 8 bytes unsigned
    for (int y = 0; y < Buffer->Height; ++y) {

        uint32_t* Pixel = (uint32_t*)Row;
        for (int x = 0; x < Buffer->Width; ++x) {

            uint8_t Blue = x + XOffset;
            uint8_t Green = y + YOffset;

            *Pixel++ = ((Green << 8) | Blue);

        }
        Row += Buffer->Pitch;

    }
}

static void GameUpdateAndRender(game_memory *Memory, game_input* Input, game_offscreen_buffer* Buffer, game_sound_output_buffer* SoundBuffer)
{
    //Assert((&Input->Controllers[0].Terminator - &Input->Controllers[0].Buttons[0]) == (ArrayCount(Input->Controllers[0].Buttons)));
    Assert(sizeof(game_state) <= Memory->PermanentStorageSize);
    game_state* GameState = (game_state*)Memory->PermanentStorage;
    if (!Memory->IsInitialised)
    {
        char* Filename = __FILE__;
        debug_read_file_result File = DEBUGPlatformReadEntireFile(Filename);
        if (File.Contents)
        {
            DEBUGPlatformWriteEntireFile("w:/handmade/data/test.out", File.ContentSize, File.Contents);
            DEBUGPlatformFreeFileMemory(File.Contents);
        }

        GameState->ToneHz = 256;
        GameState->GreenOffset = 0;
        GameState->BlueOffset = 0;
        Memory->IsInitialised = true;
    }

    for (int ControllerIndex = 0; ControllerIndex < 1 /*ArrayCount(Input->Controllers)*/; ++ControllerIndex) {
        game_controller_input* Controller = GetController(Input, ControllerIndex);
        if (Controller->IsAnalog) {
            //we dont do that
        }
        else { // Keyboard
            if (Controller->MoveLeft.EndedDown) {
                GameState->BlueOffset -= 1;
            }
            if (Controller->MoveRight.EndedDown) {
                GameState->BlueOffset += 1;
            }
        }
    }

    /*if (Input.AButtonEndedDown)
    {

    }*/

    //TODO: allow sample offsets heere
    GameOutputSound(SoundBuffer, GameState->ToneHz);
    RenderWeirdGradient(Buffer, GameState->BlueOffset, GameState->GreenOffset);
}