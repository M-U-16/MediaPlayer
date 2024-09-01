#include "stream.h"

int find_streams(
    AVFormatContext* src,
    Stream* video,
    Stream* audio
) {

    AVCodecParameters* localCodecParams = NULL;
    for (int i=0; i < src->nb_streams; i++) {
        localCodecParams = src->streams[i]->codecpar;

        const AVCodec* localCodec = avcodec_find_decoder(localCodecParams->codec_id);
        if (!localCodec) {
            continue;
        }

        if (localCodecParams->codec_type == AVMEDIA_TYPE_VIDEO) {
            if (video->index == -1) {
                video->index = i;
                video->codec = (AVCodec*)localCodec;
                video->params = localCodecParams;
            }
        } else if (localCodecParams->codec_type == AVMEDIA_TYPE_AUDIO) {
            if (audio->index == -1) {
                audio->index = i;
                audio->codec = (AVCodec*)localCodec;
                audio->params = localCodecParams;
            }
        }
    }

    return 0;
}

int init_stream(Stream* stream) {
    stream->isInit = 0;
    if (stream->index == -1) {
        printf("init_stream: stream->index=-1\n");
        goto exit_error;
    }

    if (!stream->params) {
        printf("init_steam: no stream params supplied\n");
        goto exit_error;
    }

    /* stream->frame = av_frame_alloc();
    if (!stream->frame) {
        printf("init_stream: %s\n", av_err2str(AVERROR(ENOMEM)));
        goto exit_error;
    } */
    
    stream->ctx = avcodec_alloc_context3(stream->codec);
    if (!stream->ctx) {
        printf("init_stream: error allocating AVCodecContext\n");
        goto exit_error;
    }

    if (avcodec_parameters_to_context(stream->ctx, stream->params) < 0) {
        printf(
            "init_stream: error: copying AVCodecParameters"
            "to AVCodecContext\n"
        );
        goto exit_error;
    }

    if (avcodec_open2(stream->ctx, stream->codec, NULL) < 0) {
        printf(
            "init_stream: error initializing"
            "AVCodecContext with AVCodec\n"
        );
        goto exit_error;
    }

    stream->isInit = 1;
    return 0;

    exit_error:
    free_stream(stream);
    return -1;
}

void free_stream(Stream* stream) {
    if (stream->ctx) {
        avcodec_free_context(&stream->ctx);
    }
    if (stream->params) {
        avcodec_parameters_free(&stream->params);
    }
    /* if (stream->frame) {
        av_frame_free(&stream->frame);
    } */
}