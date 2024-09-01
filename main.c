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

Uint32 PLAYER_EVENT_REFRESH_VIDEO;

void handle_events(Player* mediaplayer) {
    SDL_Event event;
    
    while(SDL_PollEvent(&event)) {
        switch (event.type) {
        case SDL_KEYUP:
            if (
                event.key.keysym.scancode ==
                SDL_SCANCODE_Q
            ) {
                mediaplayer->quit = 1;
            } else if (
                event.key.keysym.scancode ==
                SDL_SCANCODE_SPACE
            ) {
                mediaplayer->pause = !mediaplayer->pause;
                if (mediaplayer->pause) {
                    SDL_PauseAudioDevice(
                        mediaplayer->audioDev,
                        SDL_TRUE
                    );
                    continue;
                }

                SDL_PauseAudioDevice(
                    mediaplayer->audioDev,
                    SDL_FALSE
                );
            }
            break;
        case SDL_QUIT:
            mediaplayer->quit = 1;
            mediaplayer->quit_reader = 1;
            mediaplayer->quit_video_thread = 1;
            break;
        }
    }
}

int player(Player* mediaplayer) {
    int ret = 0;
    int pause = 0;
    int error = 0;
    SDL_Event event;
    int current_stage = 0;
    int skip_video_frame = 0;
    PlayerThreads threads;
    SDL_memset(&threads, 0, sizeof(threads));

    PLAYER_EVENT_REFRESH_VIDEO = SDL_RegisterEvents(1);
    if (PLAYER_EVENT_REFRESH_VIDEO < 0) {
        printf("player: SDL_RegisterEvents not enough user events\n");
    }

    /*
        -----------------------------------
        INIT SDL, WINDOW, RENDERER, TEXTURE
        -----------------------------------
    */
    if (SDL_Init(MEDIAPLAYER_SDL_FLAGS)) {
        printf("player: SDL_Init %s\n", SDL_GetError());
        return -1;
    }

    if (TTF_Init() < 0) {
        printf("player: TTF_Init %s\n", TTF_GetError());
        ret = -1;
        goto free;
    }

    mediaplayer->font = TTF_OpenFont("assets/ubuntu.ttf", 30);
    if (!mediaplayer->font) {
        printf("player: TTF_OpenFont returned error\n");
        ret = -1;
        goto free;
    }
    TTF_SetFontHinting(mediaplayer->font, TTF_HINTING_NORMAL);

    mediaplayer->window = SDL_CreateWindow(
        "MediaPlayer",
        SDL_WINDOWPOS_UNDEFINED,
        SDL_WINDOWPOS_UNDEFINED,
        mediaplayer->video.ctx->width,
        mediaplayer->video.ctx->height,
        SDL_WINDOW_SHOWN
    );
    if (!mediaplayer->window) {
        printf("SDL_CreateWindow: %s\n", SDL_GetError());
        ret = -1;
        goto free;
    }

    mediaplayer->renderer = SDL_CreateRenderer(mediaplayer->window, -1, SDL_RENDERER_ACCELERATED);
    if (!mediaplayer->renderer) {
        printf("SDL_CreateRenderer: %s\n", SDL_GetError());
        ret = -1;
        goto free;
    }

    mediaplayer->texture = SDL_CreateTexture(
        mediaplayer->renderer,
        SDL_PIXELFORMAT_IYUV,
        SDL_TEXTUREACCESS_STATIC,
        mediaplayer->video.ctx->width,
        mediaplayer->video.ctx->height
    );
    if (!mediaplayer->texture) {
        printf("error creating sdl texture\n");
        ret = -1;
        goto free;
    }

    SDL_SetRenderDrawColor(
        mediaplayer->renderer,
        0, 0, 0,
        SDL_ALPHA_OPAQUE
    );
    SDL_RenderClear(mediaplayer->renderer);

    open_audio_dev(
        &mediaplayer->audio,
        &mediaplayer->audioSpec,
        &mediaplayer->audioDev
    );

    SDL_Color color = {255, 255, 255};
    SDL_Surface* message = TTF_RenderText_Blended(
        mediaplayer->font,
        "Hello World", color
    );
    SDL_Texture* message_texture = SDL_CreateTextureFromSurface(mediaplayer->renderer, message);
    SDL_Rect srcrect = message->clip_rect;
    SDL_Rect dstrect = message->clip_rect;
    dstrect.x = 0;
    dstrect.y = 0;
    SDL_FreeSurface(message);

    // create threads for reading and decoding packets
    threads.decoder_th = SDL_CreateThread(packet_decoder, "packet_reader", (void*)mediaplayer);
    if (!threads.decoder_th) {
        ret = -1;
        goto free; 
    }

    //threads.audio_th = SDL_CreateThread(audio_thread, "audio_thread", (void*)mediaplayer);
    //threads.video_th = SDL_CreateThread(video_thread, "video_thread", (void*)mediaplayer);
    threads.video_refresh_th = SDL_CreateThread(video_refresh_thread, "video_refresh_thread", (void*)mediaplayer);
    if (!threads.video_refresh_th) {
        ret -1;
        goto free;
    }

    /*
        -------------
        SDL EVENTLOOP
        -------------
    */
    while (!mediaplayer->quit) {
        handle_events(mediaplayer);

        if (mediaplayer->quit) {
            break;
        }
        if (mediaplayer->pause) {
            SDL_Delay(100);
            continue;
        }
    }


    int th_status = 0;
    if (threads.video_th) {
        SDL_WaitThread(threads.video_th, &th_status);
        printf("player: SDL_WaitThread Video Thread returned with %d\n", th_status);
    }
    if (threads.audio_th) {
        SDL_WaitThread(threads.audio_th, &th_status);
        printf("player: SDL_WaitThread Audio Thread returned with %d\n", th_status);
    }
    if (threads.decoder_th) {
        SDL_WaitThread(threads.decoder_th, &th_status);    
        printf("player: SDL_WaitThread Decoder Thread returned with %d\n", th_status);
    }
    if (threads.video_refresh_th) {
        SDL_WaitThread(threads.video_refresh_th, &th_status);
        printf("player: SDL_WaitThread Video Refresh Thread returned %d\n", th_status);
    }

    close_audio_dev(mediaplayer->audioDev);
    queue_free(&mediaplayer->videoq_frms);
    queue_free(&mediaplayer->audioq_frms);
    //queue_free(&mediaplayer->videoq_pkts);
    //queue_free(&mediaplayer->audioq_pkts);
    

    ret = 0;

    free:
    SDL_FreeSurface(message);
    SDL_DestroyTexture(message_texture);

    SDL_DestroyRenderer(mediaplayer->renderer);
    SDL_DestroyWindow(mediaplayer->window);
    TTF_CloseFont(mediaplayer->font);
    if (SDL_WasInit(MEDIAPLAYER_SDL_FLAGS)) SDL_Quit();
    if (TTF_WasInit()) TTF_Quit();

    return ret;
}

/* printf(
    "->%d\n",
    av_image_get_buffer_size(AV_PIX_FMT_YUV422P, 1920, 1080, 1)
); */
int main(int argc, char** argv) {
    int error = 0;
    if (argc < 2) {
        printf("Not enough arguments!\n");
        return -1;
    }

    Player* mediaplayer = create_player();
    if (!mediaplayer) {
        error = -1;
        goto free;
    }

    mediaplayer->sws_ctx = NULL;
    mediaplayer->swr_ctx = NULL;

    //queue_init(&mediaplayer->audioq_pkts, QUEUE_PACKET, &mediaplayer->quit);
    /* queue_init(
        &mediaplayer->videoq_pkts,
        QUEUE_PACKET, &mediaplayer->quit,
        "video", PLAYER_VIDEOQ_PKTS_MAX_LEN
    ); */
    queue_init(
        &mediaplayer->audioq_frms,
        QUEUE_FRAME, &mediaplayer->quit,
        "audio", PLAYER_AUDIOQ_FRMS_MAX_LEN
    );
    queue_init(
        &mediaplayer->videoq_frms,
        QUEUE_FRAME, &mediaplayer->quit,
        "video", PLAYER_VIDEOQ_FRMS_MAX_LEN
    );

    char* filename = argv[1];
    if (context_from_file(filename, mediaplayer->fmt_ctx)) {
        printf("error opening '%s' and creating video context\n", filename);
        goto free;
    };

    error = find_streams(
        mediaplayer->fmt_ctx,
        &mediaplayer->video,
        &mediaplayer->audio
    );
    if (error < 0) {
        printf("error finding video streams in '%s'\n", filename);
        goto free;
    }
    
    error = init_stream(&mediaplayer->video);
    if (error < 0) {
        error = -1;
        goto free;
    }
    
    error = init_stream(&mediaplayer->audio);
    if (error < 0) {
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
        printf("error main: swr_alloc_set_opts2 %s (returned %d)\n", av_err2str(error), error);
        error = -1;
        goto free;
    }

    error = swr_init(mediaplayer->swr_ctx);
    if (error < 0) {
        printf("error main: swr_init %s (returned %d)\n", av_err2str(error), error);
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

    error = player(mediaplayer);
    if (error < 0) {
        error = -1;
    } else {
        error = 0;
    }

    free:
    if (mediaplayer->fmt_ctx) {
        avformat_free_context(mediaplayer->fmt_ctx);
    }
    if (mediaplayer) {
        av_free(mediaplayer);
    }
    free_stream(&mediaplayer->video);
    free_stream(&mediaplayer->audio);
    swr_close(mediaplayer->swr_ctx);
    swr_free(&mediaplayer->swr_ctx);
    sws_freeContext(mediaplayer->sws_ctx);

    exit:
    return error;
}