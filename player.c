#include "player.h"

extern const Uint32 MEDIAPLAYER_SDL_FLAGS;

Player* player_create(
    char* filename,
    enum player_disable_media disabled
) {
    Player* mediaplayer = SDL_malloc(sizeof(Player));
    if (!mediaplayer) {
        return NULL;
    }
    SDL_memset(mediaplayer, 0, sizeof(Player));

    mediaplayer->quit = 0;
    mediaplayer->pause = 0;
    mediaplayer->sws_ctx = NULL;
    mediaplayer->swr_ctx = NULL;
    mediaplayer->video.index = -1;
    mediaplayer->audio.index = -1;
    mediaplayer->refresh_video = 0;
    mediaplayer->disabled_media = disabled;
    mediaplayer->filename = av_strdup(filename);
    mediaplayer->fmt_ctx = avformat_alloc_context();
    if (!mediaplayer->fmt_ctx) {
        SDL_free(mediaplayer);
        return NULL;
    }

    return mediaplayer;
}

int player_init(Player* mediaplayer) {
    int error;
    if (mediaplayer->audio.index == -1 && mediaplayer->video.index == -1) {
        if (mediaplayer->disabled_media == PLAYER_DISABLE_AUDIO) {
            printf(
                "player_init: Audio for Player is disabled "
                "and no video stream could be found.\n"
            );
            return -1;
        } else if (mediaplayer->disabled_media == PLAYER_DISABLE_VIDEO) {
            printf(
                "player_init: Video for Player is disabled "
                "and no audio stream could be found.\n"
            );
            return -1;
        }
        
        printf(
            "player_init: No video stream "
            "and no audio stream could be found.\n"
        );
        return -1;
    }

    error = queue_init(
        &mediaplayer->pktq,
        QUEUE_PACKET, &mediaplayer->quit,
        "pkts", PLAYER_PKTQ_MAX_LEN
    );
    if (error < 0) {
        printf("player_init: queue_init, error initializing queue\n");
        return -1;
    }

    if (mediaplayer->disabled_media != PLAYER_DISABLE_AUDIO && mediaplayer->audio.index != -1) {
        error = queue_init(
            &mediaplayer->audioq_frms,
            QUEUE_FRAME, &mediaplayer->quit,
            "audio", PLAYER_AUDIOQ_FRMS_MAX_LEN
        );
        if (error < 0) {
            printf("player_init: queue_init, error initializing queue\n");
            return -1;
        }
    } else {
        mediaplayer->audioq_frms.length = 0;
        mediaplayer->audioq_frms.max_length = 0;
    }

    if (mediaplayer->disabled_media != PLAYER_DISABLE_VIDEO && mediaplayer->video.index != -1) {
        error = queue_init(
            &mediaplayer->videoq_frms,
            QUEUE_FRAME, &mediaplayer->quit,
            "video", PLAYER_VIDEOQ_FRMS_MAX_LEN
        );
        if (error < 0) {
            printf("player_init: queue_init, error initializing queue\n");
            return -1;
        }
    } else {
        mediaplayer->videoq_frms.length = 0;
        mediaplayer->videoq_frms.max_length = 0;
    }

    return 0;
}

int player_create_display(Player* mediaplayer) {
    mediaplayer->window = SDL_CreateWindow(
        "MediaPlayer",
        SDL_WINDOWPOS_UNDEFINED,
        SDL_WINDOWPOS_UNDEFINED,
        mediaplayer->video.ctx->width,
        mediaplayer->video.ctx->height,
        SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE
    );
    if (!mediaplayer->window) {
        printf(
            "player_create_display: SDL_CreateWindow: %s\n",
            SDL_GetError()
        );
        return -1;
    }

    mediaplayer->renderer = SDL_CreateRenderer(
        mediaplayer->window,
        -1, SDL_RENDERER_ACCELERATED
    );
    if (!mediaplayer->renderer) {
        printf(
            "player_create_display: SDL_CreateRenderer: %s\n",
            SDL_GetError()
        );
        return -1;
    }

    mediaplayer->texture = SDL_CreateTexture(
        mediaplayer->renderer,
        SDL_PIXELFORMAT_IYUV,
        SDL_TEXTUREACCESS_STATIC,
        mediaplayer->video.ctx->width,
        mediaplayer->video.ctx->height
    );
    if (!mediaplayer->texture) {
        printf("player_create_display: SDL_CreateTexture %s\n");
        return -1;
    }

    SDL_SetRenderDrawColor(
        mediaplayer->renderer,
        0, 0, 0,
        SDL_ALPHA_OPAQUE
    );
    SDL_RenderClear(mediaplayer->renderer);
}

int player_start(Player* mediaplayer) {
    int ret = 0;
    int pause = 0;
    int error = 0;
    int current_stage = 0;
    int skip_video_frame = 0;
    
    PlayerThreads threads;
    SDL_memset(&threads, 0, sizeof(threads));

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

    mediaplayer->font = TTF_OpenFont(PLAYER_FONT, 30);
    if (!mediaplayer->font) {
        printf("player: TTF_OpenFont returned error\n");
        ret = -1;
        goto free;
    }
    TTF_SetFontHinting(mediaplayer->font, TTF_HINTING_NORMAL);

    player_create_display(mediaplayer);

    audio_open_dev(
        &mediaplayer->audio,
        &mediaplayer->audioSpec,
        &mediaplayer->audioDev,
        PLAYER_USE_AUDIO_CALLBACK ? (void*)mediaplayer : NULL,
        PLAYER_USE_AUDIO_CALLBACK ? audio_callback : NULL
    );

    /* SDL_Color color = {255, 255, 255};
    SDL_Surface* message = TTF_RenderText_Blended(
        mediaplayer->font,
        "Hello World", color
    );
    SDL_Texture* message_texture = SDL_CreateTextureFromSurface(mediaplayer->renderer, message);
    SDL_Rect srcrect = message->clip_rect;
    SDL_Rect dstrect = message->clip_rect;
    dstrect.x = 0;
    dstrect.y = 0;
    SDL_FreeSurface(message); */

    // create threads for reading and decoding packets
    threads.decoder_th = SDL_CreateThread(packet_decoder, "packet_decoder", (void*)mediaplayer);
    if (!threads.decoder_th) {
        ret = -1;
        goto free;
    }

    threads.reader_th = SDL_CreateThread(packet_reader, "packet_reader", (void*)mediaplayer);
    if (!threads.reader_th) {
        ret = -1;
        goto free;
    }

    threads.video_th = SDL_CreateThread(video_thread, "video_thread", (void*)mediaplayer);
    if (!threads.video_th) {
        ret = -1;
        goto free;
    }

    if (!PLAYER_USE_AUDIO_CALLBACK) {
        threads.audio_th = SDL_CreateThread(audio_thread, "audio_thread", (void*)mediaplayer);
        if (!threads.audio_th) {
            ret = -1;
            goto free;
        }
    }

    /*
    -------------
    SDL EVENTLOOP
    -------------
    */
    SDL_Event event;
    while (!mediaplayer->quit) {
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
        SDL_Delay(20);
    }

    ret = 0;
    free:
    queue_signal(&mediaplayer->pktq);
    queue_signal(&mediaplayer->videoq_frms);
    queue_signal(&mediaplayer->audioq_frms);

    if (threads.video_th) { SDL_WaitThread(threads.video_th, NULL); }
    if (threads.audio_th) { SDL_WaitThread(threads.audio_th, NULL); }
    if (threads.decoder_th) { SDL_WaitThread(threads.decoder_th, NULL); }
    if (threads.reader_th) { SDL_WaitThread(threads.reader_th, NULL); }

    /* SDL_FreeSurface(message);
    SDL_DestroyTexture(message_texture); */
    if (SDL_WasInit(MEDIAPLAYER_SDL_FLAGS)) { SDL_Quit(); }
    if (TTF_WasInit()) { TTF_Quit(); }

    return ret;
}

void player_free(Player* mediaplayer) {
    audio_close_dev(mediaplayer->audioDev);

    queue_free(&mediaplayer->pktq);
    queue_free(&mediaplayer->videoq_frms);
    queue_free(&mediaplayer->audioq_frms);

    if (mediaplayer->fmt_ctx) {
        avformat_free_context(mediaplayer->fmt_ctx);
    }
    if (mediaplayer) { av_free(mediaplayer); }

    free_stream(&mediaplayer->video);
    free_stream(&mediaplayer->audio);
    swr_close(mediaplayer->swr_ctx);
    swr_free(&mediaplayer->swr_ctx);
    sws_freeContext(mediaplayer->sws_ctx);

    SDL_DestroyRenderer(mediaplayer->renderer);
    SDL_DestroyWindow(mediaplayer->window);
    TTF_CloseFont(mediaplayer->font);
}