#include <stdio.h>
#include <assert.h>
#include "rtfSimpleUart.h"

// Two-threaded model: FW in one thread, HW in the other

// ---------------------------------------------------------------------
// Channels to communicate between threads
// ---------------------------------------------------------------------

int create(
  void * (*start_routine)(void *),
  void *arg)
{
  __CPROVER_HIDE:;
  __CPROVER_ASYNC_1: start_routine(arg);
  return 0;
}

typedef struct chan_s {
  _Bool req;
  _Bool ack;
  unsigned char command;
  unsigned char address;
  unsigned char payload;
} chan_t;

void chan_init (chan_t *ch) {
  __CPROVER_HIDE:;
  ch->req = 0;
  ch->ack = 0;
}

void chan_destroy (chan_t *ch) {
}

// Models a one-place buffer.
// Send blocks if there's an unreceived message.
inline void chan_send (chan_t *ch, unsigned char command, unsigned char address, unsigned char payload) {
  __CPROVER_HIDE:;
  __CPROVER_atomic_begin();
  __CPROVER_assume(ch->req == ch->ack);
  ch->command = command;
  ch->address = address;
  ch->payload = payload;
  ch->req = !ch->req;
  __CPROVER_atomic_end();
}

// Receive - blocks if there is no message.
inline void chan_recv (chan_t *ch, unsigned char *command, unsigned char *address, unsigned char *payload) {
  __CPROVER_HIDE:;
  __CPROVER_atomic_begin();
  __CPROVER_assume(ch->req != ch->ack);
  *command = ch->command;
  *address = ch->address;
  *payload = ch->payload;
  ch->ack = !ch->ack;
  __CPROVER_atomic_end();
}

// Probe: returns 1 iff there's a message waiting.
inline _Bool chan_probe (chan_t *ch) {
  __CPROVER_HIDE:;
  return (ch->req != ch->ack);
}

// Wait: wait until given condition is true
inline void event_wait (_Bool ev) {
  __CPROVER_HIDE:;
  __CPROVER_assume(ev);
}

// ---------------------------------------------------------------------
// Transactions on the wishbone interface
// These are clock-aware; note that each function in the wb_* familty
// makes one call to next_timeframe. Each, therefore, corresponds
// to one clock cycle.
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
// Temporal abstraction layer
// ---------------------------------------------------------------------

chan_t fw2hw;
chan_t hw2fw;

void hw_cycle(void) {
}

void *
hw_thread(void *arg) {
  int i;
  unsigned char cmd = 0;
  unsigned char addr = 0;
  unsigned char data = 0;

  for (i=0; i<10; i++) {
    if (chan_probe(&fw2hw)) {
      chan_recv(&fw2hw, &cmd, &addr, &data);
      switch (cmd) {
      case 0: 
        wb_reset();
        break;
      case 1:
        wb_write(0xffdc0a00 | addr, data);
        break;
      default:
        wb_idle();
        break;
      }
    } else {
      wb_idle();
    }
  }
}

// ---------------------------------------------------------------------
// Linux-style inb, outb
//
// If/when we decide to run HW and FW in separate threads,
// inb/outb would execute in the FW thread, wb_read/wb_write would
// execute in the hardware thread, and communication between them
// would be via synchronization or fifo channel.
// ---------------------------------------------------------------------

typedef unsigned char u8;

unsigned char inb (unsigned long port) {
  //  return wb_read(port);
  return 0;
}

void reset (void) {
  chan_send(&fw2hw, 0, 0, 0);
}

void outb (u8 value, unsigned long port) {
  chan_send(&fw2hw, 1, port & 0x0000000f, value);
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

void *
fw_thread(void *arg) {
  reset();
  outb(42,UART_SPR);
  outb(69,UART_SPR);
}

// ---------------------------------------------------------------------
// Test code
// ---------------------------------------------------------------------

int
main(void)
{
  //  thread t[2];

  chan_init(&fw2hw);
  chan_init(&hw2fw);

  create(hw_thread, 0);
  create(fw_thread, 0);
  
  //  join(t[0], 0);
  //  join(t[1], 0);
  
  //  chan_destroy(&fw2hw);
  //  chan_destroy(&hw2fw);
  
  return 0;
}

