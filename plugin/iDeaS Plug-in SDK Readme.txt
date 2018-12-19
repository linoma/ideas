 iDeaS
Dual Screen Emulator 

Plug-in SDK Read-Me: 
First Download Plug-in SDK and extract the files using IZArc. 
It’s a good freeware alternative to WinZip. http://www.izarc.org/  I prefer it to WinZip. For Linux try PeaZip, but I recommend 7-zip 4.57 http://www.7-zip.org/ 
Follow lecture to build the plug-in.

Tech Lecture: How to build a dynamic-link library plug-in. 
Before any plug-in is a dynamic-link library (.dll), it must contain some function. 
In the SDK, there is an old version of pluginmain.h. However, they are: 
DWORD __declspec(dllexport) I_STDCALL GetInfoFunc(LPGETPLUGININFO p)
BOOL __declspec(dllexport) I_STDCALL SetInfoFunc(LPSETPLUGININFO p)
BOOL __declspec(dllexport) I_STDCALL ResetFunc()
BOOL __declspec(dllexport) I_STDCALL DeleteFunc()
and
BOOL __declspec(dllexport)I_STDCALL RunFunc(..) 

This Function is different for each plug-in type. The prototypes are defined in pluginmain.h, for both Linux and Microsoft Windows.  But, now with a Linux port of iDeaS, there might be a difference in code.  

Building the Plug-in: 
First install Mingw or another Visual C Compiler (C/C++/C#), we are not really sure if Visual C# will work, but we’ll put down that it might. You can get Mingw here: http://www.mingw.org/ 
But, you can use visual c/c++ with a new dll project, the dll must contain the some important function. And their export-name does not contain any strange characters, for example: if in my project I use dllmain.cpp, the c++ compiler creates a dll with export functions named funcname@4 etc., etc. If it's true iDeaS discards the dll, if dll contains funcname. if dll contains funcname it’s ok. In windows maybe the compiler has different entry point function, but a good developer knows it.  
Mingw uses dllmain Borland compiler dll entry point, I think Visual C uses dllmain, but it can be different. the dll(plug-in) must contain these functions, for more info on their format you can see an example in the SDK.
GetInfoFunc
SetInfoFunc
ResetFunc
DeleteFunc 

This last is different for every plug-in type
SetPropertyFunc it's optional, iDeaS call this function when it shows a "property" dialog. 
in the new SDK "__declspec(dllexport)" is I_EXPORT. 
function "GetInfoFunc"
iDeaS call the function for get info on plug-in, when the start and every time iDeaS shows a menu plug-in.



Credits: 
iDeaS Dual Screen Emulator program, developed by: Lino  
iDeaS Plug-in knowledge, and SDK by: Lino   
English Plug-in SDK Read-Me, thanks to and typed by: Wade “Rock” Lincourt.  



 iDeaS 
Dual Screen Emulator 
http://www.ideasemu.org/  
Copyright 2004-2009 Lino 
















  

