# Project
project(xc3sprog)
SET(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}")
cmake_minimum_required(VERSION 2.6)

set(USE_STATIC_FTDI ON)
find_package(libftdi)
include_directories(${LIBFTDI_INCLUDE_DIR})

#link libusb dynamic at runtime on windows at at compile time else
#this circumvents a problem with Debian
set (CONDITIONAL_FILES)
set (CONDITIONAL_LIBS)
if("${CMAKE_SYSTEM_NAME}" STREQUAL "Windows")
   set (CONDITIONAL_FILES ${CONDITIONAL_FILES} "libusb_dyn.c")
else("${CMAKE_SYSTEM_NAME}" STREQUAL "Windows")
   find_package(libusb)
   include_directories(${LIBUSB_INCLUDE_DIR})
   set(CONDITIONAL_LIBS ${CONDITIONAL_LIBS} ${LIBUSB_LIBRARIES})
endif("${CMAKE_SYSTEM_NAME}" STREQUAL "Windows")

set (LIBUSRP_INCLUDE_DIR "fallback/")
include_directories(${LIBUSRP_INCLUDE_DIR})


include_directories(${CMAKE_SOURCE_DIR})

add_executable(debug debug.cpp iobase.cpp ioparport.cpp iodebug.cpp)

add_executable(bitparse bitfile.cpp bitparse.cpp)
add_executable(jedecparse  jedecparse.cpp jedecfile.cpp)
add_executable(srecparse  srecparse.cpp srecfile.cpp)

ADD_CUSTOM_COMMAND(OUTPUT config.h
    COMMAND ${CMAKE_COMMAND} -DDEVLIST_DIR=${CMAKE_SOURCE_DIR} -P ${CMAKE_SOURCE_DIR}/devlist.cmk
    DEPENDS devlist.txt
)

INCLUDE_DIRECTORIES(BEFORE ${CMAKE_CURRENT_BINARY_DIR})

add_library(xc3sproglib  STATIC ioparport.cpp ioftdi.cpp iofx2.cpp 
                      devicedb.cpp jtag.cpp jedecfile.cpp bitfile.cpp 
			iobase.cpp  progalgxc95x.cpp 
			progalgxcf.cpp progalgxc3s.cpp progalgavr.cpp 
			ioxpc.cpp progalgspiflash.cpp ${CONDITIONAL_FILES} config.h)

add_executable(detectchain detectchain.cpp  config.h)
target_link_libraries(detectchain xc3sproglib ${LIBFTDI_LIBRARIES} ${CONDITIONAL_LIBS})

add_executable(xc3sprog xc3sprog.cpp javr.cpp srecfile.cpp progalgavr.cpp 
			config.h)

target_link_libraries(xc3sprog xc3sproglib ${LIBFTDI_LIBRARIES} ${CONDITIONAL_LIBS}  )

add_subdirectory(javr)

SET_DIRECTORY_PROPERTIES(PROPERTIES ADDITIONAL_MAKE_CLEAN_FILES "${GENERATED_FILES}") 
