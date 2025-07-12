#include "uxn9.h"

#define CONSOLE_WRITE 0x18
#define CONSOLE_ERROR 0x19

static u16int
console_write(u16int ch)
{
  write(1, ((u8int*)&ch), 1);
  return 0;
}

static u16int
console_error(u16int ch)
{
  write(2, ((u8int*)&ch), 1);
  return 0;
}

void
init_console_device(Uxn *uxn)
{
  uxn->devices[CONSOLE_WRITE] = console_write;
  uxn->devices[CONSOLE_ERROR] = console_error;
}
