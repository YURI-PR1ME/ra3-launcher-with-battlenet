# Makefile for RA3 BattleNet proxy — builds winmm.dll and the RA3.exe launcher
CXX      := i686-w64-mingw32-g++
CXXFLAGS := -std=c++17 -O2 -Wall -Wextra -static-libgcc -static-libstdc++
LDFLAGS  := -static -lws2_32 -lversion
CC       := i686-w64-mingw32-gcc
CFLAGS   := -O2 -Wall

TARGET   := winmm.dll
SOURCES  := winmm_dll.cpp forwarders.c
LAUNCHER := launcher/RA3.exe

all: $(TARGET) $(LAUNCHER)

$(TARGET): $(SOURCES)
	$(CC) $(CFLAGS) -c forwarders.c -o forwarders.o
	$(CXX) $(CXXFLAGS) -c winmm_dll.cpp -o winmm_dll.o
	$(CXX) -shared -o $@ winmm_dll.o forwarders.o $(LDFLAGS)
	@echo "Built $@ ($$(du -h $@ | cut -f1))"
	@i686-w64-mingw32-objdump -p $@ | grep "DLL Name" | sort -u

# Launcher: hand the freshly built DLL to the launcher build as
# winmm_proxy.dll (embedded via resource.rc), then build RA3.exe.
# cmp keeps the mtime stable so an unchanged DLL doesn't force a relink.
$(LAUNCHER): $(TARGET) launcher/Makefile
	cmp -s $(TARGET) launcher/winmm_proxy.dll || cp $(TARGET) launcher/winmm_proxy.dll
	$(MAKE) -C launcher

clean:
	rm -f $(TARGET) *.o *.log
	$(MAKE) -C launcher clean

.PHONY: all clean
