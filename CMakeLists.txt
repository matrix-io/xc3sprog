# Project
project(xc3sprog)

set(CMAKE_CXX_FLAGS "-g -Wall")
cmake_minimum_required(VERSION 2.6)


include_directories(${CMAKE_SOURCE_DIR})

ADD_CUSTOM_COMMAND(OUTPUT devices.h
    COMMAND ${CMAKE_COMMAND} -DDEVLIST_DIR=${CMAKE_SOURCE_DIR} -P ${CMAKE_SOURCE_DIR}/devlist.cmk
    DEPENDS devlist.txt
)

ADD_CUSTOM_COMMAND(OUTPUT cables.h
    COMMAND ${CMAKE_COMMAND} -DCABLELIST_DIR=${CMAKE_SOURCE_DIR} -P ${CMAKE_SOURCE_DIR}/cablelist.cmk
    DEPENDS cablelist.txt
)

INCLUDE_DIRECTORIES(BEFORE ${CMAKE_CURRENT_BINARY_DIR})

add_library(xc3sproglib  STATIC sysfs.cpp  
                        devicedb.cpp jtag.cpp jedecfile.cpp bitfile.cpp 
			iobase.cpp  progalgxc95x.cpp utilities.cpp
			progalgxcf.cpp progalgxcfp.cpp progalgxc3s.cpp
			progalgavr.cpp progalgxc2c.cpp  mapfile_xc2c.cpp
			progalgspiflash.cpp bitrev.cpp
                        cabledb.cpp pdioverjtag.cpp xmega_pdi_nvm.cpp
                        ${CONDITIONAL_FILES} devices.h cables.h)

add_executable(xc3sprog xc3sprog.cpp javr.cpp srecfile.cpp progalgavr.cpp 
			devices.h)
target_link_libraries(xc3sprog xc3sproglib ${CONDITIONAL_LIBS}  ${LIBS} )

install(TARGETS xc3sprog DESTINATION bin)
add_subdirectory(packages)
include(CPack)


SET_DIRECTORY_PROPERTIES(PROPERTIES ADDITIONAL_MAKE_CLEAN_FILES "${GENERATED_FILES}") 
