# V4 Hybrid procfs/perf_event/resctrl IntP
# No framework dependencies -- pure C with Linux kernel interfaces

CC      = gcc
CFLAGS  = -Wall -Wextra -O2 -std=c11 -D_GNU_SOURCE
LDFLAGS = -lpthread

TARGET  = intp-hybrid
SRCS    = intp-hybrid.c \
          src/netp.c \
          src/nets.c \
          src/blk.c \
          src/mbw.c \
          src/llcmr.c \
          src/llcocc.c \
          src/cpu.c \
          src/detect.c
OBJS    = $(SRCS:.c=.o)

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(TARGET) $(OBJS)
