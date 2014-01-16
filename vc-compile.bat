@echo off

cd temp
cl ..\main.cpp ..\part.cpp ..\song.cpp ..\fileselector.cpp ..\fftsg.cpp ..\DPInput.cpp ..\Log.cpp ..\Texture.cpp ..\pa\pa_lib.c ..\pa\pa_dsound.c ..\pa\dsound_wrapper.c   /Femusagi.exe  /O2 /W2 /ML /nologo /Gm /Zi /QIfist    /link /subsystem:windows winmm.lib dinput8.lib dsound.lib dxguid.lib opengl32.lib glu32.lib    kernel32.lib user32.lib gdi32.lib comdlg32.lib > output.txt
copy musagi.exe ..\
