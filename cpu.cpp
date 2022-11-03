#include "cpu.h"
#include <iostream>
#include <stdexcept>

CPU::CPU(Cartridge& cartridge) : cartridge(cartridge) {
  A = 0;
  X = 0;
  Y = 0;
  P = 0x24;
  SP = 0xFD;
  // PC = (mem_read(0xFFFD) << 8) + mem_read(0xFFFC);

  memset(RAM, 0, 0x0800);
  memset(ROM, 0, 0x8000);
}

void CPU::execute() {
  // Fetch opcode, increment PC
  uint8_t op = mem_read(PC++);

  uint16_t addr;
  switch (op) {
#define X(opcode, op, mode) \
  case opcode:              \
    addr = mode();          \
    op(addr);               \
    break;

#include "instructions.h"

#undef X
  }
}

void CPU::print_state() {
  printf(
      "%04X                                            A:%02X X:%02X Y:%02X "
      "P:%02X SP:%02X PPU:  0, 21 CYC:%d\n",
      PC, A, X, Y, P, SP, cycles);

  /*
  printf("A=0x%02x X=0x%02x Y=0x%02x P=0x%02x SP=0x%02x PC=0x%04x\n", A, X, Y,
         P, SP, PC);*/
}

uint8_t CPU::status(Flag f) {
  return (P >> (int)f) & 0x1;
}

void CPU::status(Flag f, bool value) {
  uint8_t mask = 1 << (int)f;
  P = (P & ~mask) | (value << (int)f);
}

void CPU::set_cv(uint8_t a, uint8_t b, uint16_t res) {
  status(Flag::C, res > 0xFF);
  status(Flag::V, (bool)((a ^ res) & (b ^ res) & 0x80));
}

void CPU::set_zn(uint8_t a) {
  set_z(a);
  set_n(a);
}

void CPU::set_z(uint8_t a) {
  status(Flag::Z, a == 0);
}

void CPU::set_n(uint8_t a) {
  status(Flag::N, (bool)(a & 0x80));
}

uint8_t* CPU::mem(uint16_t addr) {
  if (addr <= 0x1FFF) {
    // 2KB internal RAM, 0x800 bytes mirrored 3 times
    return &RAM[addr & 0x07FF];
  } else if (addr <= 0x3FFF) {
    // PPU registers, 8 bytes mirrored
  } else if (addr <= 0x401F) {
    // APU and I/O registers, 32 bytes
  } else {
    // ROM
    // return &ROM[addr & 0x7FFF];
    return cartridge.mem(addr);
  }
  return nullptr;
}

uint8_t CPU::mem_read(uint16_t addr) {
  tick();
  return *mem(addr);
}

uint16_t CPU::mem_read16(uint16_t addr) {
  return mem_read(addr) | (mem_read(addr + 1) << 8);
}

void CPU::mem_write(uint16_t addr, uint8_t value) {
  tick();
  *mem(addr) = value;
}

void CPU::stack_push(uint8_t value) {
  mem_write(0x100 + SP--, value);
}

uint8_t CPU::stack_pop() {
  return mem_read(0x100 + (++SP));
}

void CPU::tick() {
  cycles++;
}

namespace {
enum AddrMode {
  Imp,
  Acc,
  Imm,
  Zp,
  ZpX,
  ZpY,
  Abs,
  AbsX,
  AbsY,
  Ind,
  IndX,
  IndY,
  Rel,
};

AddrMode get_addr_mode(uint8_t op) {
  // See table https://www.nesdev.org/wiki/CPU_unofficial_opcodes

  uint8_t top = op & 0xE0;     // top 3 bits
  uint8_t bottom = op & 0x1F;  // bottom 5 bits

  switch (bottom) {
    case 0x00:
    case 0x02:
      return top < 0x80 ? AddrMode::Imp : AddrMode::Imm;
    case 0x01:
    case 0x03:
      return AddrMode::IndX;
    case 0x04:
    case 0x05:
    case 0x06:
    case 0x07:
      return AddrMode::Zp;
    case 0x09:
    case 0x0B:
      return AddrMode::Imm;
    case 0x0C:
    case 0x0D:
    case 0x0E:
    case 0x0F:
      return (op == 0x60) ? AddrMode::Ind : AddrMode::Acc;
    case 0x10:
      return AddrMode::Rel;
    case 0x11:
    case 0x13:
      return AddrMode::IndY;
    case 0x14:
    case 0x15:
      return AddrMode::ZpX;
    case 0x16:
    case 0x17:
      return (top == 0x80 || top == 0xA0) ? AddrMode::ZpY : AddrMode::ZpX;
    case 0x19:
    case 0x1B:
      return AddrMode::AbsY;
    case 0x1C:
    case 0x1D:
      return AddrMode::AbsX;
    case 0x1E:
    case 0x1F:
      return (top == 0x80 || top == 0xA0) ? AddrMode::AbsY : AddrMode::AbsX;

    case 0x08:
    case 0x0A:
    case 0x12:
    case 0x18:
    case 0x1A:
      return AddrMode::Imp;

    default:
      throw std::runtime_error("Could not determine addressing mode");
  }
}
}  // namespace

uint16_t CPU::oops_cycle(uint16_t addr, int index) {
  if (((addr + index) & 0xFF00) != (addr & 0xFF00)) {
    tick();
  }
  return addr + index;
}

uint16_t CPU::acc() {
  return 0;
}

uint16_t CPU::rel() {
  // Because a relative offset is interpreted as signed, leave it up to the
  // branch instructions to actually do the memory read for the operand
  return PC++;
}

uint16_t CPU::imp() {
  return 0;
}

uint16_t CPU::imm() {
  return PC++;
}

uint16_t CPU::zp() {
  return mem_read(PC++);
}

uint16_t CPU::zp_x() {
  tick();
  return (zp() + X) & 0xFF;
}

uint16_t CPU::zp_y() {
  tick();
  return (zp() + Y) & 0xFF;
}

uint16_t CPU::abs() {
  uint16_t addr = mem_read16(PC);
  PC += 2;
  return addr;
}

uint16_t CPU::abs_x() {
  return oops_cycle(abs(), X);
}

uint16_t CPU::abs_y() {
  return oops_cycle(abs(), Y);
}

uint16_t CPU::abs_x_store() {
  tick();
  return abs() + X;
}

uint16_t CPU::abs_y_store() {
  tick();
  return abs() + Y;
}

uint16_t CPU::ind() {
  // No crossing page boundary
  uint16_t addr = abs();
  return mem_read(addr) |
         (mem_read((addr & 0xFF00) | ((addr + 1) & 0xFF)) << 8);
}

uint16_t CPU::ind_x() {
  // indexed indirect
  uint8_t arg = (uint8_t)zp_x();
  return mem_read(arg) | (mem_read((arg + 1) & 0xFF) << 8);
}

uint16_t CPU::ind_y() {
  // indirect indexed
  uint8_t arg = (uint8_t)zp();
  uint16_t addr = mem_read(arg) | (mem_read((arg + 1) & 0xFF) << 8);
  return oops_cycle(addr, Y);
}

uint16_t CPU::ind_y_store() {
  // indirect indexed
  uint8_t arg = (uint8_t)zp();
  uint16_t addr = mem_read(arg) | (mem_read((arg + 1) & 0xFF) << 8);
  tick();
  return addr + Y;
}

void CPU::branch(uint16_t addr, bool cond) {
  int8_t offset = mem_read(addr);  // signed offset!
  if (cond) {
    tick();
    oops_cycle(PC, offset);
    PC += offset;
  }
}

void CPU::flag(Flag f, bool value) {
  tick();
  status(f, value);
}

void CPU::cmp(uint16_t addr, uint8_t reg) {
  uint8_t value = mem_read(addr);
  uint8_t res = reg - value;
  status(Flag::C, reg >= value);
  set_zn(res);
}

void CPU::UNIMPL(uint16_t addr) {
  printf("UNIMPL\n");
  done = true;
}

void CPU::ADC(uint16_t addr) {
  uint8_t value = mem_read(addr);
  int16_t sum = A + value + status(Flag::C);
  set_cv(A, value, sum);
  set_zn(sum);
  A = sum;
}

void CPU::AND(uint16_t addr) {
  uint8_t value = mem_read(addr);
  A = A & value;
  set_zn(A);
}

void CPU::ASL(uint16_t addr) {
  uint8_t value = mem_read(addr);
  tick();
  uint8_t res = value << 1;
  status(Flag::C, (bool)(value & 0x80));
  set_zn(res);
  mem_write(addr, res);
}

void CPU::ASL_A(uint16_t addr) {
  tick();
  uint8_t res = A << 1;
  status(Flag::C, (bool)(A & 0x80));
  set_zn(res);
  A = res;
}

void CPU::BCC(uint16_t addr) {
  branch(addr, status(Flag::C) == 0);
}

void CPU::BCS(uint16_t addr) {
  branch(addr, status(Flag::C) == 1);
}

void CPU::BEQ(uint16_t addr) {
  branch(addr, status(Flag::Z) == 1);
}

void CPU::BIT(uint16_t addr) {
  uint8_t value = mem_read(addr);
  uint8_t res = A & value;
  set_z(res);
  set_n(value);
  status(Flag::V, (bool)(value & 0x40));
}

void CPU::BMI(uint16_t addr) {
  branch(addr, status(Flag::N) == 1);
}

void CPU::BNE(uint16_t addr) {
  branch(addr, status(Flag::Z) == 0);
}

void CPU::BPL(uint16_t addr) {
  branch(addr, status(Flag::N) == 0);
}

void CPU::BRK(uint16_t addr) {
  // TODO
  status(Flag::B, true);
  done = true;
}

void CPU::BVC(uint16_t addr) {
  branch(addr, status(Flag::V) == 0);
}

void CPU::BVS(uint16_t addr) {
  branch(addr, status(Flag::V) == 1);
}

void CPU::CLC(uint16_t addr) {
  flag(Flag::C, 0);
}

void CPU::CLD(uint16_t addr) {
  flag(Flag::D, 0);
}

void CPU::CLI(uint16_t addr) {
  flag(Flag::I, 0);
}

void CPU::CLV(uint16_t addr) {
  flag(Flag::V, 0);
}

void CPU::CMP(uint16_t addr) {
  cmp(addr, A);
}

void CPU::CPX(uint16_t addr) {
  cmp(addr, X);
}

void CPU::CPY(uint16_t addr) {
  cmp(addr, Y);
}

void CPU::DEC(uint16_t addr) {
  uint8_t value = mem_read(addr);
  tick();
  uint8_t res = value - 1;
  set_zn(res);
  mem_write(addr, res);
}

void CPU::DEX(uint16_t addr) {
  tick();
  X--;
  set_zn(X);
}

void CPU::DEY(uint16_t addr) {
  tick();
  Y--;
  set_zn(Y);
}

void CPU::EOR(uint16_t addr) {
  uint8_t value = mem_read(addr);
  A = A ^ value;
  set_zn(A);
}

void CPU::INC(uint16_t addr) {
  uint8_t value = mem_read(addr);
  tick();
  uint8_t res = value + 1;
  set_zn(res);
  mem_write(addr, res);
}

void CPU::INX(uint16_t addr) {
  tick();
  X++;
  set_zn(X);
}

void CPU::INY(uint16_t addr) {
  tick();
  Y++;
  set_zn(Y);
}

void CPU::JMP(uint16_t addr) {
  PC = addr;
}

void CPU::JSR(uint16_t addr) {
  uint16_t old_PC = PC - 1;  // already incremented by 2 during addressing
  tick();
  stack_push(old_PC >> 8);
  stack_push(old_PC & 0xFF);
  PC = addr;
}

void CPU::LDA(uint16_t addr) {
  A = mem_read(addr);
  set_zn(A);
}

void CPU::LDX(uint16_t addr) {
  X = mem_read(addr);
  set_zn(X);
}

void CPU::LDY(uint16_t addr) {
  Y = mem_read(addr);
  set_zn(Y);
}

void CPU::LSR(uint16_t addr) {
  uint8_t value = mem_read(addr);
  tick();
  uint8_t res = value >> 1;
  status(Flag::C, (bool)(value & 0x01));
  set_zn(res);
  mem_write(addr, res);
}

void CPU::LSR_A(uint16_t addr) {
  tick();
  uint8_t res = A >> 1;
  status(Flag::C, (bool)(A & 0x01));
  set_zn(res);
  A = res;
}

void CPU::NOP(uint16_t addr) {
  tick();
}

void CPU::ORA(uint16_t addr) {
  uint8_t value = mem_read(addr);
  A = A | value;
  set_zn(A);
}

void CPU::PHA(uint16_t addr) {
  tick();
  stack_push(A);
}

void CPU::PHP(uint16_t addr) {
  tick();
  stack_push(P | 0x30);  // bits 4 and 5 set
}

void CPU::PLA(uint16_t addr) {
  tick();
  tick();
  A = stack_pop();
  set_zn(A);
}

void CPU::PLP(uint16_t addr) {
  tick();
  tick();
  // Always treat bit 5 as 1 and bit 4 as 0
  P = (stack_pop() & ~0x30) | 0x20;
}

void CPU::ROL(uint16_t addr) {
  uint8_t value = mem_read(addr);
  tick();
  uint8_t res = (value << 1) | status(Flag::C);
  status(Flag::C, (bool)(value & 0x80));
  set_zn(res);
  mem_write(addr, res);
}

void CPU::ROL_A(uint16_t addr) {
  tick();
  uint8_t res = (A << 1) | status(Flag::C);
  status(Flag::C, (bool)(A & 0x80));
  set_zn(res);
  A = res;
}

void CPU::ROR(uint16_t addr) {
  uint8_t value = mem_read(addr);
  tick();
  uint8_t res = (value >> 1) | (status(Flag::C) << 7);
  status(Flag::C, (bool)(value & 0x01));
  set_zn(res);
  mem_write(addr, res);
}

void CPU::ROR_A(uint16_t addr) {
  tick();
  uint8_t res = (A >> 1) | (status(Flag::C) << 7);
  status(Flag::C, (bool)(A & 0x01));
  set_zn(res);
  A = res;
}

void CPU::RTI(uint16_t addr) {
  PLP(addr);
  PC = stack_pop() | (stack_pop() << 8);
}

void CPU::RTS(uint16_t addr) {
  tick();
  tick();
  PC = (stack_pop() | (stack_pop() << 8)) + 1;
  tick();
}

void CPU::SBC(uint16_t addr) {
  uint8_t value = ~mem_read(addr);  // 1s complement of operand
  int16_t sum = A + value + status(Flag::C);
  set_cv(A, value, sum);
  set_zn(sum);
  A = sum;
}

void CPU::SEC(uint16_t addr) {
  flag(Flag::C, 1);
}

void CPU::SED(uint16_t addr) {
  flag(Flag::D, 1);
}

void CPU::SEI(uint16_t addr) {
  flag(Flag::I, 1);
}

void CPU::STA(uint16_t addr) {
  mem_write(addr, A);
}

void CPU::STX(uint16_t addr) {
  mem_write(addr, X);
}

void CPU::STY(uint16_t addr) {
  mem_write(addr, Y);
}

void CPU::TAX(uint16_t addr) {
  tick();
  X = A;
  set_zn(X);
}

void CPU::TAY(uint16_t addr) {
  tick();
  Y = A;
  set_zn(Y);
}

void CPU::TSX(uint16_t addr) {
  tick();
  X = SP;
  set_zn(X);
}

void CPU::TXA(uint16_t addr) {
  tick();
  A = X;
  set_zn(A);
}

void CPU::TXS(uint16_t addr) {
  // Note: Does not set Z or N flags
  tick();
  SP = X;
}

void CPU::TYA(uint16_t addr) {
  tick();
  A = Y;
  set_zn(A);
}