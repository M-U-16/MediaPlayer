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

int packet_reader(void* data) {
    Player* mediaplayer = (Player*)data;

    while (!mediaplayer->quit_reader && !mediaplayer->quit) {
        AVPacket* packet = av_packet_alloc();
        if (!packet) {
            printf("packet_reader: av_packet_alloc ENOMEM\n");
            continue;
        }

        int error = av_read_frame(mediaplayer->fmt_ctx, packet);
        if (error == AVERROR_EOF || error == AVERROR(EAGAIN)) {
            mediaplayer->quit = 1;
            continue;
        }

        if (packet->stream_index == mediaplayer->video.index) {
            //SDL_CondSignal();
            queue_put(&mediaplayer->videoq_pkts, TO_VPP(&packet));
        }/*  else if (packet->stream_index == mediaplayer->audio.index) {
            //queue_put(&mediaplayer->audioq_pkts, TO_VPP(&packet));
            av_packet_unref(packet);
        } else {
            av_packet_unref(packet);
        } */
    }

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