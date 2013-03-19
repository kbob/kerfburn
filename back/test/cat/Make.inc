#!/usr/bin/make -*- makefile-gmake -*-
# This file is included by the toplevel makefile.

cat_sources := back/test/cat/cat.c back/e-stop.c back/fw_assert.c \
               back/fw_stdio.c back/serial.c back/softint.c back/timer.c

load-back/test/cat/cat:  back/test/cat/cat.hex
	$(BACK_AVRDUDE) -qq -p$(BACK_MCU) -D -Uflash:w:$<

back/test/cat-tests: back/test/cat/cat

test-back/test/cat:  back/test/cat-tests

clean-back/test/cat:
	cd back/test/cat && rm -f fw *~ *.o .*.d TAGS $(JUNK)

cat_cfiles := $(filter %.c, $(cat_sources))
# cat_ofiles := $(cat_cfiles:%.c=back/test/cat/%.o)
cat_ofiles := $(cat_cfiles:%.c=%.o)

back/test/cat/cat: $(cat_ofiles)
	$(BACK_LD) $(BACK_LDFLAGS) $^ $(fw_ldlibs) -o $@