#include "uxn9.h"

#define DATETIME_YEAR   0xc0
#define DATETIME_MONTH  0xc2
#define DATETIME_DAY    0xc3
#define DATETIME_HOUR   0xc4
#define DATETIME_MINUTE 0xc5
#define DATETIME_SECOND 0xc6
#define DATETIME_DOTW   0xc7
#define DATETIME_DOTY   0xc8
#define DATETIME_ISDST  0xca

#define DEFDATIME(nam, ob)                 \
  static u16int                            \
  datetime_ ## nam(u8int getp, u16int dat) \
  { update_tm();                           \
    USED(dat);                             \
    if (getp) return tm. ob;               \
    sysfatal("set datetime");              \
    return 0; }                  


static Tzone *tz;
static Tm tm;

static void
update_tm(void)
{
  tmtimens(&tm, 0, 0, tz);
}

DEFDATIME(second, sec);
DEFDATIME(year, year + 1900);

void
init_datetime_device(Uxn *uxn)
{
  tz = tzload("local");
  uxn->devices[DATETIME_SECOND] = datetime_second;
  uxn->devices[DATETIME_YEAR] = datetime_year;
}
