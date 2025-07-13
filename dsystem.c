#include "uxn9.h"

#define SYSTEM_RED 0x08
#define SYSTEM_GREEN 0x0a
#define SYSTEM_BLUE 0x0c

extern u8int colors[12];

static void
system_set_color(u8int off, u16int dat)
{
  u8int c1 = (dat&0xf000)>>12,
        c2 = (dat&0xf00)>>8,
        c3 = (dat&0xf0)>>4,
        c4 = (dat&0xf);
  c1 = c1|(c1<<4);
  c2 = c2|(c2<<4);
  c3 = c3|(c3<<4);
  c4 = c4|(c4<<4);

  colors[off] = c1;
  colors[off+3] = c2;
  colors[off+6] = c3;
  colors[off+9] = c4;
}

u16int
system_red(u8int getp, u16int dat)
{
  if (getp)
    sysfatal("getp color");
  system_set_color(2, dat);
  return 0;
}

u16int
system_green(u8int getp, u16int dat)
{
  if (getp)
    sysfatal("getp color");
  system_set_color(1, dat);
  return 0;
}

u16int
system_blue(u8int getp, u16int dat)
{
  if (getp)
    sysfatal("getp color");
  system_set_color(0, dat);
  return 0;
}

void
init_system_device(Uxn *uxn)
{
  uxn->devices[SYSTEM_RED] = system_red;
  uxn->devices[SYSTEM_GREEN] = system_green;
  uxn->devices[SYSTEM_BLUE] = system_blue;
}
