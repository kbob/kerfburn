#!/usr/bin/make -*- makefile-gmake -*-
# This file is included by the toplevel makefile.

subdirs := softint softint

softint_sources := softint.c serial.c

back/test/softint-tests: back/test/softint/softint

test-back/test/softint:  back/test/softint-tests

softint_cfiles := $(filter %.c, $(softint_sources:%=back/test/softint/%))
softint_ofiles := $(softint_cfiles:%.c=%.o)

back/test/softint/softint: $(softint_ofiles)
	$(BACK_LD) $(BACK_LDFLAGS) $^ $(fw_ldlibs) -o $@
