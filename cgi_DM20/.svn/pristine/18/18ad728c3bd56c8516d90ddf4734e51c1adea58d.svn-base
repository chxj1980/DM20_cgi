#export CROSS:=arm-hisiv300-linux-
export CC:=$(CROSS)g++
export AR:=$(CROSS)ar



LIBPATH = $(PWD)/../lib

INC_FLAGS := -I$(PWD)/../include
INC_FLAGS += -I.


CFLAGS := -static -Werror -Wall -g -fPIC -c 
CFLAGS += -x c $(INC_FLAGS)


LDFLAGS := -L$(LIBPATH)/
LIBS := -ldmsdk -lpthread

SRC  = $(wildcard *.cpp)

OBJS    = $(SRC:%.cpp=%.o)
EXE     = dmsdk


VPATH   = $(PWD) $(PWD)/objs

OBJPATH = $(PWD)/objs

$(OBJPATH)/$(EXE): $(OBJPATH) $(OBJS)
	cd $(OBJPATH);$(CC) -static -o $(OBJPATH)/$(EXE) $(OBJS) $(LD_LIB) $(LDFLAGS) $(LIBS)
	cp -f $(OBJPATH)/$(EXE) ./

	
$(OBJPATH):
	mkdir -p $(OBJPATH)

$(OBJS): %.o: %.cpp
	$(CC) $(CFLAGS) $< -o $(OBJPATH)/$@


.PHONY:clean
clean:
	rm -rf $(OBJPATH)

	
