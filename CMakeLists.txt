# Project
project(xc3sprog)
SET(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}")
set(CMAKE_CXX_FLAGS "-g -Wall")
cmake_minimum_required(VERSION 2.6)

if(${WIN32})
set(USE_STATIC_FTDI ON)
endif(${WIN32})

option(USE_FTD2XX "Use FTDI libFTD2XX instead of free libftdi" ON)

find_package(libftdi)
include_directories(${LIBFTDI_INCLUDE_DIR})

if(USE_FTD2XX)
  find_package(libFTD2XX)
endif(USE_FTD2XX)

if(LIBFTD2XX_FOUND)
  include_directories(${LIBFTD2XX_INCLUDE_DIR})
  add_definitions( -DUSE_FTD2XX )
else(LIBFTD2XX_FOUND)
  set(LIBFTD2XX_LIBRARIES "")
endif(LIBFTD2XX_FOUND)

FIND_PACKAGE(Subversion)
IF(Subversion_FOUND)
  Subversion_WC_INFO(${PROJECT_SOURCE_DIR} Project)
  set(VERSION_STRING "svn_ver${Project_WC_REVISION}")
ELSE(Subversion_FOUND)
  set(VERSION_STRING "unknown")
ENDIF(Subversion_FOUND)

# Create suffix to eventually install inlib64
IF(CMAKE_SIZEOF_VOID_P EQUAL 4)
    SET(LIB_SUFFIX "")
    SET(PACK_ARCH ".x86")
  ELSE(CMAKE_SIZEOF_VOID_P EQUAL 8)
    SET(LIB_SUFFIX 64)
    SET(PACK_ARCH .x86_64)
endif(CMAKE_SIZEOF_VOID_P EQUAL 4)

# Package information
set(CPACK_PACKAGE_VERSION              ${VERSION_STRING})
set(CPACK_PACKAGE_CONTACT              "Uwe Bonnes <bon@elektron.ikp.physik.tu-darmstadt.de>")
set(CPACK_PACKAGE_DESCRIPTION          "JTAG Progarmming tools")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY  ${CPACK_PACKAGE_DESCRIPTION}
                                       )
# Package settings
if(${UNIX})
  set(CPACK_GENERATOR                    "DEB;RPM")
  set(CPACK_CMAKE_GENERATOR              "Unix Makefiles")
  set(CPACK_PACKAGE_NAME                 ${PROJECT_NAME})
  set(CPACK_PACKAGE_FILE_NAME            ${PROJECT_NAME}-${VERSION_STRING}${PACK_ARCH})
endif(${UNIX})

if(${WIN32})
  set(CPACK_GENERATOR                    "NSIS")
  set(CPACK_CMAKE_GENERATOR              "MinGW Makefiles")
  set(CPACK_PACKAGE_NAME                 "${PROJECT_NAME}")
  set(CPACK_PACKAGE_VENDOR               "")
  set(CPACK_PACKAGE_INSTALL_DIRECTORY    "libftdi")
  set(CPACK_PACKAGE_FILE_NAME            "${PROJECT_NAME}-${VERSION_STRING}-win32")
  set(CPACK_NSIS_DISPLAY_NAME            "libftdi")
  set(CPACK_NSIS_MODIFY_PATH             "ON")
endif(${WIN32})

set(CPACK_RESOURCE_FILE_LICENSE        ${CMAKE_SOURCE_DIR}/LICENSE)

set(CPACK_SOURCE_GENERATOR             TGZ)
set(CPACK_SOURCE_IGNORE_FILES          "\\\\.svn" "build*")
set(CPACK_SOURCE_PACKAGE_FILE_NAME     ${CPACK_PACKAGE_FILE_NAME})

# Subdirectories
if(${UNIX})
  set(CPACK_SET_DESTDIR "ON")
endif(${UNIX})

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

#add_executable(debug debug.cpp iobase.cpp ioparport.cpp iodebug.cpp)

add_executable(bitparse bitrev.cpp bitfile.cpp bitparse.cpp)
add_executable(jedecparse  jedecparse.cpp jedecfile.cpp)
add_executable(srecparse  srecparse.cpp srecfile.cpp)
add_executable(avrfuseparse  avrfuseparse.cpp avrfusefile.cpp)

ADD_CUSTOM_COMMAND(OUTPUT devices.h
    COMMAND ${CMAKE_COMMAND} -DDEVLIST_DIR=${CMAKE_SOURCE_DIR} -P ${CMAKE_SOURCE_DIR}/devlist.cmk
    DEPENDS devlist.txt
)

ADD_CUSTOM_COMMAND(OUTPUT cables.h
    COMMAND ${CMAKE_COMMAND} -DCABLELIST_DIR=${CMAKE_SOURCE_DIR} -P ${CMAKE_SOURCE_DIR}/cablelist.cmk
    DEPENDS cablelist.txt
)

INCLUDE_DIRECTORIES(BEFORE ${CMAKE_CURRENT_BINARY_DIR})

add_library(xc3sproglib  STATIC ioparport.cpp ioftdi.cpp iofx2.cpp 
                      devicedb.cpp jtag.cpp jedecfile.cpp bitfile.cpp 
			iobase.cpp  progalgxc95x.cpp utilities.cpp
			progalgxcf.cpp progalgxcfp.cpp progalgxc3s.cpp
			progalgavr.cpp progalgxc2c.cpp  mapfile_xc2c.cpp
			ioxpc.cpp progalgspiflash.cpp bitrev.cpp
                        cabledb.cpp pdioverjtag.cpp xmega_pdi_nvm.cpp
                        ${CONDITIONAL_FILES} devices.h cables.h)

add_executable(xc2c_warp xc2c_warp.cpp)
target_link_libraries(xc2c_warp xc3sproglib ${CONDITIONAL_LIBS})

add_executable(detectchain detectchain.cpp cables.h devices.h)
target_link_libraries(detectchain xc3sproglib ${LIBFTDI_LIBRARIES} ${LIBFTD2XX_LIBRARIES} ${CONDITIONAL_LIBS})

add_executable(xc3sprog xc3sprog.cpp javr.cpp srecfile.cpp progalgavr.cpp 
			devices.h)
target_link_libraries(xc3sprog xc3sproglib ${LIBFTDI_LIBRARIES}  ${LIBFTD2XX_LIBRARIES} ${CONDITIONAL_LIBS}  )

add_executable(readdna readdna.cpp devices.h)
target_link_libraries(readdna xc3sproglib ${LIBFTDI_LIBRARIES}  ${LIBFTD2XX_LIBRARIES} ${CONDITIONAL_LIBS}  )

add_subdirectory(javr)

install(TARGETS xc3sprog DESTINATION bin)
install(TARGETS xc2c_warp DESTINATION bin)
install(TARGETS readdna DESTINATION bin)
install(TARGETS bitparse DESTINATION bin)
install(TARGETS jedecparse DESTINATION bin)
install(TARGETS srecparse DESTINATION bin)
install(TARGETS detectchain DESTINATION bin)
add_subdirectory(packages)
include(CPack)


SET_DIRECTORY_PROPERTIES(PROPERTIES ADDITIONAL_MAKE_CLEAN_FILES "${GENERATED_FILES}") 
