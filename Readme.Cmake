Bauen am Besten in eigenen Verzeichnis, z.b. 
mkdir build; cd build;
"cmake .." oder "cmake -DCMAKE_BUILD_TYPE=Debug .."
mkdir build-win32; cd build-win32;
"cmake -DCMAKE_TOOLCHAIN_FILE=../Toolchain-mingw32.cmake .."

Build import library
pexports -h /opt/cross/i386-mingw32msvc/include/ftd2xx.h \
         /opt/spare/bon/ftdi/ftd2xx/i386/ftd2xx.dll > /tmp/ftd2xx.def
/opt/cross/i386-mingw32msvc/bin/dlltool  -d /tmp/ftd2xx.def -l libftd2xx.a -k
