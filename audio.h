#ifndef AUDIO_H
#define AUDIO_H

#include <SDL2/SDL_stdinc.h>
#include <libavutil/avutil.h>
#include <libavcodec/avcodec.h>
#include <libswresample/swresample.h>

#include "defs.h"
#include "queue.h"
#include "player.h"

/**
* Decode audio packet and if needed 
* resample decoded audio.
* @param swr_ctx    SwrContext used for resampling audio
* @param ctx        AVCodecContext for decoding audio
* @param packet     AVPacket containing encoded data
* @param audioq     Queue where the decoded frame is put
*/
int decode_audio(
    struct SwrContext* swr_ctx,
    AVCodecContext* ctx,
    AVPacket* packet,
    Queue* audioq
);

/* int audio_thread(void* data); */

/**
* Resample audio in inFrame to outFrame with SwrContext
* @param in_frame    Input AVFrame
* @param out_frame   Output AVFrame
* @param swr_ctx    SwrContext used to resample Audio
* @return 0 on success, or negative from ffmpeg 
*/
int resample_audio(
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
int open_audio_dev(Stream* audio, SDL_AudioSpec* spec, SDL_AudioDeviceID* dev);

/**
* Closes the given SDL_AudioDevice
* @param dev    The SDL_AudioDeviceID */
void close_audio_dev(SDL_AudioDeviceID dev);

#endif // AUDIO_H