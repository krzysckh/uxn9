#include "uxn9.h"

char **console_argv;

static void
console_type(Uxn *uxn)
{
  USED(uxn);
}

static void
console_write(Uxn *uxn)
{
  write(1, ((u8int*)uxn->dev+CONSOLE_WRITE), 1);
}

static void
console_error(Uxn *uxn)
{
  write(2, ((u8int*)uxn->dev+CONSOLE_ERROR), 1);
}

void
init_console_device(Uxn *uxn)
{
  uxn->trigo[CONSOLE_WRITE] = console_write;
  uxn->trigo[CONSOLE_ERROR] = console_error;
  uxn->trigo[CONSOLE_TYPE ] = console_type;
}
