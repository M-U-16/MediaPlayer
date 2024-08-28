#include "convert.h"

int init_convert_to_frame(
    ConvertToFrame* frame,
    int width, int height,
    enum AVPixelFormat fmt
) {
    frame->src = av_frame_alloc();
    if (!frame->src) {
        printf("init_convert_to_frame: error allocating frame 'src'\n");
        return -1;
    }
    
    frame->dest = av_frame_alloc();
    if (!frame->dest) {
        printf("init_convert_to_frame: error allocating frame 'dest'\n");
        free_convert_to_frame(frame);
        return -1;
    }

    frame->dest->width = width;
    frame->dest->height = height;
    frame->dest->format = fmt;

    if (av_frame_get_buffer(frame->dest, 0) < 0) {
        printf("init_convert_to_frame: error getting frame buffer\n");
        free_convert_to_frame(frame);
        return -1;
    }

    return 0;
}

void free_convert_to_frame(ConvertToFrame* frame) {
    av_freep(&frame->buffer);
    av_frame_free(&frame->src);
    av_frame_free(&frame->dest);
}