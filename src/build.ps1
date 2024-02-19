pushd build
cl /std:c11 /Zi /analyze /sdl /permissive- /W4 /guard:cf /I ../include ../src/*.c /Fe:main shell32.lib /link /LIBPATH:"..\lib\x86" /SUBSYSTEM:CONSOLE /MACHINE:X86
popd
sadasds