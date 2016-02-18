USE_GSL = 0
GSL_PREFIX = /usr/local/Cellar/gsl/1.16
CFLAGS = -g -O2 -std=c99

CFLAGS += -DUSE_GSL=$(USE_GSL)
ifeq "$(USE_GSL)" "1"
  CFLAGS += -I$(GSL_PREFIX)/include
  LDFLAGS += -L$(GSL_PREFIX)/lib
  LDLIBS += -lgsl -lgslcblas
endif
LDLIBS += -lm

la: la.c
