CXX=g++
CFLAGS=-O2
CXXFLAGS = $(CFLAGS)
default: autosong


OBJS=autosong.o ../../WDL/lameencdec.o

autosong: $(OBJS)
	$(CXX) -o autosong $(OBJS) -lvorbis -ldl -logg
