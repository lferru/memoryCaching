mem:	memory_system.c caching.c libframesupport.a memory_system.h
	gcc -g -o mem memory_system.c caching.c -L. -lframesupport

clean:
	rm -rf mem
