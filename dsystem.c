#include "uxn9.h"

#define NBANKS 255

/* There can be NBANKS banks with 1<<16 bytes of memory each.
 * they're allocated on-demand
 */

static u8int *banks[NBANKS] = {0};

extern u8int colors[12];
extern void print_stacks(Uxn *);

u8int *
system_getbank(u16int id)
{
  if (id >= NBANKS)
    sysfatal("asked for bank with id=%d, NBANKS=%d. bump NBANKS.", id, NBANKS);

  if (!banks[id])
    banks[id] = mallocz(1<<16, 1);

  return banks[id];
}

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

#define VAL(x)  x = uxn->mem[addr++]
#define VAL2(x) x = (uxn->mem[addr]<<8)|uxn->mem[addr+1], addr += 2

static void
system_expansion(Uxn *uxn)
{
  u8int op;
  u16int a, b, c, d, e, addr = DEV2(SYSTEM_EXPANSION);
  u8int *B, *B2;
  enum {FILL = 0, CPYL = 1, CPYR = 2};

  VAL(op);
  switch (op) {
  case FILL:
    VAL2(a);
    VAL2(b);
    VAL2(c);
    VAL(d);
    B = system_getbank(b);
    memset(B+c, d&0xff, a);
    break;
  case CPYL:
  case CPYR:
    VAL2(a); /* len   */
    VAL2(b); /* srcb  */
    VAL2(c); /* addr  */
    VAL2(d); /* dstb  */
    VAL2(e); /* addr' */
    /* print("CPY (%s) len=%d src=%d addr=%d dst=%d addr'=%d\n", op == CPYL ? "CPYL" : "CPYR", a, b, c, d, e); */
    B = system_getbank(b), B2 = system_getbank(d);
    if (op == CPYL)
      memcpy(B2+e, B+c, a);
    else
      memmove(B2+e, B+c, a); /* TODO: is this ok? */
    break;
  }
}

static void
system_red(Uxn *uxn)
{
  system_set_color(2, DEV2(SYSTEM_RED));
}

static void
system_green(Uxn *uxn)
{
  system_set_color(1, DEV2(SYSTEM_GREEN));
}

static void
system_blue(Uxn *uxn)
{
  system_set_color(0, DEV2(SYSTEM_BLUE));
}

static void
handle_quit(void *arg)
{
  Uxn *uxn = (Uxn *)arg;
  lock(&uxn->l); // wait until current vector is finished
  unlock(&uxn->l);
  if (DEV(SYSTEM_STATE)) {
    if (0x7f&DEV(SYSTEM_STATE))
      exitall(uxn, "non-normal termination");
    exitall(uxn, nil);
  }
}

static void
system_state(Uxn *uxn)
{
  proccreate(handle_quit, uxn, mainstacksize);
  // handle_quit(uxn);
}

static void
system_debug(Uxn *uxn)
{
  print_stacks(uxn);
}

void
init_system_device(Uxn *uxn)
{
  banks[0] = uxn->mem;

  uxn->trigo[SYSTEM_RED      ] = system_red;
  uxn->trigo[SYSTEM_GREEN    ] = system_green;
  uxn->trigo[SYSTEM_BLUE     ] = system_blue;
  uxn->trigo[SYSTEM_STATE    ] = system_state;
  uxn->trigo[SYSTEM_DEBUG    ] = system_debug;
  uxn->trigo[SYSTEM_EXPANSION] = system_expansion;
}
