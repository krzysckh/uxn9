#include "uxn9.h"

#define SCREEN_VECTOR 0x20
#define SCREEN_WIDTH  0x22
#define SCREEN_HEIGHT 0x24
#define SCREEN_AUTO   0x26
#define SCREEN_X      0x28
#define SCREEN_Y      0x2a
#define SCREEN_ADDR   0x2c
#define SCREEN_PIXEL  0x2e
#define SCREEN_SPRITE 0x2f

#define INITIAL_WINDOW_WIDTH (64*8)
#define INITIAL_WINDOW_HEIGHT (40*8)

#define MIN(a,b) ((a)<(b)?(a):(b))
#define MAX(a,b) ((a)>(b)?(a):(b))

int blending[4][16] = {
  {0, 0, 0, 0, 1, 0, 1, 1, 2, 2, 0, 2, 3, 3, 3, 0},
  {0, 1, 2, 3, 0, 1, 2, 3, 0, 1, 2, 3, 0, 1, 2, 3},
  {1, 2, 3, 1, 1, 2, 3, 1, 1, 2, 3, 1, 1, 2, 3, 1},
  {2, 3, 1, 2, 2, 3, 1, 2, 2, 3, 1, 2, 2, 3, 1, 2}};

#define BLEND(color, c) blending[color][c]

static Uxn *current_uxn = nil;
uint TARGET_FPS = 60;

static u16int screen_vector = 0;
extern u16int mouse_vector;
extern u16int controller_vector;
extern u8int current_button;
extern u8int mouse_state;

static u8int autoN = 0, autoX = 0, autoY = 0, autoA = 0;
static u16int screen_addr = 0;

extern Rune current_rune;

static u16int window_width = INITIAL_WINDOW_WIDTH;
static u16int window_height = INITIAL_WINDOW_HEIGHT;
static u16int current_x = 0;
static u16int current_y = 0;

static int wctl_fd = 0;

static u8int *fg = nil;
static u8int *bg = nil;
static u8int *sbuf = nil; /* data buffer */
static Image *img = nil;

static int resizeskip = 0;

static Keyboardctl *keyboard = nil;
Mousectl *mouse = nil;

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
  Rectangle r;

  while (getwindow(display, Refnone) != 1)
    ;

  r = Rect(0, 0, window_width, window_height);
  if (img)
    freeimage(img);
  img = allocimage(display, r, RGB24, 0, 0);
  if (img == nil)
    sysfatal("allocimage");

  // redraw();
}

static void
set_new_window_size(void)
{
  uint xr = screen->r.min.x,
       yr = screen->r.min.y;

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
center_window(void)
{
  int cx = display->image->r.max.x/2;
  int cy = display->image->r.max.y/2;
  int x1 = cx - window_width/2;
  int y1 = cy - window_height/2;
  fprint(wctl_fd, "resize -r %d %d %d %d\n", x1, y1, x1+window_width, y1+window_height);

  //update_window_size();
}

static u16int
screen_set_width(u8int getp, u16int w)
{
  if (getp)
    return window_width;
  window_width = w;
  set_new_window_size();
  return w;
}

static u16int
screen_set_height(u8int getp, u16int h)
{
  if (getp)
    return window_height;
  window_height = h;
  set_new_window_size();
  return h;
}

static u16int
screen_set_x(u8int getp, u16int x)
{
  if (getp)
    return current_x;
  current_x = MIN(window_width, x);
  return x;
}

static u16int
screen_set_y(u8int getp, u16int y)
{
  if (getp)
    return current_y;
  current_y = MIN(window_height, y);
  return y;
}

DEFGETSET(screen_set_vec, screen_vector);
DEFGETSET(screen_set_addr, screen_addr);

// 2bpp layer flipy flipx 3 2 1 0
//                        \_____/- color

static u16int
screen_sprite(u8int getp, u16int dat)
{
  s32int i, j, A, tx, ty, x = current_x, y = current_y;
  u8int _2bpp = 0b10000000&dat,
        layer = 0b1000000&dat,
        flipyp = 0b100000&dat,
        flipxp = 0b10000&dat,
        color = 0b1111&dat,
        *target = layer ? fg : bg,
        cur;

  /*
  if (autoX && autoY)
    sysfatal("autoX and autoY");
    */

  if (getp)
    return 0xff;

  for (A = 0; A < autoN + 1; ++A) {
    /* draw sprite */
    for (i = 0; i < 8; ++i) {
      for (j = 0; j < 8; ++j) {
        if (_2bpp)
          cur = (((current_uxn->mem[screen_addr + i]>>(flipxp?j:7-j))&1))|(((current_uxn->mem[screen_addr + i + 8]>>(flipxp?j:7-j))&1)<<1);
        else
          cur = ((current_uxn->mem[screen_addr + i])>>(flipxp?j:7-j))&0b1;
        tx = x+j, ty = (y+(flipyp?7-i:i));
        if (ty < window_height && ty >= 0 && tx < window_width && ty >= 0)
          if (cur || color % 5) /* why */
            target[ty*window_width + tx] = BLEND(cur, color);
      }
    }
    /* end draw sprite */

    if (autoX) y = flipyp? y-8 : y+8;
    if (autoY) x = flipxp? x-8 : x+8;
    if (autoA) screen_addr += _2bpp ? 16 : 8;
  }

  if (autoX) current_x = flipxp ? x-8 : x+8;
  if (autoY) current_y = flipyp ? y-8 : y+8;

  return 0;
}

// fill layer flipy flipx 3 2 1 0
//                            \_/- color

static u16int
screen_pixel(u8int getp, u16int dat)
{
  int i, j;
  u8int color = 0b11&dat,
        fillp = 0b10000000&dat,
        layer = 0b1000000&dat,
        flipxp = 0b100000&dat,
        flipyp = 0b10000&dat,
        *target = layer ? fg : bg;

  if (getp)
    return 0xff;


  if (fillp) {
    for (i = current_y; flipyp ? i >=0 : i < window_height; flipyp ? i-- : i++)
      for (j = current_x; flipxp ? j >=0 : j < window_width; flipxp? j-- : j++)
        target[(i*window_width)+j] = color;
  } else
    target[(current_y*window_width)+current_x] = color;

  if (autoX) current_x++;
  if (autoY) current_y++;

  return dat;
}

// 3 2 1 0 * A Y X

static u16int
screen_auto(u8int getp, u16int dat)
{
  if (getp)
    return (autoN<<4)|autoX|autoY|autoA;
  autoN = (dat&0xf0)>>4;
  autoX = dat&1;
  autoY = dat&(1<<1);
  autoA = dat&(1<<2);
  return 0;
}

void
init_screen_device(Uxn *uxn)
{
  current_uxn = uxn;
  wctl_fd = open("/dev/wctl", OREAD|OWRITE);
  if (wctl_fd < 0)
    sysfatal("wctl");

  if (initdraw(nil, nil, argv0) < 0)
    sysfatal("initdraw: %r");

  set_new_window_size();

  if ((mouse = initmouse(nil, screen)) == nil) sysfatal("initmouse: %r");
  // if ((keyboard = initkeyboard(nil)) == nil) sysfatal("initkeyboard: %r");

  uxn->devices[SCREEN_VECTOR] = screen_set_vec;
  uxn->devices[SCREEN_WIDTH ] = screen_set_width;
  uxn->devices[SCREEN_HEIGHT] = screen_set_height;
  uxn->devices[SCREEN_AUTO  ] = screen_auto;
  uxn->devices[SCREEN_X     ] = screen_set_x;
  uxn->devices[SCREEN_Y     ] = screen_set_y;
  uxn->devices[SCREEN_ADDR  ] = screen_set_addr;
  uxn->devices[SCREEN_PIXEL ] = screen_pixel;
  uxn->devices[SCREEN_SPRITE] = screen_sprite;
}

static void
frame_thread(void *arg)
{
  Channel *c = (Channel*)arg;

  vlong target_timeout_ns = 1000000000/TARGET_FPS, next;

  for (;;) {
    next = nsec() + target_timeout_ns;
    while (nsec() <= next) // how do i avoid busylooping?
      yield();
    send(c, nil);
  }
}

void
screen_main_loop(Uxn *uxn)
{
  int resiz[2];
  u16int run = 0x00ff;
  current_uxn = uxn;

  center_window();
  update_window_size();

  Cursor *c = mallocz(sizeof(Cursor), 1);
  setcursor(mouse, c);

  Channel *drawch = chancreate(1, 1);
  u8int _;
  threadcreate(frame_thread, drawch, 1024);
  proccreate(btn_thread, uxn, 1024);

  enum { CMOUSE, CRESIZ, CREDRAW };
  while (run /*--*/) {
    Alt a[] = {
      { mouse->c,       &mouse->Mouse,   CHANRCV },
      { mouse->resizec, &resiz,          CHANRCV },
      { drawch,         &_,              CHANRCV }, /* draw interrupt channel */
      { nil,            nil,             CHANEND }
    };
    switch (alt(a)) {
    case CMOUSE:
      mouse_state = mouse->buttons;
      if (mouse_vector) {
        lock(&uxn->l);
        uxn->pc = mouse_vector;
        vm(uxn);
        unlock(&uxn->l);
      }
      break;
    case CREDRAW:
      lock(&uxn->l);
      uxn->pc = screen_vector;
      vm(uxn);
      unlock(&uxn->l);
      redraw();
      break;
    case CRESIZ:
      update_window_size();
      break;
    }
  }
}
