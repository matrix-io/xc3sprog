# Project
cmake_minimum_required (VERSION 2.8.4 FATAL_ERROR)

project(xc3sprog C CXX)
if (CMAKE_VERSION VERSION_LESS 100)
  # C language still needed because the following required CMake modules
  # (or their dependencies, respectively) are not correctly handling
  # the case where only CXX is enabled.
  # - CheckTypeSize.cmake (fixed in CMake 3.1, cf. http://www.cmake.org/Bug/view.php?id=14056)
  # - FindThreads.cmake (--> CheckIncludeFiles.cmake <--)
  enable_language (C)
endif ()


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

add_library(xc3sproglib  STATIC 
  sysfs.cpp 
  devicedb.cpp 
  jtag.cpp 
  jedecfile.cpp 
  bitfile.cpp 
  iobase.cpp
  utilities.cpp
  progalgxc3s.cpp
  bitrev.cpp
  cabledb.cpp
  xc3sprog.cpp
  srecfile.cpp
  xc3loader.cpp
  ${CONDITIONAL_FILES} devices.h cables.h xc3loader.h sysfs.h)

add_executable(xc3sprog simple_main.cpp devices.h sysfs.h)
target_link_libraries(xc3sprog xc3sproglib log ${CONDITIONAL_LIBS}  ${LIBS} )
set_property(TARGET xc3sprog PROPERTY CXX_STANDARD 11)

install(TARGETS xc3sprog DESTINATION bin)


SET_DIRECTORY_PROPERTIES(PROPERTIES ADDITIONAL_MAKE_CLEAN_FILES "${GENERATED_FILES}") 

if(ANDROID)
  add_definitions(-frtti -fexceptions -fpermissive -std=c++11)
  add_library(xc3loader SHARED xc3loader.cpp android_utils.cpp sysfs.cpp)
  set_property(TARGET xc3loader PROPERTY CXX_STANDARD 11)
  target_link_libraries(xc3loader log)
  target_link_libraries(xc3loader xc3sproglib)
  # set back RUNTIME_OUTPUT_DIRECTORY
  set_target_properties(xc3loader PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/android")
  # install
  install(TARGETS xc3loader DESTINATION lib)
endif()

