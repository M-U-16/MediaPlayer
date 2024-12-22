#ifndef VIDEO_H
#define VIDEO_H

#include "queue.h"
#include "player.h"
#include <libswscale/swscale.h>
#include <libavutil/avutil.h>
#include <libavcodec/avcodec.h>

int decode_video(
    AVCodecContext* ctx,
    AVPacket* packet,
    Queue* videoq
);

int convert_video(
    AVFrame* src,
    AVFrame* dst,
    AVCodecContext* ctx,
    struct SwsContext* sws_ctx
);

int update_video(
    SDL_Renderer* renderer,
    SDL_Texture* texture,
    AVFrame* frame
);

int video_thread(void* data);

#endif // VIDEO_H