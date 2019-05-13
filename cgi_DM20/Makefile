#export CROSS:=arm-hisiv300-linux-
export CC:=$(CROSS)gcc
export C++:=$(CROSS)g++
export AR:=$(CROSS)ar


INC_FLAGS = -I./include
INC_FLAGS += -I./src/http	
INC_FLAGS += -I./src/bm	
INC_FLAGS += -I./src/rtp
INC_FLAGS += -I./src/rtsp

HEAD_FILES = ./include/*.h

CFLAGS  = -Wall -c -O2 -fPIC



C_SRC  = $(wildcard ./src/*.c)
C_SRC += $(wildcard ./src/http/*.c)

C++_SRC  = $(wildcard ./src/*.cpp)
C++_SRC += $(wildcard ./src/http/*.cpp)
C++_SRC += $(wildcard ./src/bm/*.cpp)
C++_SRC += $(wildcard ./src/rtp/*.cpp)
C++_SRC += $(wildcard ./src/rtsp/*.cpp)


OBJS    = $(C_SRC:.c=.o) $(C++_SRC:.cpp=.o)

EXE     = libdmsdk.a
EXE1    = libdmsdk.so


VPATH   = $(PWD)/src
VPATH  += $(PWD)/src/http
VPATH  += $(PWD)/include


$(OBJPATH)/$(EXE): $(OBJS)
	$(AR) r ./lib/$(EXE) $(OBJS);
	$(C++) -o ./lib/$(EXE1) $(OBJS) -fPIC -shared
	cd ./sample; make clean && make


%.o: %.c
	$(CC) -o $@ $(CFLAGS) $< $(INC_FLAGS)	


%.o: %.cpp
	$(C++) -o $@ $(CFLAGS) $< $(INC_FLAGS)	

	
.PHONY:clean
clean:
	rm -rf $(OBJPATH)
	rm -rf $(OBJS)
	
.PHONY:release
release:
	rm -rf release
	
	mkdir -p release/sample
	mkdir -p release/include
	mkdir -p release/lib
	mkdir -p release/doc
	cp -rf include/dmsdk.h release/include/
	cp -rf lib/*.a release/lib/
	#cp -rf lib/*.so release/lib/
	cp -rf doc/*.pdf release/doc/
	cp -rf Readme.txt release/
	cp -rf sample/*.cpp release/sample/
	cp -rf sample/Makefile release/sample/

	tar zcf libdmsdk.tar.gz release
	
