#pragma once
#include <SDL.h>
#include "nes.h"

class Audio {
 public:
  Audio(NES& nes) : nes(nes) {}

  bool init();
  void destroy();
  void output();

 private:
  NES& nes;

  SDL_AudioDeviceID audio_device;
  int average_queue_size = 0;
};