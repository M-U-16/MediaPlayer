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
#include "convert.h"
#include "queue.h"

const Uint32 MEDIAPLAYER_SDL_FLAGS = \
    SDL_INIT_VIDEO| \
    SDL_INIT_AUDIO| \
    SDL_INIT_TIMER;

#define DISPLAY_VIDEO_INFO 0
#define SDL_AUDIO_BUFFER_SIZE 1024
#define AV_SYNC_THRESHOLD 0.01
#define AV_NOSYNC_THRESHOLD 10.0

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
            break;
        }
    }
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
}

int player(
    Player* mediaplayer,
    struct SwsContext* sws_ctx,
    struct SwrContext* swr_ctx,
    AVPacket* packet
) {
    int ret = 0;
    int pause = 0;
    int error = 0;
    SDL_Event event;
    int current_stage = 0;
    int skip_video_frame = 0;
    SDL_Window* window = NULL;
    SDL_Surface* surface = NULL;
    SDL_Texture* texture = NULL;
    SDL_Renderer* renderer = NULL;

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

    window = SDL_CreateWindow(
        "MediaPlayer",
        SDL_WINDOWPOS_UNDEFINED,
        SDL_WINDOWPOS_UNDEFINED,
        mediaplayer->video.ctx->width,
        mediaplayer->video.ctx->height,
        SDL_WINDOW_SHOWN
    );
    if (!window) {
        printf("SDL_CreateWindow: %s\n", SDL_GetError());
        ret = -1;
        goto free;
    }

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        printf("SDL_CreateRenderer: %s\n", SDL_GetError());
        ret = -1;
        goto free;
    }

    texture = SDL_CreateTexture(
        renderer,
        SDL_PIXELFORMAT_IYUV,
        SDL_TEXTUREACCESS_STATIC,
        mediaplayer->video.ctx->width,
        mediaplayer->video.ctx->height
    );
    if (!texture) {
        printf("error creating sdl texture\n");
        ret = -1;
        goto free;
    }

    SDL_SetRenderDrawColor(renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
    SDL_RenderClear(renderer);

    /*
        ----------------
        SETTING UP AUDIO
        ----------------
    */

    SDL_AudioSpec wanted_audioSpec;
    SDL_zero(wanted_audioSpec);

    wanted_audioSpec.freq = mediaplayer->audio.ctx->sample_rate;
    wanted_audioSpec.channels = mediaplayer->audio.codec->ch_layouts->nb_channels;
    wanted_audioSpec.format = AUDIO_S32SYS;
    wanted_audioSpec.silence = 0;
    
    mediaplayer->audioDev = SDL_OpenAudioDevice(
        NULL, SDL_FALSE,
        &wanted_audioSpec,
        &mediaplayer->audioSpec,
        SDL_FALSE
    );
    if (!mediaplayer->audioDev) {
        printf("player: SDL_OpenAudioDevice, %s\n", SDL_GetError());
        ret = -1;
        goto free;
    }

    SDL_Color color = {255, 255, 255};
    SDL_Surface* message = TTF_RenderText_Blended(
        mediaplayer->font,
        "Hello World", color
    );
    SDL_Texture* message_texture = SDL_CreateTextureFromSurface(renderer, message);

    SDL_Rect srcrect = message->clip_rect;
    SDL_Rect dstrect = message->clip_rect;
    dstrect.x = 0;
    dstrect.y = 0;
    SDL_FreeSurface(message);

    // create thread for packet reading
    mediaplayer->decoder_id = SDL_CreateThread(packet_reader, "packet_reader", mediaplayer);
    //mediaplayer->audio_id = SDL_CreateThread(audio_thread, "audio_decoder", mediaplayer);
    //mediaplayer->audio_id = SDL_CreateThread(audio_thread, "video_thread", mediaplayer);

    /*
        -------------
        SDL EVENTLOOP
        -------------
    */
    while (!mediaplayer->quit) {
        handle_events(mediaplayer);

        if (mediaplayer->quit) break;
        if (mediaplayer->pause) {
            SDL_Delay(100);
            continue;
        }
        
        /* error = av_read_frame(mediaplayer->fmt_ctx, packet);
        if (error == AVERROR_EOF || error == AVERROR(EAGAIN)) {
            skip_video_frame = 1;
            mediaplayer->quit = 1;
        } else {
            if (packet->stream_index == mediaplayer->video.index) {
                error = decode_video(
                    mediaplayer->video.ctx,
                    packet,
                    mediaplayer->video.frame
                );

                if (error < 0) {
                    skip_video_frame = 1;
                }
            } else if (packet->stream_index == mediaplayer->audio.index) {
                decode_audio(
                    mediaplayer->audioDev,
                    mediaplayer->audio.ctx,
                    packet,
                    swr_ctx
                );
            }
        } */


        SDL_RenderClear(renderer);
        if (!skip_video_frame) {
            update_video(
                renderer, texture,
                mediaplayer->video.frame
            );
        }
        SDL_RenderPresent(renderer);
        
        skip_video_frame = 0;
        SDL_Delay(10);
    }

    ret = 0;

    free:
    SDL_FreeSurface(message);
    SDL_DestroyTexture(message_texture);

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_CloseFont(mediaplayer->font);
    if (SDL_WasInit(MEDIAPLAYER_SDL_FLAGS)) SDL_Quit();
    if (TTF_WasInit()) TTF_Quit();

    return ret;
}

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

    struct SwsContext* sws_ctx = NULL;
    struct SwrContext* swr_ctx = NULL;

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

    AVPacket* packet = av_packet_alloc();
    if (!packet) {
        printf("main: av_packet_alloc ENOMEM \n");
        error = -1;
        goto free;
    }

    // context for audio resampling
    error = swr_alloc_set_opts2(
        &swr_ctx,
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

    error = swr_init(swr_ctx);
    if (error < 0) {
        printf("error main: swr_init %s (returned %d)\n", av_err2str(error), error);
        error = -1;
        goto free;
    }

    // context for pixel format conversion
    sws_ctx = sws_getContext(
        mediaplayer->video.ctx->width,
        mediaplayer->video.ctx->height,
        mediaplayer->video.ctx->pix_fmt,
        mediaplayer->video.ctx->width,
        mediaplayer->video.ctx->height,
        AV_PIX_FMT_YUV420P,
        SWS_BILINEAR,
        NULL, NULL, NULL
    );
    if (!sws_ctx) {
        printf("main: error creating SwsContext\n");
        error = -1;
        goto free;
    }

    error = player(
        mediaplayer,
        sws_ctx, swr_ctx,
        packet
    );
    if (error < 0) {
        error = -1;
    } else {
        error = 0;
    }

    free:
    if (packet) av_packet_free(&packet);
    if (mediaplayer->fmt_ctx) {
        avformat_free_context(mediaplayer->fmt_ctx);
    }
    if (mediaplayer) SDL_free(mediaplayer);
    free_stream(&mediaplayer->video);
    free_stream(&mediaplayer->audio);
    swr_close(swr_ctx);
    swr_free(&swr_ctx);

    exit:
    return error;
}