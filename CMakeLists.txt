# Project
project(xc3sprog)
SET(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}")
cmake_minimum_required(VERSION 2.6)
set(USE_STATIC_FTDI ON)

# CONDITIONAL_LIBS is added to other unconditional libs below
set(CONDITIONAL_LIBS )
include(CheckLibraryExists)
check_library_exists(dl dlopen "" HAVE_LIBDL)
if(HAVE_LIBDL)
    set(CONDITIONAL_LIBS ${CONDITIONAL_LIBS} dl)
endif(HAVE_LIBDL)

find_package(libftdi)
include_directories(${LIBFTDI_INCLUDE_DIR})


add_executable(debug debug.cpp iobase.cpp ioparport.cpp iodebug.cpp)

add_executable(bitparse bitfile.cpp bitparse.cpp)
add_executable(jedecparse  jedecparse.cpp jedecfile.cpp)
add_executable(srecparse  srecparse.cpp srecfile.cpp)

#Bring devlist.txt in a form that we can compile as fallback
file(STRINGS devlist.txt fb_string)
configure_file(config.in.h ${CMAKE_CURRENT_BINARY_DIR}/config.h)
INCLUDE_DIRECTORIES(BEFORE ${CMAKE_CURRENT_BINARY_DIR})

add_executable(detectchain detectchain.cpp ioparport.cpp ioftdi.cpp 
			   iofx2.cpp devicedb.cpp jtag.cpp iobase.cpp 
			   libusb_dyn.c)
target_link_libraries(detectchain ${LIBFTDI_LIBRARIES} ${CONDITIONAL_LIBS})

add_executable(xc3sprog xc3sprog.cpp ioparport.cpp ioftdi.cpp iofx2.cpp 
			devicedb.cpp jtag.cpp jedecfile.cpp bitfile.cpp 
			iobase.cpp javr.cpp srecfile.cpp progalgxc95x.cpp 
			progalgxcf.cpp progalgxc3s.cpp progalgavr.cpp libusb_dyn.c)
target_link_libraries(xc3sprog ${LIBFTDI_LIBRARIES} ${CONDITIONAL_LIBS}  )

SET_DIRECTORY_PROPERTIES(PROPERTIES ADDITIONAL_MAKE_CLEAN_FILES "${GENERATED_FILES}") 
