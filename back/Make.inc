#!/usr/bin/make -*- makefile-gmake -*-
# This file is included by the toplevel makefile.

      subdirs := test
     programs := fw

   fw_sources := main.c abort.c actions.c e-stop.c fault.c fw_assert.c \
                 fw_stdio.c illum.c parser.c queues.c serial.c softint.c \
                 report.c timer.c variables.c

     THRUPORT := front/thruport/thruport
#    BACK_MCU := atmega2560
      BACK_CC := avr-gcc
      BACK_AR := avr-ar
      BACK_LD := avr-gcc
 BACK_OBJCOPY := avr-objcopy
 BACK_SUSPEND := $(THRUPORT) suspend
 BACK_AVRDUDE := avrdude

BACK_CPPFLAGS := -mmcu=$(BACK_MCU) -DF_CPU=16000000L -Iback -I.
  BACK_CFLAGS := -g -O3 -std=c99 -Wall -Werror
 BACK_LDFLAGS := -Wl,--gc-sections,--relax -mmcu=$(BACK_MCU)
         JUNK += *.hex

    fw_cfiles := $(filter %.c, $(fw_sources:%=back/%))
    fw_ofiles := $(fw_cfiles:%.c=%.o)


all-back:   build-back test-back
build-back: back-programs back-tests
test-back:  build-back $(subdirs:%=test-back/%)
load-back:  back/fw.hex $(THRUPORT)
	$(BACK_SUSPEND) $(BACK_AVRDUDE) -qq -p$(BACK_MCU) -D -Uflash:w:$<

clean-back: $(subdirs:%=clean-back/%)
	cd back && rm -f fw a.out core *~ *.o *.d TAGS $(JUNK)

back-programs: back/fw $(test_programs:%=back/%)
back-tests: $(subdirs:%:back/%-tests)

back/fw: $(fw_ofiles)
	@( \
	echo '#include "version.h"'; \
	echo 'const char version[] PROGMEM =' \
	     "\"Kerfburn Back End, built $$(date +%Y-%m-%dT%H:%M:%S%z)\";" \
        ) > back/version.c
	$(BACK_LD) $(BACK_LDFLAGS) $^ back/version.c $(fw_ldlibs) -o $@
	@rm -f back/version.c

back/%.o: back/%.c
	$(BACK_CC) $(BACK_CFLAGS) $(BACK_CPPFLAGS) -c $< -o $@

back/%.hex: back/%
	$(BACK_OBJCOPY) -O ihex -R .eeprom $< $@

# C source dependency generation.
back/%.d : back/%.c
	@rm -f "$@"
	@$(BACK_CC) -M -MG -MP -MT 'back/$*.o $@' -MF $@ $(BACK_CPPFLAGS) $< \
            || rm -f "$@"

.PHONY: all-back build-back load-back clean-back back-programs

ifeq '$(filter clean% help,$(or $(MAKECMDGOALS),help))' ''
  -include $(fw_cfiles:back/%.c=back/%.d)
endif

include $(subdirs:%=back/%/Make.inc)
