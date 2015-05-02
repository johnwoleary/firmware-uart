#include "rtfSimpleUart.h"
#include<assert.h>

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

void wb_read(_u32 addr, _u8 *b) {
  // Master presents address, data, asserts CYC and STB, deasserts WE
  rtfSimpleUart.adr_i = addr;
  rtfSimpleUart.dat_i = b;
  rtfSimpleUart.we_i = 0;
  rtfSimpleUart.cyc_i = 1;
  rtfSimpleUart.stb_i = 1;
  set_inputs();
  //assert(rtfSimpleUart.ack_o == 1);
  // We assume the acknowledge comes right away.
  // NB Wishbone does not guarantee this in general!
  // The simple UART appears to derive ack_o combinatorially from stb_i and cyc_i.
  *b = rtfSimpleUart.dat_o;
  next_timeframe();
  rtfSimpleUart.we_i = 0;
  rtfSimpleUart.cyc_i = 0;
  rtfSimpleUart.stb_i = 0;
}

// ---------------------------------------------------------------------
// 
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

int main(void) {

  wb_reset();
  wb_idle();
  // turn off the interrupts
  wb_write(UART_IE, 0);
  wb_idle();
  // turn off hardware flow control
  wb_write(UART_CR, 0);
  wb_idle();

  // attempt a few writes and reads to the scratchpad

  _u8 b;
  wb_read(UART_SPR, &b); // scratchpad is 0 after reset
  assert(b == 0);
  wb_idle();
  wb_write(UART_SPR, 42); // write a value
  wb_idle();
  wb_read(UART_SPR, &b);  // read it back
  assert(b == 42);        // same value?
  wb_idle();
  wb_write(UART_SPR, 69); // write a value
  wb_idle();              
  wb_read(UART_SPR, &b);  // read it back
  assert(b == 69);        // same?

  // Back to back reads/writes
  wb_write(UART_SPR,1);
  wb_read(UART_SPR,&b);
  assert(b==1);
  wb_write(UART_SPR,2);
  wb_read(UART_SPR,&b);
  assert(b==2);
  wb_write(UART_SPR,64);
  wb_read(UART_SPR,&b);
  assert(b==64);
  wb_read(UART_SPR,&b);
  assert(b==64);
  wb_read(UART_SPR,&b);
  assert(b==64);
  wb_write(UART_SPR,10);
  wb_write(UART_SPR,9);
  wb_write(UART_SPR,8);
  wb_read(UART_SPR,&b);
  assert(b==8);

  return 0;
}
