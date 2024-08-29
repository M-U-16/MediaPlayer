#include "player.h"

Player* create_player() {
    Player* mediaplayer = av_mallocz(sizeof(Player));
    if (!mediaplayer) {
        return NULL;
    }

    mediaplayer->quit = 0;
    mediaplayer->pause = 0;
    mediaplayer->video.index = -1;
    mediaplayer->audio.index = -1;
    mediaplayer->pause_audio_thread = 0;
    mediaplayer->pause_audio_thread = 0;
    mediaplayer->quit_audio_thread = 0;
    mediaplayer->quit_video_thread = 0;

    mediaplayer->fmt_ctx = avformat_alloc_context();
    if (!mediaplayer->fmt_ctx) {
        av_free(mediaplayer);
        return NULL;
    }

    return mediaplayer;
}