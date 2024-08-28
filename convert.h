#ifndef CONVERT_H
#define CONVERT_H

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>

typedef struct ConvertToFrame {
    AVFrame* dest;
    AVFrame* src;
    uint8_t* buffer;
} ConvertToFrame;

int init_convert_to_frame(
    ConvertToFrame* frame,
    int width, int height,
    enum AVPixelFormat fmt
);
void free_convert_to_frame(ConvertToFrame* frame);

#endif