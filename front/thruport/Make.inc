#!/usr/bin/make -*- makefile-gmake -*-
# This file is included by the toplevel makefile.

               P := front
               D := $P/thruport

        programs := thruport

thruport_sources := main.c control.c daemon.c lock.c monitor.c paths.c \
                    suspend.c

 thruport_cfiles := $(filter %.c, $(thruport_sources:%=%))
 thruport_ofiles := $(thruport_cfiles:%.c=$D/%.o)

     $P_programs += $(programs:%=$D/%)

clean-front/thruport:
	cd front/thruport && rm -f thruport a.out core *~ *.o .*.d TAGS $(JUNK)

front/thruport-programs: $(programs:%=$D/%)
front/thruport-tests:

front/thruport/thruport: $(thruport_ofiles)
	$(FRONT_LD) $(FRONT_LDFLAGS) $^ $(thruport_ldlibs) -o $@

# C source dependency generation.
front/thruport/.%.d: front/thruport/%.c
	@rm -f "$@"
	@$(FRONT_CC) -M -MP -MT 'back/$*.o $@' -MF $@ $(FRONT_CPPFLAGS) $< || \
	    rm -f "$@"

ifeq '$(filter clean% help,$(or $(MAKECMDGOALS),help))' ''
  -include $(thruport_cfiles:%.c=$D/.%.d)
endif
