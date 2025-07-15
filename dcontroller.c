#include "uxn9.h"

#define CONTROLLER_VECTOR 0x80
#define CONTROLLER_BUTTON 0x82
#define CONTROLLER_KEY    0x83

Rune current_rune = 0;
u16int controller_vector = 0;
u8int current_button = 0;

// ctrl
// alt
// shift
// home
// up
// down
// left
// right
//
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

  for (;;) {
    if ((nread = read(fd, b, sizeof(b)-1))) {
      bv = b;
      lock(&uxn->l);
      bev = 0;
      b[nread] = 0;
      b[nread+1] = 0;
loop: while (*bv) {
        switch (*bv) {
        case 'c':
          if (utfrune(bv, Kdel)) sysfatal("delete");
          break;
        case 'k':
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
              ;
                         /*
              if (runelen(r) == 1 && r > 32 && r < 127) {
                current_rune = r&0xff;
                print("RUNE! %02x\n", current_rune);
                uxn->pc = controller_vector;
                vm(uxn);
                current_rune = 0;
                break;
              }
              */
            }
          }
          if (bev) {
            current_button = button;
            uxn->pc = controller_vector;
            vm(uxn);
          }
          goto loop;
        case 'K':
          bv++;
          while (*bv) {
            bv += chartorune(&r, bv);
          }
          /*
          bev = 1;
          bv++;
          while (*bv) {
            bv += chartorune(&r, bv);
            switch (r) {
            case Kctl:   bev = 1, button &= ~1;      break;
            case Kalt:   bev = 1, button &= ~(1<<1); break;
            case Kshift: bev = 1, button &= ~(1<<2); break;
            case Khome:  bev = 1, button &= ~(1<<3); break;
            case Kup:    bev = 1, button &= ~(1<<4); break;
            case Kdown:  bev = 1, button &= ~(1<<5); break;
            case Kleft:  bev = 1, button &= ~(1<<6); break;
            case Kright: bev = 1, button &= ~(1<<7); break;
            default:
              ;
            }
          }
          */
          button = current_button = 0;
            //print("KEYUP %02x!", current_button);
          uxn->pc = controller_vector;
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
}


static u16int
button_state(u8int getp, u16int dat)
{
  USED(getp, dat);
  return current_button;
}

static u16int
key_state(u8int getp, u16int dat)
{
  USED(getp, dat);
  return current_rune;
}

static u16int
controller_vector_state(u8int getp, u16int dat)
{
  if (getp)
    return controller_vector;
  controller_vector = dat;
  return controller_vector;
}

void
init_controller_device(Uxn *uxn)
{
  uxn->devices[CONTROLLER_VECTOR] = controller_vector_state;
  uxn->devices[CONTROLLER_BUTTON] = button_state;
  uxn->devices[CONTROLLER_KEY   ] = key_state;
}
