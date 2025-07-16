#include "uxn9.h"

extern Mousectl *mouse;

static void
mouse_x(Uxn *uxn)
{
  SDEV2(MOUSE_X, mouse->xy.x - screen->r.min.x);
}

static void
mouse_y(Uxn *uxn)
{
  SDEV2(MOUSE_Y, mouse->xy.y - screen->r.min.y);
}

void
init_mouse_device(Uxn *uxn)
{
  uxn->trigi[MOUSE_X] = mouse_x;
  uxn->trigi[MOUSE_Y] = mouse_y;
}
