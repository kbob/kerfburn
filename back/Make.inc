#!/usr/bin/make -*- makefile-gmake -*-
# This file included by toplevel makefile

     BACK_MCU := atmega2560
      BACK_CC := avr-gcc
      BACK_AR := avr-ar
      BACK_LD := avr-gcc
 BACK_OBJCOPY := avr-objcopy
 BACK_AVRDUDE := avrdude

BACK_CPPFLAGS := -mmcu=$(BACK_MCU) -DF_CPU=16000000L
  BACK_CFLAGS := -g -O3 -Wall -Werror
 BACK_LDFLAGS := -Wl,--gc-sections,--relax -mmcu=$(BACK_MCU)

   fw_sources := main.c
    fw_cfiles := $(filter %.c, $(fw_sources:%=back/%))
    fw_ofiles := $(fw_cfiles:%.c=%.o)

back-programs: back/fw

back/fw: $(fw_ofiles)
	$(BACK_LD) $(BACK_LDFLAGS) $^ $(fw_ldlibs) -o $@

back/%.o: back/%.c
	$(BACK_CC) $(BACK_CFLAGS) $(BACK_CPPFLAGS) -c $< -o $@

all-back:   build-back
build-back: back-programs
clean-back:
	cd back && rm -f fw *~ *.o .*.d TAGS $(JUNK)

.PHONY: back-programs all-back build-back clean-back