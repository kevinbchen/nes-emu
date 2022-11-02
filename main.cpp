#include <cstdio>
#include "cartridge.h"
#include "cpu.h"

int main(int argc, char* agv[]) {
  Cartridge cartridge;
  cartridge.load("nestest.nes");
  CPU cpu(cartridge);
  cpu.PC = 0xC000;

  while (!cpu.done) {
    cpu.execute();
  }

  printf("0x00: %02x\n", cpu.mem_read(0x00));
  printf("0x01: %02x\n", cpu.mem_read(0x01));
  return 0;
}