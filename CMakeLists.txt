# Project
project(xc3sprog)
SET(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}")
cmake_minimum_required(VERSION 2.6)

find_package(USB REQUIRED)
include_directories(${LIBUSB_INCLUDE_DIR})
find_package(libftdi)
include_directories(${LIBFTDI_INCLUDE_DIR})


add_executable(debug debug.cpp iobase.cpp ioparport.cpp iodebug.cpp)

add_executable(bitparse bitfile.cpp bitparse.cpp)
add_executable(jedecparse  jedecparse.cpp jedecfile.cpp)

#Bring devlist.txt in a form that we can compile as fallback
file(STRINGS devlist.txt fb_string)
configure_file(config.in.h ${CMAKE_CURRENT_BINARY_DIR}/config.h)
INCLUDE_DIRECTORIES(BEFORE ${CMAKE_CURRENT_BINARY_DIR})

add_executable(detectchain detectchain.cpp ioparport.cpp ioftdi.cpp 
			   iofx2.cpp devicedb.cpp jtag.cpp iobase.cpp)
target_link_libraries(detectchain ${LIBUSB_LIBRARIES} ${LIBFTDI_LIBRARIES} )

add_executable(xc3sprog xc3sprog.cpp ioparport.cpp ioftdi.cpp iofx2.cpp 
			devicedb.cpp jtag.cpp jedecfile.cpp bitfile.cpp 
			iobase.cpp javr.cpp progalgxc95x.cpp progalgxcf.cpp 
			progalgxc3s.cpp progalgavr.cpp)
target_link_libraries(xc3sprog ${LIBUSB_LIBRARIES} ${LIBFTDI_LIBRARIES}  )

SET_DIRECTORY_PROPERTIES(PROPERTIES ADDITIONAL_MAKE_CLEAN_FILES "${GENERATED_FILES}") 
