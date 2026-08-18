# Makefile for the RA3.exe launcher (RA3Bar fork with 战网代理 support)
# Sources merged from RA3Bar-RA3Launcher (MIT, (c) 2018 Lanyi) — see launcher.patch.
#
# Build command matches the upstream CI workflow; import library names are
# lowercase so linking also works on case-sensitive (Linux) filesystems.
CXX      := i686-w64-mingw32-g++
WINDRES  := i686-w64-mingw32-windres
CXXFLAGS := -Os -Wall -std=c++17 -DUNICODE -D_UNICODE

TARGET   := RA3.exe
OBJECTS  := UserInterface.o main.o resource.res

# resource.rc embeds winmm_proxy.dll as RCDATA (PROXY_DLL); the parent
# Makefile copies the freshly built winmm.dll here before invoking us.
RES_FILES := RA3.exe.Manifest arbm.bmp ra3bar.bmp ra3bar.array \
             ra3barlauncher_chinese.csf ra3barlauncher_english.csf \
             ra3interesting.ico resource.rc resource.h winmm_proxy.dll

HDRS := UserInterface.hpp WindowsWrapper.hpp ReplaysAndMods.hpp \
        CSFParser.hpp Input.hpp common.hpp

all: $(TARGET)

UserInterface.o: UserInterface.cpp $(HDRS)
	$(CXX) $(CXXFLAGS) -c UserInterface.cpp -o $@

main.o: main.cpp $(HDRS)
	$(CXX) $(CXXFLAGS) -c main.cpp -o $@

resource.res: $(RES_FILES)
	$(WINDRES) -i resource.rc -o $@ -O coff

$(TARGET): $(OBJECTS)
	$(CXX) $(OBJECTS) -o $@ -mwindows -static-libgcc -static-libstdc++ \
	    -lcomctl32 -lshlwapi -static -lpthread -s
	@echo "Built $(TARGET) ($$(du -h $(TARGET) | cut -f1))"

# Friendly error when someone builds the launcher directly without the DLL.
winmm_proxy.dll:
	@echo "Error: winmm_proxy.dll not found — run 'make' from the parent directory first"
	@exit 1

clean:
	rm -f $(TARGET) $(OBJECTS)

.PHONY: all clean
