# Find the MbedTLS library.
#
# This module defines the following variables:
#
# * MbedTLS_FOUND
#
# This module defines the following imported targets:
#
# * MbedTLS::MbedTLS
# * MbedTLS::x509
# * MbedTLS::crypto
#
# This module defines the following interfaces:
#
# * MbedTLS

find_path(MbedTLS_INCLUDE_DIR NAMES mbedtls/net.h)

find_library(MbedTLS_LIBRARY NAMES mbedtls)
find_library(MbedTLS_x509_LIBRARY NAMES mbedx509)
find_library(MbedTLS_crypto_LIBRARY NAMES mbedcrypto)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(
    MbedTLS
    REQUIRED_VARS
        MbedTLS_INCLUDE_DIR
        MbedTLS_LIBRARY
        MbedTLS_x509_LIBRARY
        MbedTLS_crypto_LIBRARY)
mark_as_advanced(
    MbedTLS_INCLUDE_DIR
    MbedTLS_LIBRARY
    MbedTLS_x509_LIBRARY
    MbedTLS_crypto_LIBRARY)

if(MbedTLS_FOUND AND NOT TARGET MbedTLS::MbedTLS)
    add_library(MbedTLS::MbedTLS UNKNOWN IMPORTED)
    set_target_properties(MbedTLS::MbedTLS PROPERTIES
        IMPORTED_LOCATION "${MbedTLS_LIBRARY}"
        INTERFACE_COMPILE_DEFINITIONS MbedTLS_FOUND
        INTERFACE_INCLUDE_DIRECTORIES "${MbedTLS_INCLUDE_DIR}")
endif()
if(MbedTLS_FOUND AND NOT TARGET MbedTLS::x509)
    add_library(MbedTLS::x509 UNKNOWN IMPORTED)
    set_target_properties(MbedTLS::x509 PROPERTIES
        IMPORTED_LOCATION "${MbedTLS_x509_LIBRARY}"
        INTERFACE_COMPILE_DEFINITIONS MbedTLS_x509_FOUND
        INTERFACE_INCLUDE_DIRECTORIES "${MbedTLS_INCLUDE_DIR}")
endif()
if(MbedTLS_FOUND AND NOT TARGET MbedTLS::crypto)
    add_library(MbedTLS::crypto UNKNOWN IMPORTED)
    set_target_properties(MbedTLS::crypto PROPERTIES
        IMPORTED_LOCATION "${MbedTLS_crypto_LIBRARY}"
        INTERFACE_COMPILE_DEFINITIONS MbedTLS_crypto_FOUND
        INTERFACE_INCLUDE_DIRECTORIES "${MbedTLS_INCLUDE_DIR}")
endif()
if(MbedTLS_FOUND AND NOT TARGET MbedTLS)
    add_library(MbedTLS INTERFACE)
    target_link_libraries(MbedTLS INTERFACE MbedTLS::MbedTLS)
    target_link_libraries(MbedTLS INTERFACE MbedTLS::x509)
    target_link_libraries(MbedTLS INTERFACE MbedTLS::crypto)
endif()

