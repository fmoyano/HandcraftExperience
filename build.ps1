clear

Write-Output "Compiling shaders..."
fxc /Ges /Zi /T vs_4_1 .\data\shaders\VertexShader.hlsl /Ni /No /Fc .\data\shaders\vs_disassembly /Fo .\data\shaders\vs.cso
fxc /Ges /Zi /T ps_4_1 .\data\shaders\PixelShader.hlsl /Ni /No /Fc .\data\shaders\ps_disassembly /Fo .\data\shaders\ps.cso

Write-Output "Building app..."

pushd build
cl /std:c11 /Zi /sdl /permissive- /W4 /guard:cf /I ../include /I ../include/FreeType ../src/*.c /Fe:main User32.lib dxgi.lib dxguid.lib d3d11.lib freetype.lib /link /LIBPATH:"../lib" /SUBSYSTEM:WINDOWS /MACHINE:X86
popd
