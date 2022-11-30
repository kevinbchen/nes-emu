
# nes-emu

**nes-emu** is a basic NES emulator written in C++ for fun / learning. Features:

- Support for Mappers 0 to 4. Games that I've tested include Donkey Kong, Super Mario Bros 1 & 3, Mega Man 1 & 2, Metroid, The Legend of Zelda.
- Visualizations of the PPU nametables, PPU pattern tables, and APU audio waveforms (using [Dear ImGui](https://github.com/ocornut/imgui) for the UI).
- APU implementation with dynamic rate control for sampling to keep audio in sync with the v-sync'd graphics.
- Buildable for the web via emscripten.

See the [Accuracy](#accuracy) and [TODO](#todo) sections for details about limitations.

![nes-emu](https://user-images.githubusercontent.com/2881968/204781437-3d91b014-ea14-4aaa-a1a9-3b599d956190.png)

## Building

The project builds using cmake:
```
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ../ 
make
```
The GLFW and SDL2 dependencies will be automatically downloaded via FetchContent and don't require separate setup. 

To build for web using emscripten:
```
mkdir web_build && cd web_build
emcmake cmake -DCMAKE_BUILD_TYPE=Release ../
emmake make
```
The project *should* be cross-platform for linux and mac too, though I've personally only tested building for Windows and the web.

## Usage

To load a rom, simply drag-and-drop the file into the window.

The controls can be remapped, but the defaults are:

| NES         | Keyboard    | Gamepad     |
| ----------- | ----------- | ----------- |
| DPAD        | Arrow keys  | DPAD        |
| A           | Z           | A           |
| B           | X           | B           |
| Start       | Enter       | Start       |
| Select      | Space       | Back        |

## Accuracy

**nes-emu** is definitely not 100% accurate, though I did try to emulate certain details to a reasonable level.

The CPU implementation has a degree of sub-instruction level simulation, and can step the PPU and APU (via `ppu.tick()` and `apu.tick()`) 
at points within a single CPU instruction (though I don't know if this matters in practice for any games).
I have verified that the CPU cycle counts are correct for the nestest rom tests.

The PPU implementation is per-pixel, so the pixel outputs should happen at (roughly) the correct cycle timings.

Some known inaccuracies:
- [CPU] Only official instructions supported
- [CPU] IRQ timing is off
- [PPU] Memory reads for bg and sprites are not perfectly cycle accurate (and not all dummy reads are implemented)
- [PPU] Sprite evaluation pipeline happens all at once at specific stages each scanline
- [PPU] Sprite overflow bug isn't emulated
- [PPU] Mapper 4 relies on a hacky end-of-scanline signal for the interrupt, instead of the actual PPU A12 line.
- [APU] DMC doesn't emulate CPU stall

## TODO
- More mappers
- Unofficial CPU instructions
- Fix accuracy issues to pass all Blargg's NES tests
- Save state
- Additional debug visualizations (e.g. hex view of CPU and PPU memory)
- TAS videos 

## References

There's a lot of great info on NES emulation out there, but some sources I found particularly useful:

- [NESdev Wiki](https://www.nesdev.org/wiki/Nesdev_Wiki) - An invaluble resource for all the nitty-gritty details
- [An Overview of NES Rendering](https://austinmorlan.com/posts/nes_rendering_overview/) - Great high-level overview for understanding what the PPU does
- [Dynamic Rate Control for Retro Game Emulators](https://docs.libretro.com/guides/ratecontrol.pdf) - Article describing how to adjust audio sampling rate
- [LaiNES](https://github.com/AndreaOrru/LaiNES), [ANESE](https://github.com/daniel5151/ANESE) - Other NES emulators that I referenced
