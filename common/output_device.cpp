#include <cstdint>

#include "cpuid.hpp"
#include "output_device.hpp"
#include "x86.hpp"

void output_device::puts(const char *s)
{
  char c;
  while ((c = *(s++)) != 0)
    putc(c);
}

class qemu_output_device : public output_device {
  static constexpr uint16_t qemu_debug_port = 0xe9;
public:
  void putc(char c) override
  {
    outbi<qemu_debug_port>(c);;
  }
};

class serial_output_device : public output_device {
  uint16_t base_port_;

  enum class uart_reg {
    THR = 0,
    DLL = 0,
    DLH = 1,
    IER = 1,
    FCR = 2,
    LCR = 3,
    MCR = 4,
    LSR = 5,
  };

  enum {
    LCR_DLAB = 0x80,
    LCR_8BITDATA = 0x03,
    LCR_1BITSTOP = 0,

    FCR_ENABLE_FIFO = 1,
    FCR_CLEAR_RX = 2,
    FCR_CLEAR_TX = 4,

    MCR_DTS = 1,
    MCR_RTS = 2,

    LSR_CAN_TX = 0x20,
  };

  void out(uart_reg reg, uint8_t v)
  {
    outb(base_port_ + (uint16_t)reg, v);
  }

  uint8_t in(uart_reg reg)
  {
    return inb(base_port_ + (uint16_t)reg);
  }

public:

  void putc(char c) override
  {
    while (not (in(uart_reg::LSR) & LSR_CAN_TX))
      pause();

    out(uart_reg::THR, (uint8_t)c);
  }

  serial_output_device(uint16_t base_port)
    : base_port_(base_port)
  {
    out(uart_reg::LCR, LCR_DLAB);
    out(uart_reg::DLL, 1);      // Divisor=1 -> 115200 baud
    out(uart_reg::DLH, 0);

    out(uart_reg::LCR, LCR_8BITDATA | LCR_1BITSTOP);
    out(uart_reg::IER, 0);      // Disable all interrupts
    out(uart_reg::FCR, FCR_ENABLE_FIFO | FCR_CLEAR_RX | FCR_CLEAR_TX);
    out(uart_reg::MCR, MCR_RTS | MCR_DTS);
  }
};

output_device *output_device::make()
{
  if (running_virtualized()) {
    static qemu_output_device qemu_output;
    return &qemu_output;
  }

  static serial_output_device serial_output { 0x3f8 };
  return &serial_output;
}
