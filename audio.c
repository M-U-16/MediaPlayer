#include "audio.h"

int audio_thread(void* data) {
    Player* mediaplayer = (Player*)data;
}

int open_audio_dev(
    Stream* audio,
    SDL_AudioSpec* spec,
    SDL_AudioDeviceID* dev
) {
    SDL_AudioSpec wanted_audioSpec;
    SDL_zero(wanted_audioSpec);

    wanted_audioSpec.freq = audio->ctx->sample_rate;
    wanted_audioSpec.channels = audio->codec->ch_layouts->nb_channels;
    wanted_audioSpec.format = AUDIO_S32SYS;
    wanted_audioSpec.silence = 0;
    
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

void close_audio_dev(SDL_AudioDeviceID dev) {
    SDL_PauseAudioDevice(dev, SDL_TRUE);
    SDL_ClearQueuedAudio(dev);
    SDL_CloseAudioDevice(dev);
}

int resample_audio(
    struct SwrContext* swr_ctx,
    AVFrame* inFrame,
    AVFrame* outFrame
) {
    int out_linesize;
    int dst_nb_samples;
    uint8_t *out_buffer = NULL;

    /*
    Following code is based on this answer from Stackoverflow:
    https://stackoverflow.com/a/69334911
    */
    int ret = av_rescale_rnd(
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
            "decode_audio: av_samples_fill_arrays"
            "%s (code=%d)\n",
            av_err2str(ret), ret
        );
        av_freep(&out_buffer);
        return ret;
    }

    av_freep(&out_buffer);
    return 0;
}

int decode_audio(
    struct SwrContext* swr_ctx,
    AVCodecContext* ctx,
    AVPacket* packet,
    Queue* audioq
) {
    int ret = avcodec_send_packet(ctx, packet);
    if (ret < 0) {
        return ret;
    }

    AVFrame* frame = av_frame_alloc();
    if (!frame) {
        return AVERROR(ENOMEM);
    }

    AVFrame* audioFrame = av_frame_alloc();
    if (!audioFrame) {
        av_frame_free(&frame);
        return AVERROR(ENOMEM);
    }

    while (ret >= 0) {
        ret = avcodec_receive_frame(ctx, frame);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
            break;
        }
        if (ret < 0) {
            return ret;
        }

        resample_audio(swr_ctx, frame, audioFrame);
        queue_put(audioq, TO_VPP(&audioFrame));
        
        /* ret = SDL_QueueAudio(dev,
            audioFrame->data[0],
            audioFrame->linesize[0]
        );
        if (ret < 0) {
            printf(
                "decode_audio: SDL_QueueAudio %s (error code=%d)\n",
                ret, SDL_GetError()
            );
        } */
    }
    if (ret >= 0) {
        ret = 0;
    }

    if (frame) {
        av_frame_free(&frame);
    }
    if (audioFrame) {
        av_frame_free(&audioFrame);
    }
    return ret;
}