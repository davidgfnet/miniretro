
CXXFLAGS=-O2 -ggdb -Wall
CXX=$(PREFIX)g++
LDFLAGS=-ldl

all:
	$(CXX) -o miniretro miniretro.cc util.cc loader.cc $(LDFLAGS) $(CXXFLAGS)

clean:
	rm -f miniretro

