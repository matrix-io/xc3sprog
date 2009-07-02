# Project
project(javr)
SET(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}")
cmake_minimum_required(VERSION 2.6)

include_directories (${XC3SPROG_SOURCE_DIR})
link_directories (${XC3SPROG_BINARY_DIR}/xc3sproglib)
 
add_executable(javr   javr.cpp srecdec.cpp parse.cpp  menu.cpp  
		    avr_jtag.cpp fuse.cpp jtag.cpp)
target_link_libraries(javr xc3sproglib ${LIBFTDI_LIBRARIES} ${CONDITIONAL_LIBS})
