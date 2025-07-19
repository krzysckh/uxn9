#include "uxn9.h"

extern uint resized, SX, SY;
extern int do_exit;
int mouse = -1, cursor = -1;

void
mouse_thread(void *arg)
{
  static Cursor c;
  s8int buf[1 + 5*12];
  Uxn *uxn = (Uxn *)arg;
  uint state;
  int n;

  mouse = open("/dev/mouse", ORDWR);
  cursor = open("/dev/cursor", ORDWR);

  if (mouse < 0 || cursor < 0)
    sysfatal("mouse or cursor");

  write(cursor, &c, sizeof(c));

  while (!do_exit && (n = read(mouse, buf, 49))) {
    switch (*buf) {
    case 'r': resized = 1; /* fallthrough */
    case 'm':
      lock(&uxn->l);
      state = atoi(buf+1+2*12);
      if (state&(8|16)) {
        SDEV2(MOUSE_SCROLLX, 0);
        SDEV2(MOUSE_SCROLLY, (state&8) ? 1 : -1);
      } else {
        SDEV2(MOUSE_X, atoi(buf+1+0*12) - SX);
        SDEV2(MOUSE_Y, atoi(buf+1+1*12) - SY);
        SDEV(MOUSE_STATE, state);
      }
      if (DEV2(MOUSE_VECTOR)) {
        uxn->pc = DEV2(MOUSE_VECTOR);
        vm(uxn);
      }
      unlock(&uxn->l);
      break;
    default:
      print("uhh %s\n", buf);
    }
  }

  close(mouse);
  close(cursor);

  threadexitsall(nil);
}

void
init_mouse_device(Uxn *uxn)
{
  proccreate(mouse_thread, uxn, 1024);
}
