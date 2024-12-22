#ifndef PLAYER_H
#define PLAYER_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <libswscale/swscale.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswresample/swresample.h>

#include "util.h"
#include "video.h"
#include "queue.h"
#include "stream.h"
#include "decoder.h"

enum player_disable_media {
    PLAYER_DISABLE_NO_MEDIA,
    PLAYER_DISABLE_VIDEO,
    PLAYER_DISABLE_AUDIO
};

typedef struct Player {
    int quit;
    int pause;
    char* filename;
    int quit_reader;
    int refresh_video;
    AVPacket* packet;
    SDL_cond* initial_load_cnd;
    SDL_mutex* initial_load_mx;
    SDL_cond* pause_cnd;
    SDL_mutex* pause_mx;
    SDL_Window* window;
    SDL_Texture* texture;
    SDL_mutex* texture_mx;
    SDL_Surface* surface;
    SDL_Renderer* renderer;
    enum player_disable_media disabled_media;
    
    Queue pktq;
    TTF_Font* font;
    AVFormatContext* fmt_ctx;

    Stream video;
    Queue videoq_frms;
    double video_clock;
    struct SwsContext* sws_ctx;
    Stream audio;
    Queue audioq_frms;
    double audio_clock;
    struct SwrContext* swr_ctx;
    SDL_AudioDeviceID audioDev;
    SDL_AudioSpec audioSpec;
} Player;

typedef struct PlayerThreads {
    SDL_Thread* audio_th;
    SDL_Thread* video_th;
    SDL_Thread* decoder_th;
    SDL_Thread* reader_th;
} PlayerThreads;

Player* player_create(
    char* filename,
    enum player_disable_media disabled
);

int player_start(Player* mediaplayer);
int player_init(Player* mediaplayer);
void player_free(Player* mediaplayer);
int player_create_display(Player* mediaplayer);

#endif // PLAYER_H