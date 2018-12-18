#include "ideastypes.h"
#if defined(__BORLANDC__) && defined(VERSION)
#include <condefs.h>
#endif
#include "lds.h"

#if defined(__BORLANDC__) && defined(VERSION)
//---------------------------------------------------------------------------
USEUNIT("cpu\arm9\arm9.cpp");
USEUNIT("gpu.cpp");
USEUNIT("cpu\arm7\arm7.cpp");
USEUNIT("cpu\syscnt.cpp");
USEUNIT("dsmem.cpp");
USEUNIT("lds.cpp");
USEUNIT("3d\3D.cpp");
USEUNIT("gfx\modeTile.cpp");
USEUNIT("gfx\lcd.cpp");
USEUNIT("gfx\sprite.cpp");
USEUNIT("gfx\mode4.cpp");
USEUNIT("gfx\mode6.cpp");
USEUNIT("3d\math3.cpp");
USEUNIT("3d\3dguid.cpp");
USEUNIT("3d\stack.cpp");
USEUNIT("bios.cpp");
USEUNIT("dlcd.cpp");
USEUNIT("ulcd.cpp");
USEUNIT("cpu.cpp");
USEUNIT("cpu\cpu_0.cpp");
USEUNIT("ext\timers.cpp");
USEUNIT("ext\card.cpp");
USEUNIT("ext\spi.cpp");
USEUNIT("ext\dma.cpp");
USEUNIT("ext\rtc.cpp");
USEUNIT("ext\touch.cpp");
USEUNIT("ext\cpm.cpp");
USEUNIT("ext\fw.cpp");
USEUNIT("ext\pxi.cpp");
USEUNIT("fstream.cpp");
USEUNIT("util.cpp");
USEUNIT("io.cpp");
USEUNIT("lwnd.cpp");
USEUNIT("llist.cpp");
USEUNIT("lslist.cpp");
USEUNIT("lregkey.cpp");
USEUNIT("lstring.cpp");
USEUNIT("lcanvas.cpp");
USEUNIT("plugin.cpp");
USEUNIT("pluginctn.cpp");
USEUNIT("videoplug.cpp");
USEUNIT("graphicsplug.cpp");
USEUNIT("audioplug.cpp");
USEUNIT("lapp.cpp");
USEUNIT("lmenu.cpp");
USEUNIT("lopensavedlg.cpp");
USEUNIT("ldlg.cpp");
USEUNIT("debug\ToolDlg.cpp");
USEUNIT("debug\ConsolleDlg.cpp");
USEUNIT("debug\bp.cpp");
USEUNIT("debug\memview.cpp");
USEUNIT("debug\debug.cpp");
USEUNIT("debug\inputtext.cpp");
USEUNIT("debug\spriteviewer.cpp");
USEUNIT("debug\textureviewer.cpp");
USEUNIT("debug\elf.cpp");
USEUNIT("KeyConfig.cpp");
USEUNIT("zip\zipfile.cpp");
USEUNIT("savestate.cpp");
USEUNIT("fat\fat.cpp");
USEUNIT("ndsreader.cpp");
USEUNIT("cheats.cpp");
USEUNIT("debug\paletteviewer.cpp");
USEUNIT("debug\ioviewer.cpp");
USEUNIT("wifiplugin.cpp");
USEUNIT("debug\inspector.cpp");
USEUNIT("debug\tileviewer.cpp");
USEUNIT("debug\mtxviewer.cpp");
USEUNIT("debug\polyviewer.cpp");
USEUNIT("license.cpp");
USEUNIT("language.cpp");
USEUNIT("cbds.cpp");
USEUNIT("inputplug.cpp");
USEUNIT("7z\7zfile.cpp");
USEUNIT("lzari.cpp");
USEUNIT("dldiplugin.cpp");
USEUNIT("3d\gl_extension.cpp");
USEUNIT("guitar_grip.cpp");
USEUNIT("gba\gbabackup.cpp");
USEUNIT("ext\decrypt.cpp");
USEUNIT("rominfo.cpp");
USEUNIT("plugin\wifi\wifi.cpp");
USEUNIT("plugin\wifi\guid.cpp");
USEUNIT("plugin\mic\capture.cpp");
USEUNIT("plugin\mic\micguid.cpp");
USERC("ideas_ex.rc");
USEUNIT("plugin\aud\sound.cpp");
//---------------------------------------------------------------------------
#endif
//---------------------------------------------------------------------------
LDS ds;
#ifdef _DEBUG
LDebugDlg debugDlg;
LToolsDlg floatDlg;
#endif

HINSTANCE hInst;
#ifdef __WIN32__
//---------------------------------------------------------------------------
int WINAPI WinMain(HINSTANCE h, HINSTANCE hPrev, LPSTR lpCmdLine, int nCmdShow)
{
	hInst = h;
   if(!ds.Init(0,&lpCmdLine))
   	return -1;
#else
LApp *pApp;
//---------------------------------------------------------------------------
int main(int argc,char **argv)
{
	pApp = (LApp *)&ds;
	if(!ds.Init(argc,argv))
 		exit(-1);
#endif
  	ds.MainLoop();
#ifdef __WIN32__
  	return 1;
#else
	exit(1);
#endif
}

