#include "decoder.h"

int packet_decoder(void* data) {
    Player* mediaplayer = (Player*)data;
    AVPacket* packet = av_packet_alloc();
    if (!packet) {
        printf("packet_decoder: av_packet_alloc ENOMEM\n");
        return AVERROR(ENOMEM);
    }

    while (!mediaplayer->quit) {
        if (mediaplayer->videoq_frms.length >= mediaplayer->videoq_frms.max_length) {
            SDL_Delay(10);
            continue;
        }

        int error = av_read_frame(mediaplayer->fmt_ctx, packet);
        if (error == AVERROR_EOF || error == AVERROR(EAGAIN)) {
            printf("packet_decoder: av_read_frame %s\n", av_err2str(error));
            mediaplayer->quit = 1;
            continue;
        }

        if (packet->stream_index == mediaplayer->video.index) {
            decode_video(
                mediaplayer->video.ctx, packet,
                &mediaplayer->videoq_frms
            );
        } else if (packet->stream_index == mediaplayer->audio.index) {
            /* decode_audio(
                mediaplayer->swr_ctx,
                mediaplayer->audio.ctx,
                packet, &mediaplayer->audioq_frms
            ); */
        }
        //av_packet_free(&packet);
        av_packet_unref(packet);
    }

    return 0;
}