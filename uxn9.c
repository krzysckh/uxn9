#include "uxn9.h"

#define KEEPP(x)  (1<<7&x)
#define RETP(x)   (1<<6&x)
#define SHORTP(x) ((1<<5&x)?1:0)

#define WRAP(x) (((u8int)x)&0xff)
#define IT(n, s, sp)  (s[WRAP((*sp)-((n)+1))])
#define IT2(n, s, sp) ((s[WRAP((*sp)-(((n)*2)+2))]<<8)|s[WRAP((*sp)-((n*2)+1))])
#define IT2_(n, s, sp) ((s[WRAP((*sp)-(((n)*2)+3))]<<8)|s[WRAP((*sp)-((n*2)+2))])

#define PUSH(x, s, sp)   (s[(*sp)++] = x)
#define PUSH2(x, s, sp)  { PUSH((x)>>8, s, sp); PUSH((x), s, sp); }
#define Pto(v, ST, SP)                          \
  { if (_2)                                     \
      PUSH2(v, ST, SP)                          \
      else                                      \
        PUSH(v, ST, SP); }
#define P(v, op, uxn) Pto(v, S, Q)
#define STACK(op,uxn)   (RETP(op) ? uxn->rst : uxn->wst)
#define STACKPT(op,uxn) (RETP(op) ? &uxn->rstp : &uxn->wstp)

#define STACKS(op, uxn, ST, SP) u8int *ST = S, *SP = Q;

#define MEM(at) (uxn->mem[(at)&0xffff])
#define LIT()                                   \
  { STACKS(op, uxn, ST, SP);                    \
    if (_2) {                                   \
      PUSH(MEM(uxn->pc+1), ST, SP);             \
      PUSH(MEM(uxn->pc+2), ST, SP);             \
      NEXT(3);                                  \
    } else {                                    \
      PUSH(MEM(uxn->pc+1), ST, SP);             \
      NEXT(2); }}

#define UN(op, sp, n) if (!KEEPP(op)) (*sp)-=n;

#define OPCn(n, op, uxn, code)                  \
  { STACKS(op, uxn, ST, SP);                    \
    u16int RES = 0;                             \
    USED(RES);                                  \
    code;                                       \
    UN(op, SP, n);                              \
    if (_2)                                     \
      PUSH2(RES, ST, SP)                        \
    else                                        \
      PUSH(RES, ST, SP); };

#define PEEK()  (_2 ? IT2(0, S, Q) : IT(0, S, Q))
#define PEEKb() (_2 ? IT2(1, S, Q) : IT(1, S, Q))
#define PEEKc() (_2 ? IT2(2, S, Q) : IT(2, S, Q))

#define S STACK(op,uxn)
#define Q STACKPT(op,uxn)

#define NEXT(n) {uxn->pc += n; goto loop;}
#define JMP(n) {uxn->pc = n; goto loop;}
#define IMM2() ((MEM(uxn->pc+1)<<8)|MEM(uxn->pc+2))
#define BINOP(B, op, uxn) { OPCn(2+(2*_2), op, uxn, { RES = PEEKb() B PEEK(); }); NEXT(1) };

#define OP8(B, op, uxn) {u16int a = PEEK(), b = PEEKb(); UN(op, Q, 2+(2*_2)); PUSH(a B b, S, Q); NEXT(1);}

static void
usage(void)
{
  fprint(2, "usage: %s file.rom\n", argv0);
  threadexitsall("usage");
}

static void
print_stack(char *name, u8int *st, u8int p)
{
  u16int i = 0;
  for (; i < p; ++i)
    fprint(2, "%02x ", st[i]);
  fprint(2, "|(%d) %s\n", p, name);
}

void
print_stacks(Uxn *uxn)
{
  print_stack("WST", uxn->wst, uxn->wstp);
  print_stack("RST", uxn->rst, uxn->rstp);
}

void
vm(Uxn *uxn)
{
  u8int op;
loop:
  op = uxn->mem[uxn->pc];
  u8int K = KEEPP(op), _2 = SHORTP(op);
  //print_stacks(uxn);
  //print("[%04x] OPC: %02x, opcod: %02x\n", uxn->pc, op&0x1f, op);
  switch (op) { /* IMMEDIATE dispatch */
  case 0x20: {u8int a = IT(0, uxn->wst, &uxn->wstp); uxn->wstp--; if (!a) NEXT(3); /* else fallthrough */}
  case 0x40: {NEXT(3+IMM2());}
  case 0x60: {u16int pt = IMM2();
               PUSH2(uxn->pc+3, uxn->rst, &uxn->rstp); NEXT(pt+3);}
  /* LIT handled later */
  }
  switch (op&0x1f) {
  case 0x00: if (K) LIT() else return;
  case 0x01: OPCn(1+_2, op, uxn, { RES = PEEK() + 1; }); NEXT(1);
  case 0x02: if (K) { /* NOOP */ NEXT(1); } else { UN(op, Q, 1+_2); NEXT(1); }
    /* TODO: collapse 03 and 04 into one */
  case 0x03: {u16int a = PEEK()/*, b = PEEKb()*/; UN(op, Q, 2+(2*_2)); P(a, op, uxn); NEXT(1); }
  case 0x04: {u16int a = PEEK(), b = PEEKb(); UN(op, Q, 2+(2*_2)); P(a, op, uxn); P(b, op, uxn); NEXT(1); }
  case 0x05: {u16int c = PEEK(), b = PEEKb(), a = PEEKc();
      UN(op, Q, 3+(3*_2)); P(b, op, uxn); P(c, op, uxn); P(a, op, uxn); NEXT(1); }
  case 0x06: {u16int a = PEEK(); UN(op, Q, 1+_2); P(a, op, uxn); P(a, op, uxn); NEXT(1); }
  case 0x07: {u16int b = PEEK(), a = PEEKb(); UN(op, Q, 2+(2*_2)); P(a, op, uxn); P(b, op, uxn); P(a, op, uxn); NEXT(1); }
  case 0x08: OP8(==, op, uxn);
  case 0x09: OP8(!=, op, uxn);
  case 0x0a: OP8(<, op, uxn); /* reverse order of args */
  case 0x0b: OP8(>, op, uxn);
  case 0x0c: {u16int v = PEEK(); UN(op, Q, 1+_2); if (_2) JMP(v) else NEXT(((s8int)(v&0xff))+1); }
  case 0x0d: {u16int pt = PEEK(), cond = IT(_2 ? 2 : 1, S, Q); UN(op, Q, 2+_2);
      if (cond) { if (_2) JMP(pt) else NEXT(((s8int)(pt&0xff))+1); } NEXT(1); }
  case 0x0e: {u16int v = PEEK(); UN(op, Q, 1+_2); PUSH2(uxn->pc+1, uxn->rst, &uxn->rstp); if (_2) JMP(v) else NEXT(((s8int)(v&0xff))+1);}
  case 0x0f: {u16int v = PEEK(); UN(op, Q, 1+_2); Pto(v, (RETP(op)?uxn->wst:uxn->rst), (RETP(op)?&uxn->wstp:&uxn->rstp)); NEXT(1);}
  case 0x10: {u8int a = IT(0, S, Q); UN(op, Q, 1);
      u16int v = _2 ? MEM(a)<<8|MEM((a+1)&0xff) : MEM(a);
      P(v, op, uxn); NEXT(1); }
  case 0x11: {u8int pt = IT(0, S, Q);
      u16int v = IT2_(0, S, Q);
      UN(op, Q, 2+_2);
      if (_2) MEM(pt) = v>>8, MEM((pt+1)&0xff) = v; else MEM(pt) = v;
      NEXT(1); }
  case 0x12: {s8int pt = IT(0, S, Q); UN(op, Q, 1);
      if (_2) { PUSH2(MEM(uxn->pc+pt+1)<<8|MEM(uxn->pc+pt+2), S, Q) }
      else { PUSH(MEM(uxn->pc+pt+1), S, Q); } NEXT(1);}
  case 0x13: {s8int addr = IT(0, S, Q); u16int val = _2 ? IT2_(0, S, Q) : IT(1, S, Q);
      UN(op, Q, 2+_2);
      if (_2) {
        MEM(uxn->pc+addr+1) = val>>8;
        MEM(uxn->pc+addr+2) = val;
      } else MEM(uxn->pc+addr+1) = val;
      NEXT(1);}
  case 0x14: {u16int a = IT2(0, S, Q); UN(op, Q, 2);
      u16int v = _2 ? MEM(a)<<8|MEM(a+1) : MEM(a);
      P(v, op, uxn); NEXT(1); }
  case 0x15: {u16int pt = IT2(0, S, Q), v = IT2(1, S, Q); UN(op, Q, 3+_2);
      if (_2) MEM(pt) = v>>8, MEM(pt+1) = v; else MEM(pt) = v;
      NEXT(1); }
  case 0x16: {u16int dev = IT(0, S, Q), res;
      if (uxn->devices[dev]) res = uxn->devices[dev](1, 0); else
#ifdef UXN9_FATAL_NODEVICE
        sysfatal("unknown device %02x", dev);
#else
        res = 0;
#endif
      UN(op, Q, 1);
      P(res, op, uxn);
      NEXT(1);}
  case 0x17: {u16int dev = IT(0, S, Q), val = _2 ? IT2_(0, S, Q) : IT(1, S, Q);
      UN(op, Q, 2+_2);
      if (uxn->devices[dev]) uxn->devices[dev](0, val);
#ifdef UXN9_FATAL_NODEVICE
      else sysfatal("unknown device %02x", dev);
#endif
      NEXT(1);}
  case 0x18: BINOP(+, op, uxn);
  case 0x19: BINOP(-, op, uxn);
  case 0x1a: BINOP(*, op, uxn);
  case 0x1b: { OPCn(2+(2*_2), op, uxn, { u16int b = PEEK(); if (b) RES = PEEKb() / b; else RES = 0; }); NEXT(1) }; /* safe DIV */
  case 0x1c: BINOP(&, op, uxn);
  case 0x1d: BINOP(|, op, uxn);
  case 0x1e: BINOP(^, op, uxn);
  case 0x1f: {
    u8int sft = IT(0, S, Q);
    u16int v = IT2_(0, S, Q);
    if (!_2) v = v&0xff;
    UN(op, Q, 2+_2);
    P((v>>(0x0f&sft))<<((0xf0&sft)>>4), op, uxn);
    NEXT(1); }
  default: sysfatal("unimplemented opcode: %02x", op&0x1f);
  }
}

/* FIXME: move to some header */
extern char **console_argv;

/* note to future self: cannot alloc big structs on stack with libthread */

void
threadmain(int argc, char **argv)
{
  Uxn *uxn = mallocz(sizeof(Uxn), 1);
  u8int *mem = mallocz(1<<16, 1);
  int fd;

  ARGBEGIN {
    default: usage();
  } ARGEND;

  if (argc != 1) usage();

  fd = open(argv[0], OREAD);
  console_argv = argv+1;
  if (fd < 0)
    sysfatal("couldn't read file %s", argv[0]);
  int rd = read(fd, mem+0x100, (1<<16)-0x100);
  close(fd);

  init_system_device(uxn);
  init_console_device(uxn);
  init_screen_device(uxn);
  init_mouse_device(uxn);
  init_datetime_device(uxn);
  init_controller_device(uxn);

  uxn->mem = mem;
  uxn->pc = 0x100;

  vm(uxn);
  //proccreate(screen_main_loop, uxn, mainstacksize);
  screen_main_loop(uxn);

  //threadexitsall(nil);
}
