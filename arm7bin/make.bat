@echo off
set OLDPATH=%PATH%
set PATH=d:\ds\devkitarm\arm-eabi\bin;d:\ds\devkitarm\bin;%PATH%

arm-eabi-gcc -o source\main.o -c -g -fomit-frame-pointer -ffast-math -DARM7 -fno-rtti -fno-exceptions -Id:/ds/libnds/include -Id:/ds/libfat/include -Id:/ds/libdswifi/include source\defaultARM7.c
arm-eabi-gcc -specs=ds_arm7.specs -g -o main.elf source\*.o -lnds7 -ldswifi7d -Ld:\ds\libnds\lib -Ld:\ds\libdswifi
rem arm-eabi-objcopy -O binary main.elf main.bin

:uscita
set PATH=%OLDPATH%
