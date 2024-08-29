#ifndef UTIL_H
#define UTIL_H

#include <stdio.h>
#include <stdint.h>

#include <SDL2/SDL.h>

#include <libavutil/opt.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswresample/swresample.h>

#include "defs.h"
#include "audio.h"
#include "stream.h"
#include "player.h"
#include "queue.h"

int save_frame(
    uint8_t* buf,
    int linesize,
    int width,
    int height,
    const char* name
);

int decode_video(
    AVCodecContext* ctx,
    AVPacket* packet,
    Queue* videoq
);

int decode_audio(
    SDL_AudioDeviceID dev,
    AVCodecContext* ctx,
    AVPacket* packet,
    struct SwrContext* swr_ctx
);

int context_from_file(
    char* path,
    AVFormatContext* ctx
);

int packet_reader(
    Player* mediaplayer,
    struct SwrContext* swr_ctx
);

#endif