test-back/test/sched: back/fw
	cd back; test/sched/dwell1
	cd back; test/sched/dwell2
	cd back; test/sched/dwell3
	cd back; test/sched/dwell4
	cd back; test/sched/dwell5
	cd back; test/sched/dwell6
	cd back; test/sched/move1

clean-back/test/sched:
	cd back/test/sched && rm -f fw a.out core *~ *.o .*.d TAGS $(JUNK)