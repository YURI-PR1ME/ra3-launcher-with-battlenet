# Makefile for RA3 BattleNet proxy (winmm.dll) — minimal build
CXX      := i686-w64-mingw32-g++
CXXFLAGS := -std=c++17 -O2 -Wall -Wextra -static-libgcc -static-libstdc++
LDFLAGS  := -static -lws2_32 -lversion
CC       := i686-w64-mingw32-gcc
CFLAGS   := -O2 -Wall

TARGET   := winmm.dll
SOURCES  := winmm_dll.cpp forwarders.c

all: $(TARGET)

$(TARGET): $(SOURCES)
	$(CC) $(CFLAGS) -c forwarders.c -o forwarders.o
	$(CXX) $(CXXFLAGS) -c winmm_dll.cpp -o winmm_dll.o
	$(CXX) -shared -o $@ winmm_dll.o forwarders.o $(LDFLAGS)
	@echo "Built $@ ($$(du -h $@ | cut -f1))"
	@i686-w64-mingw32-objdump -p $@ | grep "DLL Name" | sort -u

clean:
	rm -f $(TARGET) *.o *.log

.PHONY: all clean
