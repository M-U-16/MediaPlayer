#ifndef PLAYER_H
#define PLAYER_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <libswscale/swscale.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswresample/swresample.h>

#include "stream.h"
#include "queue.h"

typedef struct Player {
    int quit;
    int pause;
    int quit_reader;
    int refresh_video;
    AVPacket* packet;
    SDL_cond* initial_load;

    SDL_Window* window;
    SDL_Texture* texture;
    SDL_Surface* surface;
    SDL_Renderer* renderer;
    
    TTF_Font* font;
    AVFormatContext* fmt_ctx;

    Stream video;
    Queue videoq_pkts;
    Queue videoq_frms;
    int quit_video_thread;
    double video_clock;
    struct SwsContext* sws_ctx;

    Stream audio;
    Queue audioq_pkts;
    Queue audioq_frms;
    int quit_audio_thread;
    double audio_clock;
    struct SwrContext* swr_ctx;
    SDL_AudioDeviceID audioDev;
    SDL_AudioSpec audioSpec;
} Player;

typedef struct PlayerThreads {
    SDL_Thread* audio_th;
    SDL_Thread* video_th;
    SDL_Thread* video_refresh_th;
    SDL_Thread* decoder_th;
} PlayerThreads;

Player* create_player();

#endif // PLAYER_H