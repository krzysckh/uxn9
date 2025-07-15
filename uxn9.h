#ifndef __UXN9_H
#define __UXN9_H

// #define UXN9_FATAL_NODEVICE /* sorry */
#define TARGET_FPS 60

#include <u.h>
#include <libc.h>
#include <draw.h>
#include <cursor.h>
#include <thread.h>
#include <mouse.h>
#include <keyboard.h>

typedef u16int (*Dev)(u8int, u16int);

typedef struct Uxn
{
  Lock l;
  u16int pc;
  u8int *mem, rst[0x100], wst[0x100], rstp, wstp;
  Dev devices[0x100];
} Uxn;

void vm(Uxn *);

/* system */
void init_system_device(Uxn *);

/* console */
void init_console_device(Uxn *);

/* screen */
void init_screen_device(Uxn *);
void screen_main_loop(Uxn *);

/* file */
void init_file_device(Uxn *);

/* mouse */
void init_mouse_device(Uxn *);
void update_mouse_state(int);

/* datetime */
void init_datetime_device(Uxn *);

/* controller */
void init_controller_device(Uxn *);
void btn_thread(void *);

#endif /* __UXN9_H */
