rm -rf build\msvc
mkdir build
REM mkdir build\gcc
REM cmake -H. -Bbuild\gcc -G "MSYS Makefiles"
mkdir build\msvc-64
cmake -H. -Bbuild\msvc-64 -G "Visual Studio 14 2015 Win64"
rebuild.bat
