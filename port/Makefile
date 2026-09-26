# Native headless test build of MAG (RVIP port): make -C port [ASAN=1]
# Keys from MAG_KEYS / MAG_RANDOM, screen to MAG_DUMP (see fe_tty.c).
CC = gcc
GAME := $(wildcard ../src/*.C)
PORT := pcvideo.c rvip.c menu.c fe_tty.c
CFLAGS := -std=gnu89 -fcommon -w -g -O1 -DPORT -Iinc -I. -include port.h -funsigned-char
ifneq (,$(findstring clang,$(CC)))
CFLAGS += -Wno-error=incompatible-function-pointer-types -Wno-error=int-conversion \
	-Wno-error=incompatible-pointer-types -Wno-error=implicit-function-declaration
endif
ifdef ASAN
CFLAGS += -fsanitize=address,undefined -fno-omit-frame-pointer
endif

mag-native: $(GAME) $(PORT) port.h fe.h
	$(CC) $(CFLAGS) -x c $(GAME) -x none $(PORT) -o $@ -no-pie

clean:
	rm -f mag-native
