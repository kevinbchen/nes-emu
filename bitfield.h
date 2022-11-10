
#pragma once
#include <cstdint>

template <typename T, int index, int bits>
class BitField {
 public:
  static const T mask = (1u << bits) - 1u;
  T value;

  BitField& operator=(const BitField& other) {
    *this = T(other);
    return *this;
  };

  template <class T2>
  BitField& operator=(T2 v) {
    value = (value & ~(mask << index)) | ((v & mask) << index);
    return *this;
  }

  operator T() const { return (value >> index) & mask; }

  explicit operator bool() const { return value & (mask << index); }

  BitField& operator++() { return *this = *this + 1; }

  T operator++(int) {
    T r = *this;
    ++*this;
    return r;
  }

  BitField& operator--() { return *this = *this - 1; }

  T operator--(int) {
    T r = *this;
    --*this;
    return r;
  }
};

template <int index, int bits>
using BitField8 = BitField<uint8_t, index, bits>;

template <int index, int bits>
using BitField16 = BitField<uint16_t, index, bits>;