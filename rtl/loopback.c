#include<assert.h>
#include "rtfSimpleUart.h"

// ---------------------------------------------------------------------
// Transactions on the wishbone interface
// ---------------------------------------------------------------------

void wb_reset(void) {
  rtfSimpleUart.rst_i = 1;
  set_inputs();
  next_timeframe();
  rtfSimpleUart.rst_i = 0;
  // Rule 3.20
  rtfSimpleUart.stb_i = 0; rtfSimpleUart.cyc_i = 0;
}

void wb_idle() {
  set_inputs();
  next_timeframe();
}

void wb_write(_u32 addr, _u8 b) {
  // Master presents address, data, asserts WE, CYC and STB
  rtfSimpleUart.adr_i = addr;
  rtfSimpleUart.dat_i = b;
  rtfSimpleUart.we_i = 1;
  rtfSimpleUart.cyc_i = 1;
  rtfSimpleUart.stb_i = 1;
  set_inputs();
  //assert(rtfSimpleUart.ack_o == 1);
  // We assume the acknowledge comes right away.
  // NB Wishbone does not guarantee this in general!
  // The simple UART appears to derive ack_o combinatorially from stb_i and cyc_i.
  next_timeframe();
  rtfSimpleUart.we_i = 0;
  rtfSimpleUart.cyc_i = 0;
  rtfSimpleUart.stb_i = 0;
}

_u8 wb_read(_u32 addr) {
  // Master presents address, data, asserts CYC and STB, deasserts WE
  rtfSimpleUart.adr_i = addr;
  rtfSimpleUart.we_i = 0;
  rtfSimpleUart.cyc_i = 1;
  rtfSimpleUart.stb_i = 1;
  set_inputs();
  //assert(rtfSimpleUart.ack_o == 1);
  // We assume the acknowledge comes right away.
  // NB Wishbone does not guarantee this in general!
  // The simple UART appears to derive ack_o combinatorially from stb_i and cyc_i.
  _u8 b = rtfSimpleUart.dat_o;
  next_timeframe();
  rtfSimpleUart.we_i = 0;
  rtfSimpleUart.cyc_i = 0;
  rtfSimpleUart.stb_i = 0;
  return b;
}

// ---------------------------------------------------------------------
// Linux-style inb, outb
//
// Right now, these call wb_read and wb_write directly.
//
// If/when we decide to run HW and FW in separate threads,
// inb/outb would execute in the FW thread, wb_read/wb_write would
// execute in the hardware thread, and communication between them
// would be via synchronization or fifo channel.
// ---------------------------------------------------------------------

typedef unsigned char u8;

unsigned char inb (unsigned long port) {
  return wb_read(port);
}

void outb (u8 value, unsigned long port) {
  wb_write(port, value);
}

// ---------------------------------------------------------------------
// UART Firmware
// ---------------------------------------------------------------------

// UART addresses
// Some of these are not implemented yet in the opencores UART.
#define UART_TR 0xffdc0a00         // tx/rx data (RW)
#define UART_LS (UART_TR + 1)      // line status (RO)
#define UART_MS (UART_TR + 2)      // modem status (RO)
#define UART_IS (UART_TR + 3)      // interrupt status (RO)
#define UART_IE (UART_TR + 4)      // interrupt enable (RW)
#define UART_FF (UART_TR + 5)      // frame format (RW)
#define UART_MC (UART_TR + 6)      // modem control (RW)
#define UART_CR (UART_TR + 7)      // uart control (RW)
#define UART_CM0 (UART_TR + 8)     // clock multiplier byte 0 - least significant (RW)
#define UART_CM1 (UART_TR + 9)     //                  byte 1
#define UART_CM2 (UART_TR + 10)    //                  byte 2
#define UART_CM3 (UART_TR + 11)    //                  byte 3 - most significant (RW)
#define UART_FC (UART_TR + 12)     // fifo control (RW)
#define UART_SPR (UART_TR + 15)    // scratchpad (RW)

// ---------------------------------------------------------------------
// Main test routine
// ---------------------------------------------------------------------

int main(void) {

  _u8 b;

  wb_reset();
  wb_idle();
  // turn off the interrupts
  outb(0x03, UART_IE);
  wb_idle();
  // turn off hardware flow control
  outb(0x00, UART_CR);
  wb_idle();

  // Use a really large clk multiplier! (UART_CM0 is ignored)
  outb (0x80, UART_CM3);
  outb (0x00, UART_CM2);
  outb (0x00, UART_CM1);

  // Loopback mode!
  outb (0x13, UART_MC);

  // ship out a byte through the serial port
  b = inb(UART_LS);
  assert(b & 0x40); // tx empty

  outb(0xab, UART_TR);
  b = inb(UART_LS);
  assert(~b & 0x20); // tx full

  for (int i = 0; i < 100; i++) 
    wb_idle();
  b = inb(UART_LS);
  assert(b && 0x20); // tx empty
  // ship a second byte
  outb(0xcd, UART_TR);
  for (int i = 0; i < 220; i++) 
    wb_idle();
  // Check for data ready
  b = inb(UART_LS);
  assert(b && 0x01);
  b = inb(UART_TR);
  assert(b == 0xab);
  for (int i = 0; i < 580; i++) 
    wb_idle();
  b = inb(UART_LS);
  assert(b && 0x20); // tx empty
  assert(0); // fail, so we can get a counterexample and some waveforms.

#ifdef NOPE
  // attempt a few writes and reads to the scratchpad

  b = inb(UART_SPR); // scratchpad is 0 after reset
  assert(b == 0);
  wb_idle();
  outb(0x42, UART_SPR); // write a value
  wb_idle();
  b = inb(UART_SPR);  // read it back
  assert(b == 0x42);        // same value?
  wb_idle();
  outb(0x69, UART_SPR); // write a value
  wb_idle();              
  b = inb(UART_SPR);  // read it back
  assert(b == 0x69);        // same?

  // Back to back reads/writes
  outb(0x01, UART_SPR);
  b = inb(UART_SPR);
  assert(b==0x01);
  outb(0x02, UART_SPR);
  b = inb(UART_SPR);
  assert(b==0x02);
  outb(0x40, UART_SPR);
  b = inb(UART_SPR);
  assert(b==0x40);
  b = inb(UART_SPR);
  assert(b==0x40);
  b = inb(UART_SPR);
  assert(b==0x40);
  outb(0x0a, UART_SPR);
  outb(0x09, UART_SPR);
  outb(0x08, UART_SPR);
  b = inb(UART_SPR);
  assert(b==0x08);
#endif

  return 0;
}
