This is the original musagi open source project, started by DrPetter

All the source code is provided from my old development pile, but coherent build instructions are missing. Some portability steps and cleanup have been undertaken by others and packaged separately into the musagi-linux-2.tar.gz file to avoid eroding the original source material, mainly because I haven't personally verified these changes.

Feel free to fork and modify this code, but try to contact someone that appears to be responsible or attached to it before you publish new things on your own.

Summary project information and an executable version can probably be found at http://drpetter.se

/Tomas Pettersson, DrPetter, drpetter@gmail.com


Building Musagi
====
Currently the most up to date build process for musagi is mingw-compile.bat which utilizes the windows distribution of MinGW (an open source c/c++ compiling suite). 
A visual studio compile process is available via vc-compile.bat, but instructions for getting that to work are not yet available.

To build musagi you need two major prerequisites:
  - MinGW compiler (see http://www.mingw.org/) 
    mingw-compile.bat assumes you install mingw into C:\
  - DirectX 8 SDK
    mingw-compile.bat assumes you install DX8 into C:\DX8install\DXF\DXSDK
    
You are free to update the mingw-compile.bat file and change the assumed installation location of MinGW and DirectX.    
  
Once the prerequisites have been installed, you should be able to build musagi.exe by executing mingw-compile.bat.

