#ifndef PLAYER_H
#define PLAYER_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <libavformat/avformat.h>

#include "stream.h"
#include "queue.h"

typedef struct Player {
    int quit;
    int pause;
    int skip_video_frame;
    AVPacket* packet;
    int quit_decoder;
    SDL_Thread* decoder_id;
    
    SDL_Event event;
    TTF_Font* font;
    AVFormatContext* fmt_ctx;

    Stream video;
    Queue videoq_pkts;
    Queue videoq_frms;
    SDL_Thread* video_id;
    int quit_video_thread;
    int pause_video_thread;
    double video_clock;

    Stream audio;
    Queue audioq_pkts;
    Queue audioq_frms;
    SDL_Thread* audio_id;
    int quit_audio_thread;
    int pause_audio_thread;
    double audio_clock;
    SDL_AudioDeviceID audioDev;
    SDL_AudioSpec audioSpec;
} Player;

Player* create_player();

#endif // PLAYER_H