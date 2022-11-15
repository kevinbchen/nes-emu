#include "audio.h"
#include <cmath>
#include <cstdio>
#include <stdexcept>

bool Audio::init() {
  device = alcOpenDevice(nullptr);
  if (!device) {
    fprintf(stderr, "Could not open audio device\n");
    return false;
  }
  context = alcCreateContext(device, nullptr);
  if (!alcMakeContextCurrent(context)) {
    fprintf(stderr, "Could not create audio context: %d\n",
            alcGetError(device));
    return false;
  }

  alGenSources((ALuint)1, &source);
  alSourcef(source, AL_PITCH, 1);
  alSourcef(source, AL_GAIN, 1);
  alSource3f(source, AL_POSITION, 0, 0, 0);
  alSource3f(source, AL_VELOCITY, 0, 0, 0);
  alSourcei(source, AL_LOOPING, AL_FALSE);

  alGenBuffers((ALuint)4, buffers);

  // Queue up buffers
  uint16_t data[735];
  for (int j = 0; j < 4; j++) {
    for (int i = 0; i < 735; ++i) {
      data[i] = sin(2 * M_PI * 440 * i * (j + 1) / 44100) * SHRT_MAX;
    }
    alBufferData(buffers[j], AL_FORMAT_MONO16, data, sizeof(data), 44100);
    alSourceQueueBuffers(source, 1, &buffers[j]);
  }
  alSourcePlay(source);

  return true;
}

void Audio::output() {
  // TODO: Need to properly deal with video/audio clock syncing and buffering

  ALint buffers_processed = 0;
  alGetSourcei(source, AL_BUFFERS_PROCESSED, &buffers_processed);
  if (buffers_processed > 0) {
    ALuint buffer;
    alSourceUnqueueBuffers(source, 1, &buffer);

    alBufferData(buffer, AL_FORMAT_MONO16, nes.apu.output_buffer,
                 nes.apu.sample_count * sizeof(int16_t), 44100);

    alSourceQueueBuffers(source, 1, &buffer);

    ALint source_state;
    alGetSourcei(source, AL_SOURCE_STATE, &source_state);
    if (source_state != AL_PLAYING) {
      alSourcePlay(source);
    }
  }
  nes.apu.output();
}