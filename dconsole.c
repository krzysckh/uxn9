#include "uxn9.h"

char **console_argv = nil;
int console_argc = 0;

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

#define RUN() if (DEV2(CONSOLE_VECTOR)) { uxn->pc = DEV2(CONSOLE_VECTOR); vm(uxn); }

void
console_main_loop(void *arg)
{
  Uxn *uxn = arg;
  u16int n = 0;
  char *carg, c;
  lock(&uxn->l);
  while (console_argc--) {
    carg = console_argv[n++];
    while (*carg) {
      SDEV(CONSOLE_TYPE, 2);
      SDEV(CONSOLE_READ, *carg++);
      RUN();
      // SDEV(CONSOLE_READ, 0);
    }
    if (console_argc) {
      SDEV(CONSOLE_TYPE, 3); /* spacer */
      SDEV(CONSOLE_READ, '\n');
      RUN();
    } else {
      SDEV(CONSOLE_TYPE, 4); /* arg end */
      SDEV(CONSOLE_READ, '\n');
      RUN();
    }
  }

  unlock(&uxn->l);

  while ((n = read(0, &c, 1))) {
    lock(&uxn->l);
    SDEV(CONSOLE_TYPE, 1);
    SDEV(CONSOLE_READ, c);
    RUN();
    unlock(&uxn->l);
  }
}

void
init_console_device(Uxn *uxn)
{
  uxn->trigo[CONSOLE_WRITE] = console_write;
  uxn->trigo[CONSOLE_ERROR] = console_error;

  if (console_argc)
    SDEV(CONSOLE_TYPE, 1);
  else
    SDEV(CONSOLE_TYPE, 0);
}
