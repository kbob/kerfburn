#!/usr/bin/make -*- makefile-gmake -*-
# This file included by toplevel makefile

front-libs:
front-programs:
front-tests:

all-front:   build-front test-front
test-front:  front-tests
build-front: front-libs front-programs
clean-front: 

.PHONY: front-libs front-programs front-tests \
	all-front test-front build-front clean-front
