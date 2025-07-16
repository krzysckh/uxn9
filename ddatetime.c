#include "uxn9.h"

#define Nsec 1000000000

#define DEFDATIME2(nam, ob, dev)           \
  static void                              \
  datetime_ ## nam(Uxn *uxn)               \
  { update_tm();                           \
    SDEV2(dev, tm. ob); }

#define DEFDATIME(nam, ob, dev)            \
  static void                              \
  datetime_ ## nam(Uxn *uxn)               \
  { update_tm();                           \
    SDEV(dev, tm. ob); }

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

DEFDATIME2(year,  year + 1900, DATETIME_YEAR);
DEFDATIME(month,  mon,         DATETIME_MONTH);
DEFDATIME(day,    mday,        DATETIME_DAY);
DEFDATIME(hour,   hour,        DATETIME_HOUR);
DEFDATIME(minute, min,         DATETIME_MINUTE);
DEFDATIME(second, sec,         DATETIME_SECOND);
DEFDATIME(dotw,   wday,        DATETIME_DOTW);
DEFDATIME2(doty,  yday,        DATETIME_DOTY);

void
init_datetime_device(Uxn *uxn)
{
  tz = tzload("local");
  if (tz == nil)
    sysfatal("tzload");

  uxn->trigi[DATETIME_YEAR  ] = datetime_year;
  uxn->trigi[DATETIME_MONTH ] = datetime_month;
  uxn->trigi[DATETIME_DAY   ] = datetime_day;
  uxn->trigi[DATETIME_HOUR  ] = datetime_hour;
  uxn->trigi[DATETIME_MINUTE] = datetime_minute;
  uxn->trigi[DATETIME_SECOND] = datetime_second;
  uxn->trigi[DATETIME_DOTW  ] = datetime_dotw;
  uxn->trigi[DATETIME_DOTY  ] = datetime_doty;
}
