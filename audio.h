#pragma once
#include <AL/al.h>
#include <AL/alc.h>
#include "nes.h"

class Audio {
 public:
  Audio(NES& nes) : nes(nes) {}

  bool init();
  void output();

 private:
  NES& nes;

  ALCdevice* device;
  ALCcontext* context;
  ALuint source;
  ALuint buffers[4];

  int buffer_index = 0;
};