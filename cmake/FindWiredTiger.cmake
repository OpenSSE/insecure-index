# Try to find WiredTiger headers and library.
#
# Usage of this module as follows:
#
# find_package(WiredTiger)
#
# Variables used by this module, they can change the default behaviour and need
# to be set before calling find_package:
#
# WIREDTIGER_ROOT_DIR      Set this variable to the root installation of
# WiredTiger if the module has problems finding the proper installation path.
#
# Variables defined by this module:
#
# WIREDTIGER_FOUND              System has WiredTiger library/headers.
# WIREDTIGER_LIBRARIES          The WiredTiger library.
# WIREDTIGER_INCLUDE_DIRS       The location of WiredTiger headers.

find_path(WIREDTIGER_ROOT_DIR NAMES include/wiredtiger.h)

find_library(
    WIREDTIGER_LIBRARIES
    NAMES wiredtiger
    HINTS ${WIREDTIGER_ROOT_DIR}/lib
)

find_path(
    WIREDTIGER_INCLUDE_DIRS
    NAMES wiredtiger.h
    HINTS ${WIREDTIGER_ROOT_DIR}/include
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(
    WiredTiger
    DEFAULT_MSG
    WIREDTIGER_LIBRARIES
    WIREDTIGER_INCLUDE_DIRS
)

mark_as_advanced(
    WIREDTIGER_ROOT_DIR WIREDTIGER_LIBRARIES WIREDTIGER_INCLUDE_DIRS
)
