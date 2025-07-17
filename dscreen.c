#include "uxn9.h"

#define INITIAL_WINDOW_WIDTH  (64*8)
#define INITIAL_WINDOW_HEIGHT (40*8)

#define MIN(a,b) ((a)<(b)?(a):(b))
#define MAX(a,b) ((a)>(b)?(a):(b))

#define BLEND(color, c) blending[color][c]

#define _window_width  (DEV2(SCREEN_WIDTH))
#define _window_height (DEV2(SCREEN_HEIGHT))
#define WINSIZES u16int window_width = _window_width, window_height = _window_height

#define autoN ((DEV(SCREEN_AUTO)&0xf0)>>4)
#define autoX (DEV(SCREEN_AUTO)&1)
#define autoY (DEV(SCREEN_AUTO)&(1<<1))
#define autoA (DEV(SCREEN_AUTO)&(1<<2))

int blending[4][16] = {
  {0, 0, 0, 0, 1, 0, 1, 1, 2, 2, 0, 2, 3, 3, 3, 0},
  {0, 1, 2, 3, 0, 1, 2, 3, 0, 1, 2, 3, 0, 1, 2, 3},
  {1, 2, 3, 1, 1, 2, 3, 1, 1, 2, 3, 1, 1, 2, 3, 1},
  {2, 3, 1, 2, 2, 3, 1, 2, 2, 3, 1, 2, 2, 3, 1, 2}};

uint TARGET_FPS = 60;
uint DEBUG_SMART_DRAWING = 0;

uint resized = 0, SX = 0, SY = 0;

static int wctl_fd = 0;

#ifdef USE_SMART_DRAWING
static u16int miny = 0xffff, minx = 0xffff, maxx = 0, maxy = 0;
#endif // USE_SMART_DRAWING

static u8int *fg = nil;
static u8int *bg = nil;
static u8int *sbuf = nil; /* data buffer */
static Image *img = nil;

static int resizeskip = 0;

static Keyboardctl *keyboard = nil;

u8int colors[] = {
  /*B   G     R */
  0x00, 0x00, 0x00,
  0xff, 0xff, 0xff,
  0x00, 0x00, 0x00,
  0xff, 0xff, 0xff,
};

/* draw bg & fg onto sbuf */
static void
update_screen_buffer(Uxn *uxn)
{
  uint i, j, pt;
  u8int sel;
  WINSIZES;

  if (DEBUG_SMART_DRAWING)
    memset(sbuf, 0, window_width*window_height*3);

#ifdef USE_SMART_DRAWING
  for (i = MAX(0, miny); i < MIN(window_height, maxy+1); ++i) {
    for (j = MAX(0, minx); j < MIN(window_width, maxx+1); ++j) {
#else
  for (i = 0; i < window_height; ++i) {
    for (j = 0; j < window_width; ++j) {
#endif // USE_SMART_DRAWING
      pt = (i*window_width)+j;
      if (fg[pt]) sel = fg[pt]; else sel = bg[pt];

      if (DEBUG_SMART_DRAWING)
        sel = 2;

      sbuf[pt*3]     = colors[sel*3];
      sbuf[pt*3 + 1] = colors[sel*3 + 1];
      sbuf[pt*3 + 2] = colors[sel*3 + 2];
    }
  }

#ifdef USE_SMART_DRAWING
  miny = minx = 0xffff, maxx = maxy = 0;
#endif
}

static void
redraw(Uxn *uxn)
{
  uint xr = screen->r.min.x,
       yr = screen->r.min.y;
  WINSIZES;

  update_screen_buffer(uxn);

  if (loadimage(img, img->r, (void*)sbuf, window_width*window_height*3) < 0)
    sysfatal("loadimage");

  //drawop(screen, Rect(xr, yr, w+xr, h+yr), i, nil, i->r.min, 0);
  //draw(screen, Rect(xr, yr, window_width+xr, window_height+yr), img, nil, img->r.min);
  draw(screen, screen->r, img, nil, ZP);
  flushimage(display, 1);
}

static void
update_window_size(Uxn *uxn)
{
  Rectangle r;
  WINSIZES;

  while (getwindow(display, Refnone) != 1)
    ;

  SX = screen->r.min.x, SY = screen->r.min.y; // <- this sucks

  r = Rect(0, 0, window_width, window_height);
  if (img)
    freeimage(img);
  img = allocimage(display, r, RGB24, 0, 0);
  if (img == nil)
    sysfatal("allocimage");

  // redraw();
}

static void
set_new_window_size(Uxn *uxn)
{
  uint xr = screen->r.min.x,
       yr = screen->r.min.y;
  WINSIZES;

  // print("new window size: %dx%d\n", window_width, window_height);

  if (wctl_fd)
    fprint(wctl_fd, "resize -r %d %d %d %d\n", xr, yr, xr+window_width, yr+window_height);

  if (sbuf != nil) free(sbuf);
  if (fg != nil)   free(fg);
  if (bg != nil)   free(bg);

  sbuf = mallocz(window_width*window_height*3, 1);
  bg   = mallocz(window_width*window_height, 1);
  fg   = mallocz(window_width*window_height, 1);

  if (fg == nil || bg == nil || sbuf == nil)
    sysfatal("malloc");
}

static void
center_window(Uxn *uxn)
{
  WINSIZES;
  int cx = display->image->r.max.x/2;
  int cy = display->image->r.max.y/2;
  int x1 = cx - window_width/2;
  int y1 = cy - window_height/2;
  fprint(wctl_fd, "resize -r %d %d %d %d\n", x1, y1, x1+window_width, y1+window_height);

  //update_window_size();
}

static void
screen_update_size(Uxn *uxn)
{
  set_new_window_size(uxn);
#ifdef USE_SMART_DRAWING
  minx = miny = 0, maxx = maxy = 0xffff;
#endif
}

// 2bpp layer flipy flipx 3 2 1 0
//                        \_____/- color

static void
screen_sprite(Uxn *uxn)
{
  s32int i, j, A;
  u16int tx, ty, x = DEV2(SCREEN_X), y = DEV2(SCREEN_Y), xwas = x, ywas = y;
  u16int addr = DEV2(SCREEN_ADDR);
  u8int dat     = DEV(SCREEN_SPRITE),
        _2bpp   = 0b10000000&dat,
        layer   = 0b1000000&dat,
        flipyp  = 0b100000&dat,
        flipxp  = 0b10000&dat,
        color   = 0b1111&dat,
        *target = layer ? fg : bg,
        cur;
  WINSIZES;

  /*
  if (autoX && autoY)
    sysfatal("autoX and autoY");
    */

  for (A = 0; A < autoN + 1; ++A) {
    /* draw sprite */
    for (i = 0; i < 8; ++i) {
      for (j = 0; j < 8; ++j) {
        if (_2bpp)
          cur = (((uxn->mem[addr + i]>>(flipxp?j:7-j))&1))|(((uxn->mem[addr + i + 8]>>(flipxp?j:7-j))&1)<<1);
        else
          cur = ((uxn->mem[addr + i])>>(flipxp?j:7-j))&0b1;
        tx = x+j, ty = (y+(flipyp?7-i:i));
        if (ty < window_height && tx < window_width)
          if (cur || color % 5) /* why */
            target[ty*window_width + tx] = BLEND(cur, color);
      }
    }
    /* end draw sprite */

    if (autoX) y = flipyp? y-8 : y+8;
    if (autoY) x = flipxp? x-8 : x+8;
    if (autoA) addr += _2bpp ? 16 : 8;
  }

#ifdef USE_SMART_DRAWING
    minx = MIN(xwas, MIN(minx, tx)), maxx = MAX(xwas, MAX(maxx, tx));
    miny = MIN(ywas, MIN(miny, ty)), maxy = MAX(ywas, MAX(maxy, ty));
#endif


  if (autoX) SDEV2(SCREEN_X, flipxp ? x-8 : x+8);
  if (autoY) SDEV2(SCREEN_Y, flipyp ? y-8 : y+8);
  if (autoA) SDEV2(SCREEN_ADDR, addr);
}

// fill layer flipy flipx 3 2 1 0
//                            \_/- color

static void
screen_pixel(Uxn *uxn)
{
  u8int dat     = DEV(SCREEN_PIXEL),
        color   = 0b11&dat,
        fillp   = 0b10000000&dat,
        layer   = 0b1000000&dat,
        flipxp  = 0b100000&dat,
        flipyp  = 0b10000&dat,
        *target = layer ? fg : bg ;
  WINSIZES;
  u16int x = DEV2(SCREEN_X), y = DEV2(SCREEN_Y);
  int i = x, j = y;

  if (fillp) {
    for (; flipyp ? i >=0 : i < window_height; flipyp ? i-- : i++)
      for (j = x; flipxp ? j >=0 : j < window_width; flipxp? j-- : j++)
        target[(i*window_width)+j] = color;
  } else
    target[(y*window_width)+x] = color;

  if (autoX) SDEV2(SCREEN_X, x+1);
  if (autoY) SDEV2(SCREEN_Y, y+1);

#ifdef USE_SMART_DRAWING
  minx = 0, miny = 0, maxx = 0xffff, maxy = 0xffff;
#endif
}

int do_exit = 0;

void
exitall(char *mesg)
{
  do_exit = 1;
  // postnote(PNPROC, threadpid(pidb), "shutdown");
  // postnote(PNPROC, threadpid(pidm), "shutdown");
  threadexitsall(mesg);
}

void
screen_main_loop(Uxn *uxn)
{
  u16int run = 0x00ff;
  vlong target_timeout_ns = 1000000000/TARGET_FPS, was, slp;

  center_window(uxn);
  update_window_size(uxn);

  while (do_exit == 0) {
    lock(&uxn->l);

    if (resized) {
      update_window_size(uxn);
      resized = 0;
    }
    was = nsec();
    uxn->pc = DEV2(SCREEN_VECTOR);
    vm(uxn);
    unlock(&uxn->l);
    redraw(uxn);

    slp = target_timeout_ns - (nsec() - was);
    if (slp)
      sleep(slp/1000000);
  }
}

void
init_screen_device(Uxn *uxn)
{
  SDEV2(SCREEN_WIDTH, INITIAL_WINDOW_WIDTH);
  SDEV2(SCREEN_HEIGHT, INITIAL_WINDOW_HEIGHT);

  wctl_fd = open("/dev/wctl", OREAD|OWRITE);
  if (wctl_fd < 0)
    sysfatal("wctl");

  if (initdraw(nil, nil, argv0) < 0)
    sysfatal("initdraw: %r");

  set_new_window_size(uxn);

  uxn->trigo[SCREEN_WIDTH ] = screen_update_size;
  uxn->trigo[SCREEN_HEIGHT] = screen_update_size;
  uxn->trigo[SCREEN_PIXEL ] = screen_pixel;
  uxn->trigo[SCREEN_SPRITE] = screen_sprite;
}
