# Good targets:
# build all load test tests programs libs clean
# all-front build-front test-front front-tests clean-front
# all-back build-back load-back clean-back

default: help
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
	@echo 'Individual Programs'
	@$(foreach p, $(PROGRAMS),      echo '    $(patsubst ./%,%,$p)';)
	@echo ''
	@echo 'Individual Test Programs'
	@$(foreach t, $(TEST_PROGRAMS), echo '    $(patsubst ./%,%,$t)';)
	@echo ''
	@echo 'Individual Test Scripts'
	@$(foreach t, $(TEST_SCRIPTS), echo '    $(patsubst ./%,%,$t)';)
	@echo ''

all:      all-front all-back
test:     test-front
build:    build-front build-back
clean:    clean-front clean-back

programs: front-programs back-programs
libs:     front-libs
tests:    front-tests


.PHONY: default help all test tests load build frontend backend programs clean

include front/Make.inc
include back/Make.inc
