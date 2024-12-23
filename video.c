#include "video.h"

int decode_video(
    AVCodecContext* ctx,
    AVPacket* packet,
    Queue* videoq
) {
    AVFrame* frame = av_frame_alloc();
    if (!frame) {
        return -1;
    }

    int ret = avcodec_send_packet(ctx, packet);
    if (ret < 0) {
        return ret;
    }
    //printf("decode_video: packet->duration=%d\n", packet->duration);
    //printf("decode_video: packet->size=%d\n", packet->size);

    ret = avcodec_receive_frame(ctx, frame);
    if (ret == AVERROR_EOF) {
        return AVERROR_EOF;
    } else if (ret < 0) {
        return ret;
    }

    

    queue_put(videoq, TO_VPP(&frame));
    return 0;
}

int video_thread(void* data) {
    int ret;
    AVFrame* frame;
    Player* mediaplayer = (Player*)data;

    while (!mediaplayer->quit) {
        if (!mediaplayer->pause) {
            ret = queue_get(&mediaplayer->videoq_frms, TO_VPP(&frame), 1);
            if (ret != 0) {
                continue;
            }
            
            SDL_LockMutex(mediaplayer->texture_mx);
            update_video(
                mediaplayer->renderer,
                mediaplayer->texture,
                frame
            );
            SDL_UnlockMutex(mediaplayer->texture_mx);
            av_frame_free(&frame);
            SDL_RenderPresent(mediaplayer->renderer);
        }
        SDL_Delay(30);
    }

    return 0;
}

int update_video(
    SDL_Renderer* renderer,
    SDL_Texture* texture,
    AVFrame* frame
) {
    SDL_UpdateYUVTexture(
        texture, NULL,
        frame->data[0],
        frame->linesize[0],
        frame->data[1],
        frame->linesize[1],
        frame->data[2],
        frame->linesize[2]
    );
    SDL_RenderCopy(renderer, texture, NULL, NULL);
    return 0;
}

int convert_video(
    AVFrame* src,
    AVFrame* dst,
    AVCodecContext* ctx,
    struct SwsContext* sws_ctx
) {
    sws_scale(
        sws_ctx,
        (uint8_t const* const*)src->data,
        src->linesize, 0,
        ctx->height,
        dst->data,
        dst->linesize
    );

    return 0;
}

double synchronize_video(Player* mediaplayer, AVFrame* frame, double pts) {
    double frame_delay;
    if (pts != 0) {
        mediaplayer->video_clock = pts;
    } else {
        pts = mediaplayer->video_clock;
    }

    frame_delay = av_q2d(mediaplayer->video.ctx->time_base);
    frame_delay += frame->repeat_pict * (frame_delay * 0.5);
    mediaplayer->video_clock += frame_delay;
    return pts;
}