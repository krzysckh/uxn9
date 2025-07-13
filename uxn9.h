#ifndef __UXN9_H
#define __UXN9_H

#include <u.h>
#include <libc.h>

typedef u16int (*Dev)(u8int, u16int);

typedef struct Uxn
{
  u16int pc;
  u8int *mem, rst[0x100], wst[0x100], rstp, wstp;
  Dev devices[0x100];
} Uxn;

void vm(Uxn *);

/* system */
void init_system_device(Uxn *);

/* console */
void init_console_device(Uxn *);

/* screen */
void init_screen_device(Uxn *);
void screen_main_loop(Uxn *);

#endif /* __UXN9_H */
