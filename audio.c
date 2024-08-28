#include "audio.h"

/* void audio_callback(void* userdata, Uint8* stream, int len) {
    AVCodecContext* codecContext = (AVCodecContext*)userdata;
    int len1, audio_size;

    static uint8_t audio_buf[(AVCODEC_MAX_AUDIO_FRAME_SIZE)*3 / 2];
    static unsigned int audio_buf_size = 0;
    static unsigned int audio_buf_index = 0;

    while(len > 0) {
        if (audio_buf_index >= audio_buf_size) {
            
        }
    }   
} */