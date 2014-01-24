SET PATH=$PATH;c:\MinGW\bin
SET DXSDK=C:\DX8install\DXF\DXSDK
SET SOURCE=main.cpp part.cpp song.cpp fileselector.cpp fftsg.cpp DPInput.cpp Log.cpp Texture.cpp
SET PA=pa_v14\pa_common\pa_lib.c pa_v14\pa_win_ds\pa_dsound.c pa_v14\pa_win_ds\dsound_wrapper.c
SET INCLUDES=-I "%DXSDK%\include" -I "pa_v14/pa_common" -I "pa_v14/pa_win_ds"
SET LINKS=-ldxguid -ldsound -ldinput8 -lWinmm -lOpenGL32 -lGlu32 -lGdi32 -lComdlg32

gcc %SOURCE% %PA% -w %INCLUDES% -o musagi.exe %LINKS% -fpermissive -static -static-libgcc -static-libstdc++ -lstdc++

del *.o

pause