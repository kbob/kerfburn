#!/usr/bin/make -*- makefile-gmake -*-
# This file included by toplevel makefile

       subdirs := thruport

      FRONT_CC := gcc
      FRONT_AR := ar
      FRONT_LD := gcc

FRONT_CPPFLAGS := -Ifront -D_GNU_SOURCE
  FRONT_CFLAGS := -g -std=c99 -Wall -Werror
 FRONT_LDFLAGS := 

front-libs:
front-programs: $(subdirs:%=front/%-programs)
front-tests:    $(subdirs:%=front/%-tests)

all-front:   build-front test-front
test-front:  front-tests
build-front: front-libs front-programs
clean-front: $(subdirs:%=clean-front/%)

front/%.o: front/%.c
	$(FRONT_CC) $(FRONT_CFLAGS) $(FRONT_CPPFLAGS) -c $< -o $@

.PHONY: front-libs front-programs front-tests \
	all-front test-front build-front clean-front

include $(subdirs:%=front/%/Make.inc)
