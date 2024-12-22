#include "decoder.h"

int packet_decoder(void* data) {
    int ret;
    AVPacket* packet;
    Player* mediaplayer = (Player*)data;

    while (!mediaplayer->quit) {
        if (mediaplayer->audioq_frms.length >= mediaplayer->audioq_frms.max_length &&
        mediaplayer->audioq_frms.max_length != 0 ||
        mediaplayer->videoq_frms.length >= mediaplayer->videoq_frms.max_length &&
        mediaplayer->videoq_frms.max_length != 0) {
            SDL_Delay(10);
            continue;
        }

        ret = queue_get(
            &mediaplayer->pktq,
            TO_VPP(&packet), 1
        );
        if (ret != 0) {
            continue;
        }

        if (packet->stream_index == mediaplayer->video.index) {
            decode_video(
                mediaplayer->video.ctx,
                packet, &mediaplayer->videoq_frms
            );

        } else if (packet->stream_index == mediaplayer->audio.index) {
            audio_decode(
                mediaplayer->swr_ctx,
                mediaplayer->audio.ctx,
                packet, &mediaplayer->audioq_frms
            );
        }
        av_packet_free(&packet);
    }

    return 0;
}