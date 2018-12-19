set OLDPATH=%PATH%
set PATH=h:\ds\devkitarm\arm-eabi\bin;h:\ds\devkitarm\bin;%PATH%
@if %1 == 9 goto arm9
@if %1 == 7 goto arm7
@if %1 == gba goto gba
@goto uscita

:arm9
gcc -mcpu=arm946e-s -mtune=arm946e-s -c arm9.s
ld -o arm9.elf -nostartfile -nostdlib arm9.o -Ttext=0xFFFF0000
arm-eabi-objcopy -O binary startfile arm9.bin
goto uscita

:arm7
gcc -c arm7.s
ld -o arm7.elf -nostartfile -nostdlib arm7.o -Ttext=0x00000000
arm-eabi-objcopy -O binary startfile arm7.bin 
goto uscita

:gba
gcc -c arm7_gba.s
ld -o arm7_gba.elf -nostartfile -nostdlib arm7_gba.o -Ttext=0x00000000
arm-eabi-objcopy -O binary startfile arm7_gba.bin 

:uscita
set PATH=%OLDPATH%
