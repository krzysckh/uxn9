#include "uxn9.h"

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

static Bank *banks = nil;

extern u8int colors[12];
extern void print_stacks(Uxn *);

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

#define VAL(x)  x = uxn->mem[addr++]
#define VAL2(x) x = (uxn->mem[addr]<<8)|uxn->mem[addr+1], addr += 2

static void
system_expansion(Uxn *uxn)
{
  u8int op;
  u16int a, b, c, d, e, addr = DEV2(SYSTEM_EXPANSION);
  Bank *B, *B2;
  enum {FILL = 0, CPYL = 1, CPYR = 2};

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
system_state(Uxn *uxn)
{
  if (DEV(SYSTEM_STATE)) {
    if (0x7f&DEV(SYSTEM_STATE))
      exitall("non-normal termination");
    exitall(nil);
  }
}

static void
system_debug(Uxn *uxn)
{
  print_stacks(uxn);
}

void
init_system_device(Uxn *uxn)
{
  banks = mallocbank(0); /* zero-bank */
  free(banks->mem);
  banks->mem = uxn->mem;

  uxn->trigo[SYSTEM_RED      ] = system_red;
  uxn->trigo[SYSTEM_GREEN    ] = system_green;
  uxn->trigo[SYSTEM_BLUE     ] = system_blue;
  uxn->trigo[SYSTEM_STATE    ] = system_state;
  uxn->trigo[SYSTEM_DEBUG    ] = system_debug;
  uxn->trigo[SYSTEM_EXPANSION] = system_expansion;
}
