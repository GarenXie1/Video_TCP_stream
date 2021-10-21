CUR_DIR :=$(shell pwd)


UNAME := $(shell uname -sm)

# HOST_ARCH
ifneq (,$(findstring aarch64,$(UNAME)))
	HOST_ARCH := aarch64
endif

ifneq (,$(findstring 86,$(UNAME)))
	HOST_ARCH := x86
endif

ifeq ($(HOST_ARCH),)
$(error Unable to determine HOST_ARCH from uname -sm: $(UNAME)!)
endif


RM = rm -r -f

ifeq ($(HOST_ARCH),x86)
    export PATH:=/svdisk/sunchengzhi/AD500A_Debian/ffmpeg/aarch64-linux-gnu/bin:$(PATH)
	COMPILERPREFIX=aarch64-linux-gnu-
	CC = $(COMPILERPREFIX)gcc
	OMXCORE_INC :=../omx-include
	ENC_LIB := ../lib
	CFLAGS += -fPIC
	CFLAGS += -g -O2 -I$(OMXCORE_INC)
	LDFLAGS := -L$(ENC_LIB) -ldl -lpthread -lion -lalc
endif

ifeq ($(HOST_ARCH),aarch64)
	COMPILERPREFIX=aarch64-linux-gnu-
	CC = $(COMPILERPREFIX)gcc
	LIBS_1 :=/usr/lib/aarch64-linux-gnu
    LIBS_2 :=/lib/aarch64-linux-gnu
	OMXCORE_INC :=/usr/include/omx
	OMX_LIB := /usr/lib/omx
	CFLAGS := -fPIC
	CFLAGS += -g -O2  -I$(OMXCORE_INC)
	LDFLAGS := -L$(OMX_LIB) -ldl -lpthread -lion -lalc
endif


PROG := omx_vce_test

SOURCES_C := $(wildcard *.c)
OBJS_C := $(patsubst %.c, %.o,$(SOURCES_C))

all: clean $(PROG)

$(PROG): $(OBJS_C)
	$(CC) -o $@  $(OBJS_C) $(LDFLAGS)
		
$(OBJS_C) : %.o:%.c
	$(CC) -c $(CFLAGS) $< -o $@	
	
clean:
	echo $(UNAME)
	$(RM) $(PROG) *.o

.PHONY: clean
