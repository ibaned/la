USE_GSL = 0
USE_METIS = 0
USE_ZOLTAN = 0
GSL_PREFIX = /usr/local/Cellar/gsl/1.16
METIS_PREFIX = /Users/dibanez/code/parmetis-install
CFLAGS = -g -O2 -std=c99

CFLAGS += -DUSE_GSL=$(USE_GSL)
ifeq "$(USE_GSL)" "1"
  CFLAGS += -I$(GSL_PREFIX)/include
  LDFLAGS += -L$(GSL_PREFIX)/lib
  LDLIBS += -lgsl -lgslcblas
endif
CFLAGS += -DUSE_METIS=$(USE_METIS)
ifeq "$(USE_METIS)" "1"
  CFLAGS += -I$(METIS_PREFIX)/include
  LDFLAGS += -L$(METIS_PREFIX)/lib
  LDLIBS += -lmetis
endif
CFLAGS += -DUSE_ZOLTAN=$(USE_ZOLTAN)
LDLIBS += -lm

la: la.c
