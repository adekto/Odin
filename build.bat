@echo off

:: Make sure this is a decent name and not generic
set exe_name=odin.exe

:: Debug = 0, Release = 1
set release_mode=0
set compiler_flags= -nologo -Oi -TC -fp:fast -fp:except- -Gm- -MP -FC -GS- -EHsc- -GR-

if %release_mode% EQU 0 ( rem Debug
	set compiler_flags=%compiler_flags% -Od -MDd -Z7
	rem -DDISPLAY_TIMING
) else ( rem Release
	set compiler_flags=%compiler_flags% -O2 -MT -Z7
)

set compiler_warnings= ^
	-W4 -WX ^
	-wd4100 -wd4101 -wd4127 -wd4189 ^
	-wd4201 -wd4204 -wd4244 ^
	-wd4306 ^
	-wd4456 -wd4457 -wd4480 ^
	-wd4505 -wd4512 -wd4550

set compiler_includes=
set libs= ^
	kernel32.lib
	rem "src\dyncall\lib\*.lib"

set linker_flags= -incremental:no -opt:ref -subsystem:console

if %release_mode% EQU 0 ( rem Debug
	set linker_flags=%linker_flags% -debug
) else ( rem Release
	set linker_flags=%linker_flags% -debug
)

set compiler_settings=%compiler_includes% %compiler_flags% %compiler_warnings%
set linker_settings=%libs% %linker_flags%


del *.pdb > NUL 2> NUL
del *.ilk > NUL 2> NUL

cl %compiler_settings% "src\main.c" ^
	/link %linker_settings% -OUT:%exe_name% ^
	&& odin run code/demo.odin
	rem && odin build_dll code/example.odin ^
	rem odin run code/demo.odin

rem pushd src\asm
rem 	nasm hellope.asm -fwin64 -o hellope.obj ^
rem 	&& cl /nologo hellope.obj /link kernel32.lib /entry:main  ^
rem 	&& hellope.exe
rem popd

:end_of_build

