#include "util.h"

int save_frame(uint8_t* buf, int linesize, int width, int height, const char* name) {
    printf("save_frame...\n");
    FILE* file = fopen(name, "wb");
    if (!file) {
        return -1;
    }

    fprintf(file, "P6\n%d %d\n255\n", width, height);
    for (int y = 0; y < height; y++) {
        fwrite(buf + y * linesize, 1, width*3, file);
    }
    fclose(file);
}

int decode_video(
    struct SwsContext* sws_ctx,
    AVCodecContext* ctx,
    AVPacket* packet,
    AVFrame* frame
) {
    //frame->best_effort_timestamp

    int ret = avcodec_send_packet(ctx, packet);
    if (ret < 0) {
        return ret;
    }

    ret = avcodec_receive_frame(ctx, frame);
    if (ret == AVERROR_EOF) {
        return AVERROR_EOF;
    } else if (ret < 0) {
        return ret;
    }

    /* sws_scale(
        sws_ctx,
        (uint8_t const* const*)srcFrame->data,
        srcFrame->linesize, 0,
        ctx->height,
        destFrame->data,
        destFrame->linesize
    ); */

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

int decode_audio(
    SDL_AudioDeviceID dev,
    AVCodecContext* ctx,
    AVPacket* packet,
    AVFrame* frame,
    struct SwrContext* swr_ctx
) {
    uint8_t *out_buffer = NULL;

    int ret = avcodec_send_packet(ctx, packet);
    if (ret < 0) {
        return ret;
    }

    AVFrame* audioFrame = av_frame_alloc();
    if (!audioFrame) {
        return AVERROR(ENOMEM);
    }

    int out_linesize;
    int dst_nb_samples;
    int src_sample_fmt = ctx->sample_fmt;
    int src_num_channels = ctx->ch_layout.nb_channels;

    while (ret >= 0) {
        ret = avcodec_receive_frame(ctx, frame);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
            break;
        }
        if (ret < 0) {
            return ret;
        }

        /*
        Following code is based on this answer from Stackoverflow:
        https://stackoverflow.com/a/69334911
        */
        ret = av_rescale_rnd(
            swr_get_delay(swr_ctx, frame->sample_rate) + frame->nb_samples,
            frame->sample_rate,
            frame->sample_rate,
            AV_ROUND_UP
        );
        dst_nb_samples = frame->ch_layout.nb_channels * ret;

        ret = av_samples_alloc(
            &out_buffer, NULL, 1,
            dst_nb_samples,
            AV_SAMPLE_FMT_S16, 1
        );
        if (ret < 0) {
            printf(
                "decode_audio: av_samples_alloc %s (code=%d)\n",
                av_err2str(ret),
                ret
            );
            break;
        }

        ret = swr_convert(
            swr_ctx, &out_buffer,
            dst_nb_samples,
            (const uint8_t**)frame->data,
            frame->nb_samples
        );
        if (ret < 0) {
            printf("decode_audio: swr_convert %s (code=%d)\n",
                av_err2str(ret),
                ret
            );
            break;
        }

        dst_nb_samples = frame->ch_layout.nb_channels * ret;

        ret = av_samples_fill_arrays(
            audioFrame->data,
            audioFrame->linesize,
            out_buffer,
            1, dst_nb_samples,
            AV_SAMPLE_FMT_S16, 1
        );

        if (ret < 0) {
            printf(
                "decode_audio: av_samples_fill_arrays %s (code=%d)\n",
                av_err2str(ret), ret
            );
            break;
        }

        ret = SDL_QueueAudio(
            dev,
            audioFrame->data[0],
            audioFrame->linesize[0]
        );
        if (ret < 0) {
            printf(
                "decode_audio: SDL_QueueAudio %s (error code=%d)\n",
                ret, SDL_GetError()
            );
        }
    }

    av_freep(&out_buffer);
    av_frame_free(&audioFrame);
    
    return 0;
}

int context_from_file(char* path, AVFormatContext* ctx) {
    if (avformat_open_input(&ctx, path, NULL, NULL) != 0) {
        return 1;
    }
    
    if (avformat_find_stream_info(ctx, NULL) < 0) {
        return 1;
    }
    
    if (DEBUG) {
        av_dump_format(ctx, 0, path, 0);
    }

    return 0;
}