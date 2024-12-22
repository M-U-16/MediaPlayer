#include "audio.h"

void audio_callback(void* opaque, Uint8* stream, int len) {
    static int first_run = 1;
    Player* mediaplayer = (Player*)opaque;
    
    //SDL_MixAudioFormat();
    //static uint8_t audio_buf[(AVCODEC_MAX_AUDIO_FRAME_SIZE*3)/2];

    AVFrame* frame;
    queue_get(&mediaplayer->audioq_frms, TO_VPP(&frame), 0);
    if (first_run) {
        //printf("audio_callback: frame->sample_rate=%d\n", frame->sample_rate);
        first_run = 0;
    }
    
    av_frame_free(&frame);
    memset(stream, 0, len);
    
    //printf("audio_callback: len=%d\n", len);
}

int audio_thread(void* data) {
    int ret = 0;
    AVFrame* frame;
    Player* mediaplayer = (Player*)data;

    while (!mediaplayer->quit) {
        if (!mediaplayer->pause) {
            ret = queue_get(
                &mediaplayer->audioq_frms,
                TO_VPP(&frame), 1
            );
            if (ret != 0) {
                continue;
            }
            
            ret = SDL_QueueAudio(
                mediaplayer->audioDev,
                frame->data[0],
                frame->linesize[0]
            );
            if (ret < 0) {
                printf(
                    "audio_thread: SDL_QueueAudio %s\n",SDL_GetError());
            }
            av_frame_free(&frame);
        }
    }

    return 0;
}

int audio_open_dev(
    Stream* audio,
    SDL_AudioSpec* spec,
    SDL_AudioDeviceID* dev,
    void* userdata,
    void (*func)(void*, Uint8*, int)
) {
    SDL_AudioSpec wanted_audioSpec;
    SDL_zero(wanted_audioSpec);

    wanted_audioSpec.freq = audio->ctx->sample_rate;
    wanted_audioSpec.channels = audio->codec->ch_layouts->nb_channels;
    wanted_audioSpec.format = AUDIO_S32SYS;
    wanted_audioSpec.samples = PLAYER_AUDIO_BUFFER_SIZE;
    if (PLAYER_USE_AUDIO_CALLBACK) {
        wanted_audioSpec.callback = func;
        wanted_audioSpec.userdata = userdata;
    }
    //av_log2()
    
    *dev = SDL_OpenAudioDevice(
        NULL, SDL_FALSE,
        &wanted_audioSpec,
        spec, SDL_FALSE
    );
    if (!*dev) {
        printf("player: SDL_OpenAudioDevice, %s\n", SDL_GetError());
        return -1;
    }

    return 0;
}

void audio_close_dev(SDL_AudioDeviceID dev) {
    SDL_PauseAudioDevice(dev, SDL_TRUE);
    SDL_ClearQueuedAudio(dev);
    SDL_CloseAudioDevice(dev);
}

int audio_resample(
    struct SwrContext* swr_ctx,
    AVFrame* inFrame,
    AVFrame* outFrame
) {
    int ret;
    int out_linesize;
    int dst_nb_samples;
    uint8_t *out_buffer = NULL;

    /*
    Following code is based on this answer from Stackoverflow:
    https://stackoverflow.com/a/69334911
    */
    ret = av_rescale_rnd(
        swr_get_delay(swr_ctx, inFrame->sample_rate)
        + inFrame->nb_samples,
        inFrame->sample_rate,
        inFrame->sample_rate,
        AV_ROUND_UP
    );
    dst_nb_samples = inFrame->ch_layout.nb_channels * ret;

    ret = av_samples_alloc(
        &out_buffer, NULL, 1,
        dst_nb_samples,
        AV_SAMPLE_FMT_S16, 1
    );
    if (ret < 0) {
        printf(
            "decode_audio: av_samples_alloc %s (code=%d)\n",
            av_err2str(ret), ret
        );
        return ret;
    }

    ret = swr_convert(
        swr_ctx, &out_buffer,
        dst_nb_samples,
        (const uint8_t**)inFrame->data,
        inFrame->nb_samples
    );
    if (ret < 0) {
        printf(
            "decode_audio: swr_convert %s (code=%d)\n",
            av_err2str(ret), ret
        );
        return ret;
    }

    dst_nb_samples = inFrame->ch_layout.nb_channels * ret;

    ret = av_samples_fill_arrays(
        outFrame->data,
        outFrame->linesize,
        out_buffer,
        1, dst_nb_samples,
        AV_SAMPLE_FMT_S16, 1
    );
    if (ret < 0) {
        printf(
            "decode_audio: av_samples_fill_arrays %s (code=%d)\n",
            av_err2str(ret), ret
        );
        av_freep(&out_buffer);
        return ret;
    }

    av_freep(&out_buffer);
    return 0;
}

int audio_decode(
    struct SwrContext* swr_ctx,
    AVCodecContext* ctx,
    AVPacket* packet,
    Queue* audioq
) {
    AVFrame* frame = av_frame_alloc();
    if (!frame) {
        return AVERROR(ENOMEM);
    }

    int ret = avcodec_send_packet(ctx, packet);
    if (ret < 0) {
        return ret;
    }

    while (ret >= 0) {
        ret = avcodec_receive_frame(ctx, frame);
        if (ret < 0) {
            break;
        }

        AVFrame* audioFrame = av_frame_alloc();
        if (!audioFrame) {
            ret = AVERROR(ENOMEM);
            break;
        }

        audio_resample(swr_ctx, frame, audioFrame);
        queue_put(audioq, TO_VPP(&audioFrame));
    }
    if (ret >= 0) {
        ret = 0;
    }

    av_frame_free(&frame);
    return ret;
}