#include "uxn9.h"

#define FILE_SUCCESS 0xa2
#define FILE_STAT    0xa4
#define FILE_DELETE  0xa6
#define FILE_APPEND  0xa7
#define FILE_NAME    0xa8
#define FILE_LENGTH  0xaa
#define FILE_READ    0xac
#define FILE_WRITE   0xae

// im so sorry for the hacks

#define HACK(func, body) \
  static u16int func ## 1 (u8int getp, u16int dat) { current_file = 0; body } \
  static u16int func ## 2 (u8int getp, u16int dat) { current_file = 1; body }

#define HACKDGS(name, body) DEFGETSET_(name ## 1, body, {current_file = 0;}); DEFGETSET_(name ## 2, body, {current_file = 1;});

static uint current_file = 0;

static u16int file_name_addr[2] = {0, 0};
static u16int file_length[2]    = {0, 0};
static u16int file_success[2]   = {0, 0};
static u8int  file_append[2]    = {0, 0};
static u8int  file_delete[2]    = {0, 0};

static int fds[2] = {-1, -1};

static Uxn *current_uxn = nil;

HACKDGS(update_file_length,  file_length[current_file]);
HACKDGS(update_file_success, file_success[current_file]);
HACKDGS(update_file_append,  file_append[current_file]);

HACK(file_stat, {
  USED(getp, dat);
  sysfatal("stat unimplemented");
  return 0;
});

HACK(file_delete, {
  if (getp) return 0;
  USED(dat);
  remove((char*)current_uxn->mem+file_name_addr[current_file]);
  return 0;
});

HACK(file_name, {
  int mode = ORDWR;
  Dir *d;
  if (file_append[current_file])
    sysfatal("TODO: file_append");
  if (getp)
    return file_name_addr[current_file];
  file_name_addr[current_file] = dat;
  if (fds[current_file] > 0)
    close(fds[current_file]);

  fds[current_file] = open((char*)current_uxn->mem+file_name_addr[current_file], mode);

  if ((d = dirfstat(fds[current_file])) != nil)
    sysfatal("dir unsupported");

  return dat;
});

HACK(file_read, {
  if (getp)
    sysfatal("why read from read");
  if (fds[current_file] > 0)
    file_success[current_file] = read(fds[current_file], current_uxn->mem+dat, file_length[current_file]);
  else 
    file_success[current_file] = 0;

  return 0;
});

HACK(file_write, {
  if (getp)
    sysfatal("why read from write");
  if (fds[current_file] > 0)
    file_success[current_file] = write(fds[current_file], current_uxn->mem+dat, file_length[current_file]);
  else 
    file_success[current_file] = 0;

  return 0;
});

void
init_file_device(Uxn *uxn)
{
  current_uxn = uxn;

  uxn->devices[FILE_STAT   ] = file_stat1;
  uxn->devices[FILE_SUCCESS] = update_file_success1;
  uxn->devices[FILE_APPEND ] = update_file_append1;
  uxn->devices[FILE_NAME   ] = file_name1;
  uxn->devices[FILE_LENGTH ] = update_file_length1;
  uxn->devices[FILE_READ   ] = file_read1;
  uxn->devices[FILE_WRITE  ] = file_write1;
  uxn->devices[FILE_DELETE ] = file_delete1;

  uxn->devices[0x10+FILE_STAT   ] = file_stat2;
  uxn->devices[0x10+FILE_SUCCESS] = update_file_success2;
  uxn->devices[0x10+FILE_APPEND ] = update_file_append2;
  uxn->devices[0x10+FILE_NAME   ] = file_name2;
  uxn->devices[0x10+FILE_LENGTH ] = update_file_length2;
  uxn->devices[0x10+FILE_READ   ] = file_read2;
  uxn->devices[0x10+FILE_WRITE  ] = file_write2;
  uxn->devices[0x10+FILE_DELETE ] = file_delete2;
}
