test-back/test/sched: back/fw
	cd back; test/sched/dwell1
	cd back; test/sched/dwell2
	cd back; test/sched/dwell3
	cd back; test/sched/dwell4
	cd back; test/sched/dwell5
	cd back; test/sched/dwell6
	cd back; test/sched/move1
	cd back; test/sched/move2
	cd back; test/sched/move3
	cd back; test/sched/move4
	cd back; test/sched/move5
	cd back; test/sched/move6
	cd back; test/sched/move7
	cd back; test/sched/move8
	cd back; test/sched/move9
	cd back; test/sched/move10
	cd back; test/sched/move11
	cd back; test/sched/move12

clean-back/test/sched:
	cd back/test/sched && rm -f fw a.out core *~ *.o .*.d TAGS $(JUNK)
