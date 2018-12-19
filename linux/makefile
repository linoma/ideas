PROG = libideas.a
CXX = g++
LIB = 
RM = rm -f
CFLAGS = -c -O2
AR = ar rc
RANLIB = ranlib
C_INCLUDE_DIRS = -I"/mnt/win_e/ideas/3d" -I"/mnt/win_e/ideas" -I"/mnt/win_e/ideas/cpu" -I"/mnt/win_e/ideas/cpu/arm7" -I"/mnt/win_e/ideas/cpu/arm9" -I"/mnt/win_e/ideas/7z" -I"/mnt/win_e/ideas/fat" -I"/mnt/win_e/ideas/zip" -I"/mnt/win_e/ideas/debug" -I"/mnt/win_e/ideas/ext" -I"/mnt/win_e/ideas/gfx" -I"/mnt/win_e/ideas/plugin" -I"/mnt/win_e/ideas/plugin/aud" -I"/mnt/win_e/ideas/sound" -I"/mnt/win_e/lzma/c" -I"/mnt/win_e/ideas/linux" -I"/mnt/win_e/lzma/c/Archive/7z" -I"/mnt/win_e/lzma/c/Compress/Lzma" -I"/mnt/win_e/lzma/c/Compress/Branch" -I"/usr/include/gtk-2.0" -I"/usr/lib/gtk-2.0/include" -I"/usr/include/glib-2.0" -I"/usr/lib/glib-2.0/include" -I"/usr/include/pango-1.0" -I"/usr/X11R6/include" -I"/usr/include/freetype2" -I"/usr/include/atk-1.0" -I"/usr/include/cairo"

OBJS = lwindow_base.o

all: $(PROG)

$(PROG): $(OBJS)
	$(AR) $@ $(OBJS)
	-@ ($(RANLIB) $@ || true) >/dev/null 2>&1

lwindow_base.o: lwindow_base.cpp
	$(CXX) $(CFLAGS) $(C_INCLUDE_DIRS) lwindow_base.cpp



clean:
	-$(RM) $(PROG) $(OBJS)

