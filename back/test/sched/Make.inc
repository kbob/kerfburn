test-back/test/sched: back/fw
	cd back; test/sched/dwell1
	cd back; test/sched/dwell2
	cd back; test/sched/dwell3
	cd back; test/sched/dwell4
	cd back; test/sched/dwell5
	cd back; test/sched/dwell6
	cd back; test/sched/move+xf
	cd back; test/sched/move+xm
	cd back; test/sched/move-xs

	cd back; test/sched/move+xf+yf
	cd back; test/sched/move-xf-yf
	cd back; test/sched/move+xf+yq
	cd back; test/sched/move+xf+ym
	cd back; test/sched/move+xf+ys

	cd back; test/sched/move+xm-ym
	cd back; test/sched/move+xm-yq
	cd back; test/sched/move+xs+ys
	cd back; test/sched/move-xs+yq
	cd back; test/sched/move-xs-ys

clean-back/test/sched:
	cd back/test/sched && rm -f fw a.out core *~ *.o .*.d TAGS $(JUNK)
