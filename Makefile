# Top level makefile.
# XXX write a doc on the build system.

MAKEFLAGS += -j

.MAKE_DEFAULT: help
help:
	@echo 'Common Targets'
	@echo '    all            - build everything, run all tests'
	@echo '    test           - run all tests'
	@echo '    load           - build and download firmware'
	@echo '    build          - build everything'
	@echo '    programs       - build all programs'
	@echo '    libs           - build all libraries'
	@echo '    tests          - build all tests'
	@echo '    clean          - remove generated files'
	@echo '    help           - print this text'
	@echo ''
	@echo 'Single-Ended Targets'
	@echo '    all-front      - everything for front end'
	@echo '    all-back       - everything for back end'
	@echo '    build-front    - build front end'
	@echo '    build-back     - build back end'
	@echo '    clean-front    - remove generated files in front end'
	@echo '    clean-back     - remove generated files in back end'
	@echo ''

all:      all-front all-back
test:     test-front
load:     load-back
build:    build-front build-back
clean:    clean-front clean-back

programs: front-programs back-programs
libs:     front-libs
tests:    front-tests


.PHONY: all build clean default help libs load programs test tests

include front/Make.inc
include back/Make.inc
