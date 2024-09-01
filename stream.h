#ifndef STREAM_H
#define STREAM_H

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>

typedef struct Stream {
    AVCodecContext* ctx;
    /* AVFrame* frame; */
    AVCodec* codec;
    AVCodecParameters* params;
    int index;
    int isInit;
} Stream;

int init_stream(Stream* stream);
void free_stream(Stream* stream);
int find_streams(
    AVFormatContext* src ,
    Stream* video,
    Stream* audio
);

#endif