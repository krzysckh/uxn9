#include "uxn9.h"

// im so sorry for the hacks

#define HACK(func, body) \
  static void func ## 1 (Uxn *uxn) { current_file = 0; body } \
  static void func ## 2 (Uxn *uxn) { current_file = 1; body }

#define DEVF(dev)  DEV(dev+current_file*0x10)
#define DEVF2(dev) DEV2(dev+current_file*0x10)

#define SDEVF(dev,v)  SDEV(dev+current_file*0x10, v)
#define SDEVF2(dev,v) SDEV2(dev+current_file*0x10, v)

static uint current_file = 0;
static int fds[2] = {-1, -1};

HACK(file_stat, {
  sysfatal("stat unimplemented");
});

HACK(file_delete, {
  remove((char*)uxn->mem+DEVF2(FILE_NAME));
});

static void
maybe_create(char *name)
{
  int fd = open(name, OREAD);
  if (fd > 0)
    close(fd);
  else
    create(name,  OREAD|OEXCL, 0666);
}

HACK(file_name, {
  int mode = ORDWR;
  char *name;
  Dir *d;
  if (DEVF(FILE_APPEND))
    sysfatal("TODO: file_append");

  if (fds[current_file] > 0)
    close(fds[current_file]);

  name = (char*)uxn->mem+DEVF2(FILE_NAME);
  maybe_create(name);
  fds[current_file] = open(name, mode);

  // print("file: %s", (char*)uxn->mem+DEVF2(FILE_NAME));
  /*
  if ((d = dirfstat(fds[current_file])) != nil)
    sysfatal("dir unsupported: %s", (char*)uxn->mem+DEVF2(FILE_NAME));
    */
});

HACK(file_read, {
  if (fds[current_file] > 0)
    SDEVF2(FILE_SUCCESS, read(fds[current_file], uxn->mem+DEVF2(FILE_READ), DEVF2(FILE_LENGTH)))
  else 
    SDEVF2(FILE_SUCCESS, 0);
});

HACK(file_write, {
  if (fds[current_file] > 0)
    SDEVF2(FILE_SUCCESS, write(fds[current_file], uxn->mem+DEVF2(FILE_WRITE), DEVF2(FILE_LENGTH)))
  else 
    SDEVF(FILE_SUCCESS, 0);
});

void
init_file_device(Uxn *uxn)
{
  uxn->trigo[FILE_STAT   ] = file_stat1;
  uxn->trigo[FILE_READ   ] = file_read1;
  uxn->trigo[FILE_WRITE  ] = file_write1;
  uxn->trigo[FILE_NAME   ] = file_name1;
  uxn->trigo[FILE_DELETE ] = file_delete1;

  uxn->trigo[0x10+FILE_STAT   ] = file_stat2;
  uxn->trigo[0x10+FILE_READ   ] = file_read2;
  uxn->trigo[0x10+FILE_WRITE  ] = file_write2;
  uxn->trigo[0x10+FILE_NAME   ] = file_name2;
  uxn->trigo[0x10+FILE_DELETE ] = file_delete2;
}

void
close_file_device(Uxn *uxn)
{
  USED(uxn);
  if (fds[0] > 0) close(fds[0]);
  if (fds[1] > 0) close(fds[1]);
}
