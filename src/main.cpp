#include <chrono>
#include <cstdio>
#include <stdexcept>
#include "audio.h"
#include "nes/nes.h"
#include "renderer.h"

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

NES nes;
Renderer renderer(nes);
Audio audio(nes);
double last_time = 0.0f;
double accumulator = 0.0f;

void loop() {
  double time = renderer.time();
  double delta = time - last_time;
  last_time = time;
  constexpr double target_frame_time = 1.0 / 60.0;
  // Set delta to exactly 1 / 60 if within 3%
  if (std::abs(delta - target_frame_time) / target_frame_time <= 0.03) {
    delta = target_frame_time;
  }
  accumulator = std::min(accumulator + delta, target_frame_time * 3);
  while (accumulator >= target_frame_time) {
    nes.run_frame();
    audio.output();
    accumulator -= target_frame_time;
  }
  renderer.render();
}

int main(int argc, char* agv[]) {
  if (!renderer.init()) {
    return -1;
  }
  if (!audio.init()) {
    return -1;
  }

  nes.load("assets/nestest.nes");

#ifdef __EMSCRIPTEN__
  emscripten_set_main_loop(&loop, 0, 1);
#else
  while (!renderer.done()) {
    loop();
  }
#endif

  renderer.destroy();
  audio.destroy();
  return 0;
}