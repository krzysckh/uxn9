#include "uxn9.h"

#define KEEPP(x)  (1<<7&x)
#define RETP(x)   (1<<6&x)
#define SHORTP(x) ((1<<5&x)?1:0)

#define IT(n, s, sp)  (s[(*sp)-(n+1)])
#define IT2(n, s, sp) ((s[(*sp)-((n*2)+2)]<<8)|s[(*sp)-((n*2)+1)])
#define IT2_(n, s, sp) ((s[(*sp)-((n*2)+3)]<<8)|s[(*sp)-((n*2)+2)])

#define A(s, sp) IT(0, s, sp)
#define A2(s, sp) IT2(0, s, sp)
#define B(s, sp) IT(1, s, sp)
#define B2(s, sp) IT2(1, s, sp)
#define C(s, sp) IT(2, s, sp)
#define C2(s, sp) IT2(2, s, sp)

#define PUSH(x, s, sp)   (s[(*sp)++] = x)
#define PUSH2(x, s, sp)  { PUSH((x)>>8, s, sp); PUSH((x), s, sp); }
#define Pto(v, ST, SP)                          \
  { if (SHORTP(op))                             \
      PUSH2(v, ST, SP)                          \
      else                                      \
        PUSH(v, ST, SP); }
#define P(v, op, uxn) Pto(v, STACK(op, uxn), STACKPT(op, uxn))
#define STACK(op, uxn)   (RETP(op) ? uxn->rst : uxn->wst)
#define STACKPT(op, uxn) (RETP(op) ? &uxn->rstp : &uxn->wstp)

#define STACKS(op, uxn, ST, SP) u8int *ST = STACK(op, uxn), *SP = STACKPT(op, uxn);

#define LIT(op, uxn)                            \
  { STACKS(op, uxn, ST, SP);                    \
    if (SHORTP(op)) {                           \
      PUSH(uxn->mem[uxn->pc+1], ST, SP);        \
      PUSH(uxn->mem[uxn->pc+2], ST, SP);        \
      NEXT(3);                                  \
    } else {                                    \
      PUSH(uxn->mem[uxn->pc+1], ST, SP);        \
      NEXT(2); }}

#define UN(op, sp, n) if (!KEEPP(op)) (*sp)-=n;

#define OPCn(n, op, uxn, code)                  \
  { STACKS(op, uxn, ST, SP);                    \
    u16int RES = 0;                             \
    code;                                       \
    UN(op, SP, n);                              \
    if (SHORTP(op))                             \
      PUSH2(RES, ST, SP)                        \
    else                                        \
      PUSH(RES, ST, SP); };

#define PEEK(op, uxn) \
  (SHORTP(op) ? IT2(0, STACK(op, uxn), STACKPT(op, uxn)) : IT(0, STACK(op, uxn), STACKPT(op, uxn)))

#define PEEKb(op, uxn) \
  (SHORTP(op) ? IT2(1, STACK(op, uxn), STACKPT(op, uxn)) : IT(1, STACK(op, uxn), STACKPT(op, uxn)))

#define PEEKc(op, uxn) \
  (SHORTP(op) ? IT2(2, STACK(op, uxn), STACKPT(op, uxn)) : IT(2, STACK(op, uxn), STACKPT(op, uxn)))

void
usage(void)
{
  fprint(2, "usage: %s file.rom\n", argv0);
  sysfatal("usage");
}

void
print_stack(char *name, u8int *st, u8int p)
{
  u16int i = 0;
  for (; i < p; ++i)
    print("%02x ", st[i]);
  print("|(%d) %s\n", p, name);
}

void
print_stacks(Uxn *uxn)
{
  print_stack("WST", uxn->wst, uxn->wstp);
  print_stack("RST", uxn->rst, uxn->rstp);
}

#define NEXT(n) {uxn->pc += n; break;}
#define JMP(n) {uxn->pc = n; break;}
#define IMM2(uxn) (uxn->mem[uxn->pc+1]<<8|uxn->mem[uxn->pc+2])

#define BINOP(B, op, uxn) { OPCn(2+(2*SHORTP(op)), op, uxn, { RES = PEEKb(op, uxn) B PEEK(op, uxn); }); NEXT(1) };
#define OP8(B, op, uxn) {u16int a = PEEK(op, uxn), b = PEEKb(op, uxn); UN(op, STACKPT(op, uxn), 2+(2*SHORTP(op))); PUSH(a B b, STACK(op, uxn), STACKPT(op, uxn)); NEXT(1);}

void
vm(Uxn *uxn)
{
  u8int op;
  while (1) {
    /* print_stacks(uxn); */
    op = uxn->mem[uxn->pc];
    // print_stacks(uxn);
    // print("[%04x] OPC: %02x, opcod: %02x\n", uxn->pc, op&0x1f, op);
    switch (op) { /* IMMEDIATE dispatch */
    case 0x20: {u8int a = IT(0, STACK(op, uxn), STACKPT(op, uxn)); UN(op, STACKPT(op, uxn), 1); if (!a) NEXT(3); /* else fallthrough */}
    case 0x40: {JMP(IMM2(uxn));}
    case 0x60: {u16int pt = IMM2(uxn); PUSH2(pt, uxn->rst, &uxn->rstp); JMP(pt);}
    /* LIT handled later */
    }
    switch (op&0x1f) {
    case 0x00: if (KEEPP(op)) LIT(op, uxn) else return;
    case 0x01: OPCn(1+SHORTP(op), op, uxn, { RES = PEEK(op, uxn) + 1; }); NEXT(1);
    case 0x02: if (KEEPP(op)) { /* NOOP */ NEXT(1); } else { UN(op, STACKPT(op, uxn), 1+SHORTP(op)); NEXT(1); }
      /* TODO: collapse 03 and 04 into one */
    case 0x03: {u16int a = PEEK(op, uxn)/*, b = PEEKb(op, uxn)*/; UN(op, STACKPT(op, uxn), 2+(2*SHORTP(op))); P(a, op, uxn); NEXT(1); }
    case 0x04: {u16int a = PEEK(op, uxn), b = PEEKb(op, uxn); UN(op, STACKPT(op, uxn), 2+(2*SHORTP(op))); P(a, op, uxn); P(b, op, uxn); NEXT(1); }
    case 0x05: {u16int c = PEEK(op, uxn), b = PEEKb(op, uxn), a = PEEKc(op, uxn);
        UN(op, STACKPT(op, uxn), 3+(3*SHORTP(op))); P(b, op, uxn); P(c, op, uxn); P(a, op, uxn); NEXT(1); }
    case 0x06: {u16int a = PEEK(op, uxn); UN(op, STACKPT(op, uxn), (2*SHORTP(op))); P(a, op, uxn); P(a, op, uxn); NEXT(1); }
    case 0x07: {u16int b = PEEK(op, uxn), a = PEEKb(op, uxn); UN(op, STACKPT(op, uxn), 2+(2*SHORTP(op))); P(a, op, uxn); P(b, op, uxn); P(a, op, uxn); NEXT(1); }
    case 0x08: OP8(==, op, uxn);
    case 0x09: OP8(!=, op, uxn);
    case 0x0a: OP8(<, op, uxn); /* reverse order of args */
    case 0x0b: OP8(>, op, uxn);
    case 0x0c: {u16int v = PEEK(op, uxn); UN(op, STACKPT(op, uxn), 1+SHORTP(op)); if (SHORTP(op)) JMP(v) else NEXT(v); }
    case 0x0d: {u16int pt = PEEK(op, uxn), cond = IT(SHORTP(op) ? 3 : 2, STACK(op, uxn), STACKPT(op, uxn)); UN(op, STACKPT(op, uxn), 2+SHORTP(op));
        /* print("pt: %04x\n", pt); */
        /* print("cond: %02x\n", cond); */
        if (cond) { if (SHORTP(op)) JMP(pt) else NEXT(pt); } NEXT(1); }
    case 0x0e: {u16int v = PEEK(op, uxn); UN(op, STACKPT(op, uxn), 1+SHORTP(op)); PUSH2(uxn->pc+1, uxn->rst, &uxn->rstp); /*print("will jmp to %04x\n", v);*/ if (SHORTP(op)) JMP(v) else NEXT(v);}
    case 0x0f: {u16int v = PEEK(op, uxn); UN(op, STACKPT(op, uxn), 1+SHORTP(op)); Pto(v, (RETP(op)?uxn->wst:uxn->rst), (RETP(op)?&uxn->wstp:&uxn->rstp)); NEXT(1);}
    case 0x10: {u8int a = IT(0, STACK(op, uxn), STACKPT(op, uxn)); UN(op, STACKPT(op, uxn), 1);
        u16int v = SHORTP(op) ? uxn->mem[a]<<8|uxn->mem[a+1] : uxn->mem[a];
        P(v, op, uxn); NEXT(1); }
    case 0x11: {u8int pt = IT(0, STACK(op, uxn), STACKPT(op, uxn));
        u16int v = IT2_(0, STACK(op, uxn), STACKPT(op, uxn));
        UN(op, STACKPT(op, uxn), 2+SHORTP(op));
        if (SHORTP(op)) uxn->mem[pt] = v>>8, uxn->mem[pt+1] = v; else uxn->mem[pt] = v;
        NEXT(1); }
    case 0x12: {s8int pt = IT(0, STACK(op, uxn), STACKPT(op, uxn)); UN(op, STACKPT(op, uxn), 1);
        if (SHORTP(op)) { PUSH2(uxn->mem[uxn->pc+pt]<<8|uxn->mem[uxn->pc+pt+1], STACK(op, uxn), STACKPT(op, uxn)) }
        else { PUSH(uxn->mem[uxn->pc+pt], STACK(op, uxn), STACKPT(op, uxn)); } NEXT(1);}
    case 0x13: {s8int addr = IT(0, STACK(op, uxn), STACKPT(op, uxn)); u16int val = SHORTP(op) ? IT2_(0, STACK(op, uxn), STACKPT(op, uxn)) : IT(1, STACK(op, uxn), STACKPT(op, uxn));
        UN(op, STACKPT(op, uxn), 2+SHORTP(op));
        if (SHORTP(op)) {
          uxn->mem[uxn->pc+addr] = val>>8;
          uxn->mem[uxn->pc+addr+1] = val;
        } uxn->mem[uxn->pc+addr] = val;
        NEXT(1);}
    case 0x14: {u16int a = IT2(0, STACK(op, uxn), STACKPT(op, uxn)); UN(op, STACKPT(op, uxn), 2);
        u16int v = SHORTP(op) ? uxn->mem[a]<<8|uxn->mem[a+1] : uxn->mem[a];
        P(v, op, uxn); NEXT(1); }
    case 0x15: {u16int pt = IT2(0, STACK(op, uxn), STACKPT(op, uxn)), v = IT2(1, STACK(op, uxn), STACKPT(op, uxn)); UN(op, STACKPT(op, uxn), 3+SHORTP(op));
        if (SHORTP(op)) uxn->mem[pt] = v>>8, uxn->mem[pt+1] = v; else uxn->mem[pt] = v;
        NEXT(1); }
    case 0x16: {u16int dev = IT(0, STACK(op, uxn), STACKPT(op, uxn)), res;
        if (uxn->devices[dev]) res = uxn->devices[dev](0xffff); else res = 0;
        UN(op, STACKPT(op, uxn), 1);
        P(res, op, uxn);
        NEXT(1);}
    case 0x17: {u16int dev = IT(0, STACK(op, uxn), STACKPT(op, uxn)), val = SHORTP(op) ? IT2_(0, STACK(op, uxn), STACKPT(op, uxn)) : IT(1, STACK(op, uxn), STACKPT(op, uxn));
        if (uxn->devices[dev]) uxn->devices[dev](val); /*else sysfatal("unknown device %02x", dev); */
        UN(op, STACKPT(op, uxn), 2+SHORTP(op));
        NEXT(1);}
    case 0x18: BINOP(+, op, uxn);
    case 0x19: BINOP(-, op, uxn);
    case 0x1a: BINOP(*, op, uxn);
    case 0x1b: BINOP(/, op, uxn);
    case 0x1c: BINOP(&, op, uxn);
    case 0x1d: BINOP(|, op, uxn);
    case 0x1e: BINOP(^, op, uxn);
    case 0x1f: {
      u8int sft = IT(0, STACK(op, uxn), STACKPT(op, uxn));
      u16int v = IT2_(0, STACK(op, uxn), STACKPT(op, uxn));
      if (!SHORTP(op)) v = v&0xff;
      UN(op, STACKPT(op, uxn), 2+SHORTP(op));
      P((v>>(0x0f&sft))<<((0xf0&sft)>>4), op, uxn);
      NEXT(1); }
    default: sysfatal("unimplemented opcode: %02x", op&0x1f);
    }
  }
}

void
main(int argc, char **argv)
{
  Uxn uxn = {0};
  u8int mem[1<<16] = {0};
  int fd;

  ARGBEGIN {
    default: usage();
  } ARGEND;

  if (argc != 1) usage();

  fd = open(argv[0], OREAD);
  if (fd < 0)
    sysfatal("couldn't read file %s", argv[0]);
  int rd = read(fd, mem+0x100, (1<<16)-0x100);
  print("read %d bytes\n", rd);
  close(fd);

  init_console_device(&uxn);
  init_screen_device(&uxn);

  uxn.mem = mem;
  uxn.pc = 0x100;

  vm(&uxn);
  screen_main_loop(&uxn);

  exits(nil);
}
