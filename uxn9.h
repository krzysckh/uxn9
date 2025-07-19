#ifndef __UXN9_H
#define __UXN9_H

#define USE_SMART_DRAWING

#include <u.h>
#include <libc.h>
#include <draw.h>
#include <cursor.h>
#include <thread.h>
#include <mouse.h>
#include <keyboard.h>

typedef struct Uxn Uxn;
typedef void (*Dev)(Uxn*);

struct Uxn
{
  Lock l;
  u16int pc;
  u8int *mem, rst[0x100], wst[0x100], rstp, wstp;
  Dev   trigi[0x100]; // trigger in
  Dev   trigo[0x100]; // trigger out
  u8int dev[0x100];
};

void vm(Uxn *);

/* varvara devices */

/* system */

#define SYSTEM_EXPANSION 0x02
#define SYSTEM_WST       0x04
#define SYSTEM_RST       0x05
#define SYSTEM_METADATA  0x06
#define SYSTEM_RED       0x08
#define SYSTEM_GREEN     0x0a
#define SYSTEM_BLUE      0x0c
#define SYSTEM_DEBUG     0x0e
#define SYSTEM_STATE     0x0f

void init_system_device(Uxn *);

/* console */
#define CONSOLE_VECTOR 0x10
#define CONSOLE_READ   0x12
#define CONSOLE_TYPE   0x17
#define CONSOLE_WRITE  0x18
#define CONSOLE_ERROR  0x19

void init_console_device(Uxn *);
void console_main_loop(void *);

/* screen */

#define SCREEN_VECTOR 0x20
#define SCREEN_WIDTH  0x22
#define SCREEN_HEIGHT 0x24
#define SCREEN_AUTO   0x26
#define SCREEN_X      0x28
#define SCREEN_Y      0x2a
#define SCREEN_ADDR   0x2c
#define SCREEN_PIXEL  0x2e
#define SCREEN_SPRITE 0x2f

void init_screen_device(Uxn *);
void screen_main_loop(Uxn *);
void exitall(char *);
extern uint TARGET_FPS;
extern uint DEBUG_SMART_DRAWING;

/* file */

#define FILE_SUCCESS 0xa2
#define FILE_STAT    0xa4
#define FILE_DELETE  0xa6
#define FILE_APPEND  0xa7
#define FILE_NAME    0xa8
#define FILE_LENGTH  0xaa
#define FILE_READ    0xac
#define FILE_WRITE   0xae

void init_file_device(Uxn *);
void close_file_device(Uxn *);

/* mouse */

#define MOUSE_VECTOR  0x90
#define MOUSE_X       0x92
#define MOUSE_Y       0x94
#define MOUSE_STATE   0x96
#define MOUSE_SCROLLX 0x9a
#define MOUSE_SCROLLY 0x9c

void init_mouse_device(Uxn *);
void mouse_thread(void *);

/* datetime */

#define DATETIME_YEAR   0xc0
#define DATETIME_MONTH  0xc2
#define DATETIME_DAY    0xc3
#define DATETIME_HOUR   0xc4
#define DATETIME_MINUTE 0xc5
#define DATETIME_SECOND 0xc6
#define DATETIME_DOTW   0xc7
#define DATETIME_DOTY   0xc8
#define DATETIME_ISDST  0xca

void init_datetime_device(Uxn *);

/* controller */

#define CONTROLLER_VECTOR 0x80
#define CONTROLLER_BUTTON 0x82
#define CONTROLLER_KEY    0x83

void init_controller_device(Uxn *);
void btn_thread(void *);

/* end devices */

#define DEV(x) (uxn->dev[x])
#define DEV2(x) ((uxn->dev[x]<<8)|uxn->dev[((x)+1)&0xff])

#define SDEV(x, y) uxn->dev[x] = (y)
#define SDEV2(x, y) {u16int v = y; uxn->dev[x] = ((v)>>8), uxn->dev[((x)+1)&0xff] = ((v)&0xff);}

#endif /* __UXN9_H */
