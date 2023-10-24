#Ajustar tudo isso aq


preemp: ppos-core-aux.o libppos_static.a pingpong-preempcao.o
			gcc ppos-core-aux.c libppos_static.a pingpong-preempcao.c -o preemp

sche: ppos-core-aux.o libppos_static.a pingpong-scheduler.o
			gcc ppos-core-aux.c libppos_static.a pingpong-scheduler.c -o sche

ppos-core-aux.o: ppos-core-aux.c
			gcc -c ppos-core-aux.c

pingpong-preempcao.o: pingpong-preempcao.c
			gcc -c pingpong-preempcao.c

pingpong-scheduler.o: pingpong-scheduler.c
			gcc -c pingpong-scheduler.c

clean: *.o preemp sche
			rm *.o	preemp sche

target: dependencies
	action