# Adapter by Uwe Bonnes from
# Copyright 2009  SoftPLC Corporation  http://softplc.com
# Dick Hollenbeck <d...@softplc.com>
# License: GPL v2
#
# - Try to find libusb
#
# Before calling, USE_STATIC_USB may be set to mandate a STATIC library
#
# Once done this will define
#
# LIBUSB_FOUND - system has libusb
# LIBUSB_INCLUDE_DIR - the libusb include directory
# LIBUSB_LIBRARIES - Link these to use libusb


if (NOT LIBUSB_FOUND)

    if(NOT WIN32)
        include(FindPkgConfig)
        pkg_check_modules(LIBUSB_PKG libusb)
    endif(NOT WIN32)

    find_path(LIBUSB_INCLUDE_DIR
        NAMES
            usb.h
        HINTS
            ${LIBUSB_PKG_INCLUDE_DIRS}
        PATHS
            /usr/include
            /usr/local/include
            /usr/include/libftdi
            /usr/local/include/libftdi
    )

    if(USE_STATIC_USB)
        set(_save ${CMAKE_FIND_LIBRARY_SUFFIXES})
        set(CMAKE_FIND_LIBRARY_SUFFIXES ${CMAKE_STATIC_LIBRARY_SUFFIX})
    endif(USE_STATIC_USB)

    find_library(LIBUSB_LIBRARIES
        NAMES
            usb
        HINTS
            ${LIBUSB_PKG_LIBRARY_DIRS}
        PATHS
            /usr/lib
            /usr/local/lib
    )

    if(USE_STATIC_USB)
        set(CMAKE_FIND_LIBRARY_SUFFIXES ${_save} )
    endif(USE_STATIC_USB)
    include(FindPackageHandleStandardArgs)

    # handle the QUIETLY AND REQUIRED arguments AND set LIBUSB_FOUND to TRUE if
    # all listed variables are TRUE
    find_package_handle_standard_args(LIBUSB DEFAULT_MSG LIBUSB_LIBRARIES LIBUSB_INCLUDE_DIR)

    if(USE_STATIC_USB)
        add_library(libusb STATIC IMPORTED)
    else(USE_STATIC_USB)
        add_library(libusb SHARED IMPORTED)
    endif(USE_STATIC_USB)

    set_target_properties(libusb PROPERTIES IMPORTED_LOCATION ${LIBUSB_LIBRARIES})
    set(${LIBUSB_LIBRARIES} libusb)

    #mark_as_advanced(LIBUSB_INCLUDE_DIR LIBUSB_LIBRARIES)

endif(NOT LIBUSB_FOUND)
