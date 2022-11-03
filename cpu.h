#pragma once

#include <cstdint>
#include "cartridge.h"

// 2A03 CPU
class CPU {
 public:
  Cartridge& cartridge;

  uint8_t A;
  uint8_t X;
  uint8_t Y;
  uint8_t P;    // status
                // 7 6 5 4 3 2 1 0
                // N V _ B D I Z C
  uint16_t PC;  // program counter
  uint8_t SP;   // stack pointer

  uint8_t RAM[0x0800];
  uint8_t ROM[0x8000];

  int cycles = 7;
  bool done = false;

  CPU(Cartridge& cartridge);
  void execute();
  void print_state();

  uint8_t* mem(uint16_t addr);
  uint8_t mem_read(uint16_t addr);
  uint16_t mem_read16(uint16_t addr);
  void mem_write(uint16_t addr, uint8_t value);
  void stack_push(uint8_t value);
  uint8_t stack_pop();

 private:
  enum class Flag : int { C = 0, Z, I, D, B, _, V, N };
  uint8_t status(Flag f);
  void status(Flag f, bool value);
  void set_cv(uint8_t a, uint8_t b, uint16_t res);
  void set_zn(uint8_t a);
  void set_z(uint8_t a);
  void set_n(uint8_t a);

  void tick();

  // addressing
  uint16_t oops_cycle(uint16_t addr, int index);
  uint16_t acc();
  uint16_t rel();
  uint16_t imp();
  uint16_t imm();
  uint16_t zp();
  uint16_t zp_x();
  uint16_t zp_y();
  uint16_t abs();
  uint16_t abs_x();
  uint16_t abs_y();
  uint16_t abs_x_store();
  uint16_t abs_y_store();
  uint16_t ind();
  uint16_t ind_x();
  uint16_t ind_y();
  uint16_t ind_y_store();

  // instructions
  void branch(uint16_t addr, bool cond);
  void flag(Flag f, bool value);
  void cmp(uint16_t addr, uint8_t reg);

  void UNIMPL(uint16_t addr);
  void ADC(uint16_t addr);
  void AND(uint16_t addr);
  void ASL(uint16_t addr);
  void BCC(uint16_t addr);
  void BCS(uint16_t addr);
  void BEQ(uint16_t addr);
  void BIT(uint16_t addr);
  void BMI(uint16_t addr);
  void BNE(uint16_t addr);
  void BPL(uint16_t addr);
  void BRK(uint16_t addr);
  void BVC(uint16_t addr);
  void BVS(uint16_t addr);
  void CLC(uint16_t addr);
  void CLD(uint16_t addr);
  void CLI(uint16_t addr);
  void CLV(uint16_t addr);
  void CMP(uint16_t addr);
  void CPX(uint16_t addr);
  void CPY(uint16_t addr);
  void DEC(uint16_t addr);
  void DEX(uint16_t addr);
  void DEY(uint16_t addr);
  void EOR(uint16_t addr);
  void INC(uint16_t addr);
  void INX(uint16_t addr);
  void INY(uint16_t addr);
  void JMP(uint16_t addr);
  void JSR(uint16_t addr);
  void LDA(uint16_t addr);
  void LDX(uint16_t addr);
  void LDY(uint16_t addr);
  void LSR(uint16_t addr);
  void NOP(uint16_t addr);
  void ORA(uint16_t addr);
  void PHA(uint16_t addr);
  void PHP(uint16_t addr);
  void PLA(uint16_t addr);
  void PLP(uint16_t addr);
  void ROL(uint16_t addr);
  void ROR(uint16_t addr);
  void RTI(uint16_t addr);
  void RTS(uint16_t addr);
  void SBC(uint16_t addr);
  void SEC(uint16_t addr);
  void SED(uint16_t addr);
  void SEI(uint16_t addr);
  void STA(uint16_t addr);
  void STX(uint16_t addr);
  void STY(uint16_t addr);
  void TAX(uint16_t addr);
  void TAY(uint16_t addr);
  void TSX(uint16_t addr);
  void TXA(uint16_t addr);
  void TXS(uint16_t addr);
  void TYA(uint16_t addr);

  void ASL_A(uint16_t addr);
  void LSR_A(uint16_t addr);
  void ROL_A(uint16_t addr);
  void ROR_A(uint16_t addr);
};
