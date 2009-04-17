Bauen am Besten in eigenen Verzeichnis, z.b. 
mkdir build; cd build;
"cmake .." oder "cmake -DCMAKE_BUILD_TYPE=Debug .."
mkdir build-win32; cd build-win32;
"cmake -DCMAKE_TOOLCHAIN_FILE=../Toolchain-mingw32.cmake .."