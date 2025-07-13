#include "uxn9.h"

#include <draw.h>
#include <event.h>
#include <keyboard.h>

#define SCREEN_VECTOR 0x20
#define SCREEN_WIDTH  0x22
#define SCREEN_HEIGHT 0x24
#define SCREEN_AUTO   0x26
#define SCREEN_X      0x28
#define SCREEN_Y      0x2a
#define SCREEN_ADDR   0x2c
#define SCREEN_PIXEL  0x2e
#define SCREEN_SPRITE 0x2f

blending[4][16] = {
 {0, 0, 0, 0, 1, 0, 1, 1, 2, 2, 0, 2, 3, 3, 3, 0},
 {0, 1, 2, 3, 0, 1, 2, 3, 0, 1, 2, 3, 0, 1, 2, 3},
 {1, 2, 3, 1, 1, 2, 3, 1, 1, 2, 3, 1, 1, 2, 3, 1},
 {2, 3, 1, 2, 2, 3, 1, 2, 2, 3, 1, 2, 2, 3, 1, 2}};

#define BLEND(color, c) blending[color][c]

static Uxn *current_uxn = nil;

static u16int screen_vector = 0;
static u16int screen_addr = 0;

static u16int window_width = 400;
static u16int window_height = 300;
static u16int current_x = 0;
static u16int current_y = 0;

static int wctl_fd = 0;

static u8int *fg = 0;
static u8int *bg = 0;

static u8int *sbuf = 0;
static Image *img = nil;

u8int colors[] = {
  /*B   G     R */
  0x00, 0x00, 0x00,
  0xff, 0xff, 0xff,
  0x00, 0x00, 0x00,
  0xff, 0xff, 0xff,
};

/* draw bg & fg onto sbuf */
static void
update_screen_buffer(void)
{
  uint i, j, pt;
  u8int sel;
  for (i = 0; i < window_height; ++i) {
    for (j = 0; j < window_width; ++j) {
      pt = (i*window_width)+j;
      if (fg[pt]) sel = fg[pt]; else sel = bg[pt];

      sbuf[pt*3] = colors[sel*3];
      sbuf[pt*3 + 1] = colors[sel*3 + 1];
      sbuf[pt*3 + 2] = colors[sel*3 + 2];
    }
  }
}

static void
redraw(void)
{
  uint xr = screen->r.min.x,
       yr = screen->r.min.y;

  update_screen_buffer();

  if (loadimage(img, img->r, (void*)sbuf, window_width*window_height*3) < 0)
    sysfatal("loadimage");

  //drawop(screen, Rect(xr, yr, w+xr, h+yr), i, nil, i->r.min, 0);
  //draw(screen, Rect(xr, yr, window_width+xr, window_height+yr), img, nil, img->r.min);
  draw(screen, screen->r, img, nil, ZP);
  flushimage(display, 1);
}

static void
update_window_size(void)
{
  int cx = display->image->r.max.x/2;
  int cy = display->image->r.max.y/2;
  int x1 = cx - window_width/2;
  int y1 = cy - window_height/2;
  Rectangle r;
  // TODO: uncomment this
  fprint(wctl_fd, "resize -r %d %d %d %d\n", x1, y1, x1+window_width, y1+window_height);
  while (getwindow(display, Refnone) != 1)
    ;

  if (sbuf) free(sbuf);
  if (fg)   free(fg);
  if (bg)   free(bg);

  sbuf = malloc(window_width*window_height*3);
  bg = malloc(window_width*window_height);
  fg = malloc(window_width*window_height);

  memset(sbuf, 0, window_width*window_height*3);
  memset(bg, 0, window_width*window_height);
  memset(fg, 0, window_width*window_height);

  r = Rect(0, 0, window_width, window_height);
  if (img)
    freeimage(img);
  img = allocimage(display, r, RGB24, 0, 0);
  if (img == nil)
    sysfatal("allocimage");

  redraw();
}

void
eresized(int new)
{
  while (getwindow(display, Refnone) != 0)
    ;
  update_window_size();
}


static u16int
screen_set_width(u8int getp, u16int w)
{
  if (getp)
    return window_width;
  window_width = w;
  update_window_size();
  return w;
}

static u16int
screen_set_height(u8int getp, u16int h)
{
  if (getp)
    return window_height;
  window_height = h;
  update_window_size();
  return h;
}

static u16int
screen_set_x(u8int getp, u16int x)
{
  if (getp)
    return current_x;
  current_x = x;
  return x;
}

static u16int
screen_set_y(u8int getp, u16int y)
{
  if (getp)
    return current_y;
  current_y = y;
  return y;
}

static u16int
screen_set_vec(u8int getp, u16int vec)
{
  if (getp)
    return screen_vector;
  screen_vector = vec;
  return vec;
}

static u16int
screen_set_addr(u8int getp, u16int addr)
{
  if (getp)
    return screen_addr;
  screen_addr = addr;
  return addr;
}

// 2bpp layer flipy flipx 3 2 1 0
//                        \_____/- color

static u16int
screen_sprite(u8int getp, u16int dat)
{
  if (getp)
    return 0xff;
  uint i, j;
  u8int _2bpp = 0b10000000&dat,
        layer = 0b1000000&dat,
        flipyp = 0b100000&dat,
        flipxp = 0b10000&dat,
        color = 0b1111&dat,
        *target = layer ? fg : bg,
        cur;

  if (_2bpp)
    sysfatal("2 bit per pixel not implemented");
  if (flipxp)
    sysfatal("flipx not implemented");
  if (flipyp)
    sysfatal("flipy not implemented");

  /* 1bpp */
  for (i = 0; i < 8; ++i) {
    if (current_y + i >= window_height) break;
    for (j = 0; j < 8; ++j) {
      if (current_x + j >= window_width) break;
      cur = ((current_uxn->mem[screen_addr + i])>>7-j)&0b1;
      target[((current_y+i)*window_width)+current_x+j] = BLEND(cur, color);
    }
  }
  return 0;
}

// fill layer flipy flipx 3 2 1 0
//                            \_/- color

static u16int
screen_pixel(u8int getp, u16int dat)
{
  if (getp)
    return 0xff;
  int i, j;
  u8int *target,
        color = 0b11&dat,
        fillp = 0b10000000&dat,
        layer = 0b1000000&dat,
        flipxp = 0b100000&dat,
        flipyp = 0b10000&dat;

  target = layer ? fg : bg;

  if (fillp) {
    for (i = current_y; flipyp ? i >=0 : i < window_height; flipyp ? i-- : i++)
      for (j = current_x; flipxp ? j >=0 : j < window_width; flipxp? j-- : j++)
        target[(i*window_width)+j] = color;
  } else
    target[(current_y*window_width)+current_x] = color;

  return dat;
}

void
init_screen_device(Uxn *uxn)
{
  current_uxn = uxn;
  initdraw(nil, nil, argv0);
  einit(Emouse|Ekeyboard);
  wctl_fd = open("/dev/wctl", OREAD|OWRITE);
  if (wctl_fd < 0)
    sysfatal("wctl");
  update_window_size();
  redraw();

  uxn->devices[SCREEN_VECTOR] = screen_set_vec;
  uxn->devices[SCREEN_WIDTH ] = screen_set_width;
  uxn->devices[SCREEN_HEIGHT] = screen_set_height;
  uxn->devices[SCREEN_AUTO  ] = 0;
  uxn->devices[SCREEN_X     ] = screen_set_x;
  uxn->devices[SCREEN_Y     ] = screen_set_y;
  uxn->devices[SCREEN_ADDR  ] = screen_set_addr;
  uxn->devices[SCREEN_PIXEL ] = screen_pixel;
  uxn->devices[SCREEN_SPRITE] = screen_sprite;
}

void
screen_main_loop(Uxn *uxn)
{
  u16int run = 0x0fff;
  current_uxn = uxn;

  vlong target_timeout_ns = 1000000000/60, was, is;

  while (run--) {
    was = nsec();
    uxn->pc = screen_vector;
    vm(uxn);
    redraw();
    is = nsec();
    if (is-was < target_timeout_ns)
      sleep((target_timeout_ns - (is-was))/1000000);
  }
}
