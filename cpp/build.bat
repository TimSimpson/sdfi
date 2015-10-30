rm -rf build
mkdir build
REM mkdir build\gcc
REM cmake -H. -Bbuild\gcc -G "MSYS Makefiles"
mkdir build\msvc-64
cmake -H. -Bbuild\msvc-64 -G "Visual Studio 14 2015 Win64"
cmake --build build\msvc-64 --config Debug --clean-first
build\msvc-64\Debug\count


