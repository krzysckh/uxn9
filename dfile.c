#include "uxn9.h"

#define FILE_NAME 0x28

static u16int current_file_name = 0;

u16int
file_name(u8int getp, u16int dat)
{
  if (getp)
    return current_file_name;
  current_file_name = dat;
  return dat;
}

void
init_file_device(Uxn *uxn)
{
  /* not implemented */
}
