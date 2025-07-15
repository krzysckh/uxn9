#include "uxn9.h"

#define CONSOLE_WRITE 0x18
#define CONSOLE_ERROR 0x19
#define CONSOLE_TYPE  0x17

char **console_argv;

u16int
console_type(u8int getp, u16int dat)
{
  if (getp)
    print("console_type queried\n");
  return 0;
}

static u16int
console_write(u8int getp, u16int ch)
{
  if (getp)
    return 0;
  write(1, ((u8int*)&ch), 1);
  return 0;
}

static u16int
console_error(u8int getp, u16int ch)
{
  if (getp)
    return 0;
  write(2, ((u8int*)&ch), 1);
  return 0;
}

void
init_console_device(Uxn *uxn)
{
  uxn->devices[CONSOLE_WRITE] = console_write;
  uxn->devices[CONSOLE_ERROR] = console_error;
  uxn->devices[CONSOLE_TYPE ] = console_type;
}
