clear

Write-Output "Compiling shaders..."
fxc /T vs_4_1 .\data\shaders\VertexShader.hlsl /Fo .\data\shaders\vs.cso
fxc /T ps_4_1 .\data\shaders\PixelShader.hlsl /Fo .\data\shaders\ps.cso

Write-Output "Building app..."

pushd build
cl /std:c11 /Zi /analyze /sdl /permissive- /W4 /guard:cf /I ../include ../src/*.c /Fe:main User32.lib shell32.lib dxgi.lib dxguid.lib d3d11.lib d3dcompiler.lib /link /LIBPATH:"..\lib\x86" /SUBSYSTEM:WINDOWS /MACHINE:X86
popd
