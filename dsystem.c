#include "uxn9.h"

#define SYSTEM_EXPANSION 0x02
#define SYSTEM_WST       0x04
#define SYSTEM_RST       0x05
#define SYSTEM_METADATA  0x06
#define SYSTEM_RED       0x08
#define SYSTEM_GREEN     0x0a
#define SYSTEM_BLUE      0x0c
#define SYSTEM_DEBUG     0x0e
#define SYSTEM_STATE     0x0f

/* There can be 0xffff banks with 0xffff bytes of memory each
 * i allocate them as a linked list. it's not the fastest but allows using all of the banks
 * in a random order with minimal memory consumption
 */

typedef struct Bank
{
  u16int id;
  u8int *mem;
  struct Bank *next;
} Bank;

static Uxn *current_uxn = nil;
static Bank *banks = nil;

extern u8int colors[12];
extern void print_stacks(Uxn *);

static u16int system_current_metadata = 0;

static Bank *
mallocbank(u16int id)
{
  Bank *b = mallocz(sizeof(Bank), 1);
  b->mem = mallocz(0xffff, 1);
  b->id = id;

  return b;
}

static Bank *
getbank(u16int id)
{
  Bank *cur;

  /* initialize banks if there's none allocd */
  if (banks == nil) {
    banks = mallocbank(id);
    return banks;
  }

  cur = banks;

  /* O(n) search for the right bank */
  while (1) {
    if (cur->id == id)
      return cur;
    if (cur->next)
      cur = cur->next;
    else
      break;
  }

  /* allocate and stash if not found */
  cur->next = mallocbank(id);
  return cur->next;
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

#define VAL(x)  x = current_uxn->mem[addr++]
#define VAL2(x) x = (current_uxn->mem[addr]<<8)|current_uxn->mem[addr+1], addr += 2

static u16int
system_expansion(u8int getp, u16int addr)
{
  u8int op;
  u16int a, b, c, d, e;
  Bank *B, *B2;
  enum {FILL = 0, CPYL = 1, CPYR = 2};
  if (getp)
    sysfatal("getp expansion");

  VAL(op);
  switch (op) {
  case FILL:
    VAL2(a);
    VAL2(b);
    VAL2(c);
    VAL(d);
    B = getbank(b);
    memset(B->mem+c, d&0xff, a);
    break;
  case CPYL:
  case CPYR:
    VAL2(a); /* len   */
    VAL2(b); /* srcb  */
    VAL2(c); /* addr  */
    VAL2(d); /* dstb  */
    VAL2(e); /* addr' */
    // print("CPY len=%d src=%d addr=%d dst=%d addr'=%d\n", a, b, c, d, e);
    B = getbank(b), B2 = getbank(d);
    if (op == CPYL)
      memcpy(B2->mem+e, B->mem+c, a);
    else
      memmove(B2->mem+e, B->mem+c, a); /* TODO: is this ok? */
    break;
  }

  return 0;
}

// FIXME: getp color

u16int
system_red(u8int getp, u16int dat)
{
  if (getp)
    return 0;
  system_set_color(2, dat);
  return 0;
}

u16int
system_green(u8int getp, u16int dat)
{
  if (getp)
    return 0;
  system_set_color(1, dat);
  return 0;
}

u16int
system_blue(u8int getp, u16int dat)
{
  if (getp)
    return 0;
  system_set_color(0, dat);
  return 0;
}

u16int
system_state(u8int getp, u16int dat)
{
  if (getp)
    return 0;
  if (0x7f&dat)
    threadexitsall("non-normal termination");
  threadexitsall(nil);
  return 0;
}

u16int
system_debug(u8int getp, u16int dat)
{
  if (getp)
    sysfatal("get debug?");
  print_stacks(current_uxn);
  return 0;
}

DEFGETSET(system_metadata, system_current_metadata);
DEFGETSET(system_wst, current_uxn->wstp);
DEFGETSET(system_rst, current_uxn->rstp);

void
init_system_device(Uxn *uxn)
{
  banks = mallocbank(0); /* zero-bank */
  free(banks->mem);
  banks->mem = uxn->mem;
  current_uxn = uxn;

  uxn->devices[SYSTEM_RED      ] = system_red;
  uxn->devices[SYSTEM_GREEN    ] = system_green;
  uxn->devices[SYSTEM_BLUE     ] = system_blue;
  uxn->devices[SYSTEM_STATE    ] = system_state;
  uxn->devices[SYSTEM_METADATA ] = system_metadata;
  uxn->devices[SYSTEM_DEBUG    ] = system_debug;
  uxn->devices[SYSTEM_EXPANSION] = system_expansion;
  uxn->devices[SYSTEM_WST      ] = system_wst;
  uxn->devices[SYSTEM_RST      ] = system_rst;
}
