#include <stc12.h>
#include <stdio.h>

int putchar(int c)
{
  SBUF = c;

  while (TI==0);

  TI = 0;

  return 0;
}

void initUART(void)
{
  // using Timer1 for baudrate generator
  // 57600bps (28800bps x 2)
  TMOD |= 0x20;   // set TMOD.5; mode=2
  PCON |= 0x80;   // SMOD=1; double bitrate
  TH1   = 0xFF;   // rset reload value (28800bps)
  TCON |= 0x40;   // set TR1; start Timer1
  SCON  = 0x40;   // 8bit-UART(SM0=0,SM1=1)
}

#define PORT_MAX (8)

static unsigned char hdmi_gpio[PORT_MAX][2] = {
  {0b00100000, 0b01000000},
  {0b00010000, 0b01000000},
  {0b10000100, 0b00000000},
  {0b10000010, 0b00000000},
  {0b10000001, 0b00000000},
  {0b01000000, 0b00000010},
  {0b01000000, 0b00100000},
  {0b01000000, 0b00010000}
};

static volatile unsigned char select_port = 0;
static volatile unsigned long sw_count = 0;
static volatile unsigned char timer_count = 0;

void select_prev(void)
{
  select_port--;
  select_port &= 0b00000111;
}

void select_next(void)
{
  select_port++;
  select_port &= 0b00000111;
}

void set_timer(void)
{
  TR0 = 0;
  timer_count = 0;

  TMOD &= ~(T0_GATE | T0_CT | T0_M1);
  TMOD |= T0_M0;

  TH0 = 0x4C;
  TL0 = 0x00;

  TR0 = 1;
  ET0 = 1;
  EA = 1;
}

void loop(void)
{
  unsigned char p4_data;

  if (P3_7) {
    if (sw_count >= 5000) {
      select_next();
      P1 = ~(1 << select_port);

      set_timer();
    }
    sw_count = 0;
  }else{
    sw_count++;
  }

  P0 = hdmi_gpio[select_port][0];

  p4_data = hdmi_gpio[select_port][1];
  if ((P2 >> select_port) & 0x1) {
    p4_data |= 0b00000100;
  }

  P4 = p4_data;
}

void int0_isr(void) __interrupt 0 __using 0
{
  int i;

  P3_6 = 1;
  for (i = 0; i < 10000; i++);
  P3_6 = 0;
}

void timer0_isr(void) __interrupt 1 __using 1
{
  timer_count++;

  if (timer_count == 60) {
    P1 = 0xFF;
    TR0 = 0;
  }else{
    // More 50ms
    TH0 = 0x4C;
    TL0 = 0x00;
  }
}

void main(void)
{
  initUART();

  P4SW = 0b01110000;

  P0M0 = 0b11111111;
  P0M1 = 0b00000000;

  P1M0 = 0b11111111;
  P1M1 = 0b00000000;

  P2M0 = 0b00000000;
  P2M1 = 0b11111111;

  P3M0 = 0b01000000;
  P3M1 = 0b00000100;

  P4M0 = 0b01110111;
  P4M1 = 0b00001000;

  // Turn on "POWER" LED
  P3_6 = 0;

  // Enable IR interrupt
  EX0 = 1;
  IT0 = 1;
  EA = 1;

  // Set input to #1
  P1 = 0b11111110;
  set_timer();

  while (1) {
    loop();
  }
}
