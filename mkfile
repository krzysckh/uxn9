</$objtype/mkfile

CFLAGS=-FTVwp

BIN=$home/bin/$objtype
TARG=uxn9
HFILES=uxn9.h
OFILES=\
  uxn9.$O \
  dsystem.$O \
  ddatetime.$O \
  dmouse.$O \
  dfile.$O \
  dconsole.$O \
  dcontroller.$O \
  dscreen.$O \

</sys/src/cmd/mkone

# profiling
# LDFLAGS=-p
