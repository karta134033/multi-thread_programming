AVXFLAGS = -march=native -mtune=intel -malign-data=compat -mstackrealign -mtls-dialect=gnu2 -maccumulate-outgoing-args
all: pi
pi: pi.c
	gcc -O3 $(AVXFLAGS) -m64 pi.c -lpthread -o pi.out
