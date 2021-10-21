CROSS_COMPLICE=/home/garen/gcc-linaro-7.3.1-2018.05-x86_64_aarch64-linux-gnu/bin/aarch64-linux-gnu-
GCC=$(CROSS_COMPLICE)gcc


cap: cap.c videodev2.h
	$(GCC) -o cap cap.c -lm -O3

CLEAN:
	rm cap
