@echo off

REM create build directory
pushd %CD%
IF NOT EXIST build mkdir build
cd build

REM CF = Compiler Flags
REM LF = Linker Flags

set CF_DEFAULT= -MTd -nologo -Gm- -GR- -EHa- -Od -Oi -FC -Z7 -W3 -EHsc -D_CRT_SECURE_NO_WARNINGS /I..\stb
set CF_SDL= /I..\sdl-vc\include 
set CF_OPENGL= /I..\glad
set CF_VULKAN= /I..\VulkanSDK\1.3.268.0\Include

set LF_DEFAULT= -incremental:no -opt:ref -subsystem:windows 
set LF_SDL= shell32.lib ..\sdl-vc\lib\x64\SDL2main.lib ..\sdl-vc\lib\x64\SDL2.lib
set LF_OPENGL= opengl32.lib
set LF_VULKAN= ..\VulkanSDK\1.3.268.0\Lib\vulkan-1.lib

cl %CF_DEFAULT% %CF_SDL% %CF_VULKAN% -DWINDOWS -DSDL -DVULKAN -DDEBUG ../sdl_application.cpp /link %LF_DEFAULT% %LF_SDL% %LF_VULKAN% /out:vulkan.exe


IF NOT EXIST SDL2.dll copy ..\sdl-vc\lib\x64\SDL2.dll