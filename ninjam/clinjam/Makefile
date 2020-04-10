#############################################################
# CPU optimization section
#############################################################

NAME = clinjam
OPTFLAGS =  -O2
OPTFLAGS += -malign-double 

#############################################################
# Basic Configuration
#############################################################

CFLAGS = -fdiagnostics-color=always $(OPTFLAGS) -s -pedantic
CFLAGS += -Wshadow
LFLAGS = -lm -lasound -ljack -lvorbis -lvorbisenc -logg
CC=gcc
CXX=g++

OBJS = ../audiostream_alsa.o ../audiostream_jack.o
OBJS += ../../WDL/jnetlib/asyncdns.o
OBJS += ../../WDL/jnetlib/connection.o
OBJS += ../../WDL/jnetlib/listen.o
OBJS += ../../WDL/jnetlib/util.o
OBJS += ../../WDL/rng.o
OBJS += ../../WDL/sha.o
OBJS += ../mpb.o
OBJS += ../netmsg.o
OBJS += ../njclient.o
OBJS += clinjam.o

CXXFLAGS = $(CFLAGS)

#############################################################
# Builds
#############################################################
${NAME}: $(OBJS)
	$(CXX) $(CXXFLAGS) -o $(NAME) $(OBJS) -lpthread -L.. $(LFLAGS)

install:
	install -d /opt/ninjam
	install -m 755 clinjam /opt/ninjam

clean:
	rm -f $(NAME) *.o
