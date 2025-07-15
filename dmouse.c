#include "uxn9.h"

#define MOUSE_VECTOR 0x90
#define MOUSE_X      0x92
#define MOUSE_Y      0x94
#define MOUSE_STATE  0x96

u16int mouse_vector = 0;
u8int mouse_state = 0;

extern Mousectl *mouse;

static u16int
mouse_x(u8int getp, u16int dat)
{
  USED(dat);
  if (getp)
    return mouse->xy.x - screen->r.min.x;
  return 0;
}

static u16int
mouse_y(u8int getp, u16int dat)
{
  USED(dat);
  if (getp)
    return mouse->xy.y - screen->r.min.y;
  return 0;
}

DEFGETSET(set_mouse_vector, mouse_vector);

static u16int
get_mouse_state(u8int getp, u16int dat)
{
  USED(getp, dat);
  return mouse_state;
}

void
init_mouse_device(Uxn *uxn)
{
  uxn->devices[MOUSE_X     ] = mouse_x;
  uxn->devices[MOUSE_Y     ] = mouse_y;
  uxn->devices[MOUSE_VECTOR] = set_mouse_vector;
  uxn->devices[MOUSE_STATE ] = get_mouse_state;
}
