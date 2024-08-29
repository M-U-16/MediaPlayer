#ifndef AUDIO_H
#define AUDIO_H

#include <SDL2/SDL_stdinc.h>
#include <libavcodec/avcodec.h>

#include "defs.h"
#include "queue.h"

void audio_callback(void* userdata, Uint8* stream, int len);

#endif // AUDIO_H