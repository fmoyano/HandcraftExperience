clear

Write-Output "Compiling shaders..."
fxc /Ges /Zi /T vs_4_1 .\data\shaders\VertexShader.hlsl /Ni /No /Fc .\data\shaders\vs_disassembly /Fo .\data\shaders\vs.cso
fxc /Ges /Zi /T ps_4_1 .\data\shaders\PixelShader.hlsl /Ni /No /Fc .\data\shaders\ps_disassembly /Fo .\data\shaders\ps.cso

Write-Output "Building app..."

pushd build
cl /std:c11 /Zi /analyze /sdl /permissive- /W4 /guard:cf /I ../include ../src/*.c /Fe:main User32.lib dxgi.lib dxguid.lib d3d11.lib /link /LIBPATH:"..\lib\x86" /SUBSYSTEM:WINDOWS /MACHINE:X86
popd
