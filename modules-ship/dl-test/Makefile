
CC = gcc
LD_SHARED = gcc -shared
COREFLAGS = -rdynamic

all: dl-test-lib.so dl-test-core

dl-test-lib.so: dl-test-lib.c
	$(CC) $(LIBFLAGS) -c -o dl-test-lib.o dl-test-lib.c
	$(LD_SHARED) -o dl-test-lib.so dl-test-lib.o

dl-test-core: dl-test-core.c
	$(CC) $(COREFLAGS) -o dl-test-core dl-test-core.c -ldl

clean:
	rm -f dl-test-lib.o dl-test-core.o dl-test-core dl-test-lib.so
