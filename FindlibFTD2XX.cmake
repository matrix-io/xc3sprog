# Copyright 2009  SoftPLC Corporation  http://softplc.com
# Dick Hollenbeck <d...@softplc.com>
# License: GPL v2
#
# - Try to find libftd2xx
# Once done this will define
#
# LIBFTD2XX_FOUND - system has libftdi
# LIBFTD2XX_INCLUDE_DIR - the libftdi include directory
# LIBFTD2XX_LIBRARIES - Link these to use libftdi


if (NOT LIBFTD2XX_FOUND)

	if(NOT WIN32)
		include(FindPkgConfig)
		pkg_check_modules(LIBFTD2XX_PKG libftd2xx)
	endif(NOT WIN32)

	find_path(LIBFTD2XX_INCLUDE_DIR
		NAMES
			ftd2xx.h
                HINTS
 			${LIBFTD2XX_PKG_INCLUDE_DIRS}
		PATHS
			/usr/include
			/usr/local/include
	)

	find_library(LIBFTD2XX_LIBRARIES
		NAMES
			ftd2xx
                HINTS
			${LIBFTD2XX_PKG_LIBRARY_DIRS}
		PATHS
			/usr/lib
			/usr/local/lib
	)

	include(FindPackageHandleStandardArgs)

	# handle the QUIETLY AND REQUIRED arguments AND set LIBFTD2XX_FOUND to TRUE if
	# all listed variables are TRUE
	find_package_handle_standard_args(LIBFTD2XX DEFAULT_MSG LIBFTD2XX_LIBRARIES LIBFTD2XX_INCLUDE_DIR)

	#mark_as_advanced(LIBFTD2XX_INCLUDE_DIR LIBFTD2XX_LIBRARIES)

endif(NOT LIBFTD2XX_FOUND)
