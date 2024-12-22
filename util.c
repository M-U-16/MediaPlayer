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

    while (!mediaplayer->quit) {

        if (mediaplayer->pktq.length >= mediaplayer->pktq.max_length) {
            SDL_Delay(10);
            continue;
        }

        AVPacket* packet = av_packet_alloc();
        if (!packet) {
            printf("packet_reader: av_packet_alloc ENOMEM\n");
            continue;
        }

        int error = av_read_frame(mediaplayer->fmt_ctx, packet);
        if (error == AVERROR_EOF || error == AVERROR(EAGAIN)) {
            printf("packet_reader: av_read_frame %s\n", av_err2str(error));
            mediaplayer->quit = 1;
            break;
        }

        if (packet->stream_index == mediaplayer->video.index ||
        packet->stream_index == mediaplayer->audio.index) {
            queue_put(&mediaplayer->pktq, TO_VPP(&packet));
        } else {
            av_packet_free(&packet);
        }
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
    
    /* if (DEBUG) {
        av_dump_format(ctx, 0, path, 0);
    } */

    return 0;
}