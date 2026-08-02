# Makefile for RA3 BattleNet proxy (d3d9.dll) — minimal build
CXX      := i686-w64-mingw32-g++
CXXFLAGS := -std=c++17 -O2 -Wall -Wextra -static-libgcc -static-libstdc++
LDFLAGS  := -static -lws2_32 -lversion
CC       := i686-w64-mingw32-gcc
CFLAGS   := -O2 -Wall

TARGET   := d3d9.dll
SOURCES  := d3d9_dll.cpp forwarders.c

all: $(TARGET)

$(TARGET): $(SOURCES)
	$(CC) $(CFLAGS) -c forwarders.c -o forwarders.o
	$(CXX) $(CXXFLAGS) -c d3d9_dll.cpp -o d3d9_dll.o
	$(CXX) -shared -o $@ d3d9_dll.o forwarders.o $(LDFLAGS)
	@echo "Built $@ ($$(du -h $@ | cut -f1))"
	@i686-w64-mingw32-objdump -p $@ | grep "DLL Name" | sort -u

clean:
	rm -f $(TARGET) *.o *.log

.PHONY: all clean
