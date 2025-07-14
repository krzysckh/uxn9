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
