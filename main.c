#include <time.h>
#include <stdio.h>

// SDL2
#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_thread.h>

// FFMPEG
#include <libavcodec/packet.h>
#include <libavutil/imgutils.h>
#include <libavutil/time.h>
#include <libswscale/swscale.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswresample/swresample.h>

// INTERNAL
#include "defs.h"
#include "util.h"
#include "player.h"
#include "stream.h"
#include "queue.h"
#include "video.h"
#include "audio.h"
#include "decoder.h"

const Uint32 MEDIAPLAYER_SDL_FLAGS = \
    SDL_INIT_VIDEO| \
    SDL_INIT_AUDIO| \
    SDL_INIT_EVENTS| \
    SDL_INIT_TIMER;

#define DISPLAY_VIDEO_INFO 0
#define SDL_AUDIO_BUFFER_SIZE 1024
#define AV_SYNC_THRESHOLD 0.01
#define AV_NOSYNC_THRESHOLD 10.0

int main(int argc, char** argv) {
    int error = 0;
    if (argc < 2) {
        printf("Not enough arguments!\n");
        return -1;
    }

    Player* mediaplayer = player_create(argv[1], PLAYER_DISABLE_NO_MEDIA);
    if (!mediaplayer) {
        error = -1;
        printf("main: player_create -1\n");
        goto free;
    }

    if (context_from_file(mediaplayer->filename, mediaplayer->fmt_ctx)) {
        printf(
            "main: error opening file '%s'\n",
            mediaplayer->filename
        );
        goto free;
    };

    error = find_streams(
        mediaplayer->fmt_ctx,
        mediaplayer->disabled_media != PLAYER_DISABLE_VIDEO ? &mediaplayer->video : NULL,
        mediaplayer->disabled_media != PLAYER_DISABLE_AUDIO ? &mediaplayer->audio : NULL
    );
    if (error < 0) {
        printf(
            "main: error finding video streams in '%s'\n",
            mediaplayer->filename
        );
        goto free;
    }
    
    if (mediaplayer->audio.index != -1) {
        error = init_stream(&mediaplayer->audio);
        if (error < 0) {
            error = -1;
            goto free;
        }
    }

    if (mediaplayer->video.index != -1) {
        error = init_stream(&mediaplayer->video);
        if (error < 0) {
            error = -1;
            goto free;
        }
    }

    if (player_init(mediaplayer) < 0) {
        error = -1;
        goto free;
    }
    
    // context for audio resampling
    error = swr_alloc_set_opts2(
        &mediaplayer->swr_ctx,
        (const AVChannelLayout*)&mediaplayer->audio.ctx->ch_layout,
        AV_SAMPLE_FMT_S16, 44100,
        (const AVChannelLayout*)&mediaplayer->audio.ctx->ch_layout,
        mediaplayer->audio.ctx->sample_fmt,
        mediaplayer->audio.ctx->sample_rate,
        0, NULL
    );
    if (error < 0) {
        printf("main: swr_alloc_set_opts2 %s\n", av_err2str(error));
        error = -1;
        goto free;
    }

    error = swr_init(mediaplayer->swr_ctx);
    if (error < 0) {
        printf("main: swr_init %s\n", av_err2str(error));
        error = -1;
        goto free;
    }

    //pixel format conversion
    mediaplayer->sws_ctx = sws_getContext(
        mediaplayer->video.ctx->width,
        mediaplayer->video.ctx->height,
        mediaplayer->video.ctx->pix_fmt,
        mediaplayer->video.ctx->width,
        mediaplayer->video.ctx->height,
        AV_PIX_FMT_YUV420P,
        SWS_BILINEAR,
        NULL, NULL, NULL
    );
    if (!mediaplayer->sws_ctx) {
        printf("main: error creating SwsContext\n");
        error = -1;
        goto free;
    }

    error = player_start(mediaplayer);

    free:
    player_free(mediaplayer);
    
    exit:
    return error;
}