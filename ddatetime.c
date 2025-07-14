#include "uxn9.h"

#define Nsec 1000000000

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
  vlong now = nsec(), s;
  s = now/Nsec;
  now = now%Nsec;
  if (tmtimens(&tm, s, now, tz) == nil)
    sysfatal("tmtimens");
}

DEFDATIME(year, year + 1900);
DEFDATIME(month, mon);
DEFDATIME(day, mday);
DEFDATIME(hour, hour);
DEFDATIME(minute, min);
DEFDATIME(second, sec);
DEFDATIME(dotw, wday);
DEFDATIME(doty, yday);

void
init_datetime_device(Uxn *uxn)
{
  tz = tzload("local");
  if (tz == nil)
    sysfatal("tzload");

  uxn->devices[DATETIME_YEAR  ] = datetime_year;
  uxn->devices[DATETIME_MONTH ] = datetime_month;
  uxn->devices[DATETIME_DAY   ] = datetime_day;
  uxn->devices[DATETIME_HOUR  ] = datetime_hour;
  uxn->devices[DATETIME_MINUTE] = datetime_minute;
  uxn->devices[DATETIME_SECOND] = datetime_second;
  uxn->devices[DATETIME_DOTW  ] = datetime_dotw;
  uxn->devices[DATETIME_DOTY  ] = datetime_doty;
}
