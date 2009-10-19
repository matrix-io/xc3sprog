# Debian
if("${PACKAGE}" STREQUAL "Debian")

   # Settings
   set(REVISION                           0)
   set(CPACK_GENERATOR                    "DEB" PARENT_SCOPE)
   set(CPACK_PACKAGE_VERSION              ${CPACK_PACKAGE_VERSION}-${REVISION} PARENT_SCOPE)

   # Dependencies
   set(CPACK_DEBIAN_PACKAGE_DEPENDS       "libusb-0.1-4" PARENT_SCOPE)
   set(DEBIAN_PACKAGE_BUILDS_DEPENDS      "cmake, libusb-dev" PARENT_SCOPE)

endif("${PACKAGE}" STREQUAL "Debian")

# General RPM rules
set(CPACK_RPM_PACKAGE_DEPENDS       "libusb" "libftdi" PARENT_SCOPE)
