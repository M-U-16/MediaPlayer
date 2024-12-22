#ifndef AUDIO_H
#define AUDIO_H

#include <SDL2/SDL_stdinc.h>
#include <SDL2/SDL_audio.h>
#include <libavutil/avutil.h>
#include <libavcodec/avcodec.h>
#include <libswresample/swresample.h>

#include "defs.h"
#include "queue.h"
#include "stream.h"
#include "player.h"

int audio_thread(void* data);
void audio_callback(void *opaque, Uint8* stream, int len);

/**
* Decode audio packet and if needed 
* resample decoded audio.
* @param swr_ctx    SwrContext used for resampling audio
* @param ctx        AVCodecContext for decoding audio
* @param packet     AVPacket containing encoded data
* @param dev        SDL_AudioDeviceID of audio device
* @param audioq     Queue where the decoded frame is put
*/
int audio_decode(
    struct SwrContext* swr_ctx,
    AVCodecContext* ctx,
    AVPacket* packet,
    Queue* audioq
);

/**
* Resample audio in inFrame to outFrame with SwrContext
* @param in_frame    Input AVFrame
* @param out_frame   Output AVFrame
* @param swr_ctx    SwrContext used to resample Audio
* @return 0 on success, or negative from ffmpeg 
*/
int audio_resample(
    struct SwrContext* swr_ctx,
    AVFrame* in_frame,
    AVFrame* out_frame
);

/**
* Open Audio Device and set spec to output from SDL_OpenAudioDevice.
* @param audio      Stream containing Audio information
* @param[out] spec  Output SDL_AudioSpec
* @param[out] dev   Output SDL_AudioDeviceID
* @return -1 on error, 0 on success
* @todo dont hardcode input SDL_AudioSpec */
int audio_open_dev(
    Stream* audio,
    SDL_AudioSpec* spec,
    SDL_AudioDeviceID* dev,
    void* userdata,
    void (*func)(void*, Uint8*, int)
);

/**
* Closes the given SDL_AudioDevice
* @param dev    The SDL_AudioDeviceID */
void audio_close_dev(SDL_AudioDeviceID dev);

#endif // AUDIO_H