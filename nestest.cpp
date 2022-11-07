#include <cstdio>
#include <fstream>
#include <iostream>
#include <string>
#include "nes.h"

int main(int argc, char* agv[]) {
  NES nes;
  nes.cartridge.load("roms/nestest.nes");

  CPU& cpu = nes.cpu;
  cpu.PC = 0xC000;

  // Load nestest.log
  std::ifstream log_file("roms/nestest.log");
  std::string log_line;

  printf("Running nestest.nes\n\n");

  while (!cpu.done) {
    std::getline(log_file, log_line);
    uint16_t PC = strtol(&log_line[0], nullptr, 16);
    uint8_t A = strtol(&log_line[log_line.find("A:") + 2], nullptr, 16);
    uint8_t X = strtol(&log_line[log_line.find("X:") + 2], nullptr, 16);
    uint8_t Y = strtol(&log_line[log_line.find("Y:") + 2], nullptr, 16);
    uint8_t P = strtol(&log_line[log_line.find("P:") + 2], nullptr, 16);
    uint8_t SP = strtol(&log_line[log_line.find("SP:") + 3], nullptr, 16);
    int cycles = strtol(&log_line[log_line.find("CYC:") + 4], nullptr, 10);

    printf("%s\n", log_line.c_str());

    if (cpu.PC != PC || cpu.A != A || cpu.X != X || cpu.Y != Y || cpu.P != P ||
        cpu.SP != SP || cpu.cycles != cycles) {
      printf("ERROR: Mismatch found:\n");
      cpu.print_state();
      break;
    }
    cpu.execute();
  }
  printf("\n");
  printf("0x00: %02x\n", cpu.mem_read(0x00));
  printf("0x01: %02x\n", cpu.mem_read(0x01));
  return 0;
}