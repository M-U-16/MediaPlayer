#include <math.h>
#include <time.h>
#include <stdio.h>
#include <conio.h>

#define M_PI 3.14159265358979323846
#define SAMPLE_RATE 44100
#define BUFFER_SIZE 4096

#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>
#include <SDL2/SDL_thread.h>

const float FREQ = SAMPLE_RATE/440.00f;
const float STEP_SIZE = (2*M_PI) / FREQ;

int quit = 0;
float amplitude = 0.2f;
float current_step = 0;

static inline float oscillate(float rate, float volumne) {
    current_step += STEP_SIZE;
    return sinf(current_step);
    
}

void callback(void* userdata, Uint8* stream, int len) {
    printf("%d\n", len);
    float* fstream = (float*)stream;

    for (int i = 0; i < BUFFER_SIZE; i++) {
        fstream[i] = oscillate(STEP_SIZE, amplitude);
    }
}

int cmd_input_loop(void* data) {
    while (!quit) {
        int input = getch();
        char input_c = (char)input;

        if (input_c == 'w') {
            amplitude += 0.1;
            if (amplitude > 1.0f)
                amplitude = 0.0f;
        } else if (input_c == 's') {
            amplitude -= 0.1;
            if (amplitude < 0.0f)
                amplitude = 1.0f;
        } else if (input_c == 'q') {
            quit = 1;
            break;
        }
    }

    return 0;
}

int main(int argc, char** argv) {

    if (SDL_Init(SDL_INIT_AUDIO|SDL_INIT_EVENTS) < 0) {
        printf("main: SDL_Init %s\n", SDL_GetError());
        return -1;
    }

    SDL_Thread* cmd_th = SDL_CreateThread(
        cmd_input_loop,
        "sound-cmd-input",
        &amplitude
    );

    SDL_AudioSpec audio_spec;
    SDL_AudioSpec wanted_spec;
    wanted_spec.format = AUDIO_F32;
    wanted_spec.channels = 1;
    wanted_spec.samples = BUFFER_SIZE;
    wanted_spec.freq = SAMPLE_RATE;
    wanted_spec.callback = callback;

    SDL_AudioDeviceID dev = SDL_OpenAudioDevice(
        NULL, SDL_FALSE,
        &wanted_spec,
        &audio_spec,
        SDL_FALSE
    );
    if (!dev) {
        printf("main: SDL_OpenAudioDevice %s\n", SDL_GetError());
    }

    SDL_PauseAudioDevice(dev, SDL_FALSE);
    while (1) {
        if (quit) {
            SDL_PauseAudioDevice(dev, SDL_TRUE);
            break;
        }
        printf("%f\n", amplitude);
        SDL_Delay(100);
    }

    int cmd_ret;
    SDL_WaitThread(cmd_th, &cmd_ret);

    SDL_CloseAudioDevice(dev);
    SDL_Quit();

    return 0;
}