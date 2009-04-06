# - Try to find libftdi
# Once done this will define
#
# LIBFTDI_FOUND - system has libftdi
# LIBFTDI_INCLUDE_DIRS - the libftdi include directory
# LIBFTDI_LIBRARIES - Link these to use libftdi
# LIBFTDI_DEFINITIONS - Compiler switches required for using libftdi
#
# Adapted from cmake-modules Google Code project
#
# Copyright (c) 2006 Andreas Schneider <mail@cynapses.org>
#
# (Changes for libftdi) Copyright (c) 2008 Kyle Machulis <kyle@nonpolynomial.com>
#
# Redistribution and use is allowed according to the terms of the New BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.
#
 
 
if (LIBFTDI_LIBRARIES AND LIBFTDI_INCLUDE_DIRS)
  # in cache already
  set(LIBFTDI_FOUND TRUE)
else (LIBFTDI_LIBRARIES AND LIBFTDI_INCLUDE_DIRS)
  find_path(LIBFTDI_INCLUDE_DIR
    NAMES
      ftdi.h
    PATHS
      /usr/include
      /usr/local/include
      /opt/local/include
      /sw/include
  )
 
  find_library(LIBFTDI_LIBRARY
    NAMES
      ftdi
    PATHS
      /usr/lib
      /usr/local/lib
      /opt/local/lib
      /sw/lib
  )
 
  set(LIBFTDI_INCLUDE_DIRS
    ${LIBFTDI_INCLUDE_DIR}
  )
  set(LIBFTDI_LIBRARIES
    ${LIBFTDI_LIBRARY}
)
 
  if (LIBFTDI_INCLUDE_DIRS AND LIBFTDI_LIBRARIES)
     set(LIBFTDI_FOUND TRUE)
  endif (LIBFTDI_INCLUDE_DIRS AND LIBFTDI_LIBRARIES)
 
  if (LIBFTDI_FOUND)
    if (NOT libftdi_FIND_QUIETLY)
      message(STATUS "Found libftdi: ${LIBFTDI_LIBRARIES}")
    endif (NOT libftdi_FIND_QUIETLY)
  else (LIBFTDI_FOUND)
    if (libftdi_FIND_REQUIRED)
      message(FATAL_ERROR "Could not find libftdi")
    endif (libftdi_FIND_REQUIRED)
  endif (LIBFTDI_FOUND)
 
  # show the LIBFTDI_INCLUDE_DIRS and LIBFTDI_LIBRARIES variables only in the advanced view
  mark_as_advanced(LIBFTDI_INCLUDE_DIRS LIBFTDI_LIBRARIES)
 
endif (LIBFTDI_LIBRARIES AND LIBFTDI_INCLUDE_DIRS)
