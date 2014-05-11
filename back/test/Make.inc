#!/usr/bin/make -*- makefile-gmake -*-
subdirs := cat # softint

back/test-tests: $(subdirs:%=back/test/%-tests)

test-back/test:  $(subdirs:%=test-back/test/%)

clean-back/test: $(subdirs:%=clean-back/test/%)
	cd back/test && rm -f fw a.out core *~ *.o .*.d TAGS $(JUNK)


include $(subdirs:%=back/test/%/Make.inc)
