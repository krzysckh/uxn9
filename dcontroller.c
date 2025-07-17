#include "uxn9.h"

// ctrl
// alt
// shift
// home
// up
// down
// left
// right

extern int do_exit;

void
btn_thread(void *arg)
{
  Uxn *uxn = arg;
  Rune r;
  int fd = open("/dev/kbd", OREAD);
  s8int b[256] = {0}, *bv;
  u8int button = 0, bev = 0;
  int nread;
  if (fd < 0)
    sysfatal("devkbd");

  while (!do_exit) {
    if ((nread = read(fd, b, sizeof(b)-1))) {
      bv = b;
      lock(&uxn->l);
      bev = 0;
      b[nread] = 0;
      b[nread+1] = 0;
loop: while (*bv) {
        switch (*bv) {
        case 'c':
          if (utfrune(bv, Kdel)) exitall(nil);
          break;
        case 'k': /* keydown */
          bev = 1;
          bv++;
          while (*bv) {
            bv += chartorune(&r, bv);
            switch (r) {
            case Kctl:   bev = 1, button |= 1;      break;
            case Kalt:   bev = 1, button |= (1<<1); break;
            case Kshift: bev = 1, button |= (1<<2); break;
            case Khome:  bev = 1, button |= (1<<3); break;
            case Kup:    bev = 1, button |= (1<<4); break;
            case Kdown:  bev = 1, button |= (1<<5); break;
            case Kleft:  bev = 1, button |= (1<<6); break;
            case Kright: bev = 1, button |= (1<<7); break;
            default:
              if (r < 0x80) {
                // print("char: %c (%d)\n", r&0xff, r&0xff);
                SDEV(CONTROLLER_KEY, r&0xff);
                uxn->pc = DEV2(CONTROLLER_VECTOR);
                vm(uxn);
                SDEV(CONTROLLER_KEY, 0);
                break;
              }
            }
          }
          if (bev) {
            SDEV(CONTROLLER_BUTTON, button);
            uxn->pc = DEV2(CONTROLLER_VECTOR);
            vm(uxn);
          }
          goto loop;
        case 'K': /* keyup */
          button = DEV(CONTROLLER_KEY);
          bev = 1;
          bv++;
          while (*bv) {
            bv += chartorune(&r, bv);
            switch (r) {
            case Kctl:   button &= ~1;      break;
            case Kalt:   button &= ~(1<<1); break;
            case Kshift: button &= ~(1<<2); break;
            case Khome:  button &= ~(1<<3); break;
            case Kup:    button &= ~(1<<4); break;
            case Kdown:  button &= ~(1<<5); break;
            case Kleft:  button &= ~(1<<6); break;
            case Kright: button &= ~(1<<7); break;
            default:
              ;
            }
          }
          SDEV(CONTROLLER_BUTTON, button);
          uxn->pc = DEV2(CONTROLLER_VECTOR);
          vm(uxn);
          goto loop;
        default:
          ;
          // fprint(1, "unknown: %s\n", bv);
        }
        while (*bv++)
          ;
      }
      unlock(&uxn->l);
    }
  }

  close(fd);
  threadexitsall(nil);
}

// TODO: maybe move the start of btn_thread here
void
init_controller_device(Uxn *uxn)
{
  proccreate(btn_thread, uxn, 1024);

}
