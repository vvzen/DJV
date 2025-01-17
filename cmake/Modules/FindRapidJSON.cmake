# Find the RapidJSON library.
#
# This module defines the following variables:
#
# * RapidJSON_FOUND
#
# This module defines the following interfaces:
#
# * RapidJSON

find_path(RapidJSON_INCLUDE_DIR rapidjson/rapidjson.h)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(
    RapidJSON
    REQUIRED_VARS RapidJSON_INCLUDE_DIR)
mark_as_advanced(RapidJSON_INCLUDE_DIR)

if(RapidJSON_FOUND AND NOT TARGET RapidJSON)
    add_library(RapidJSON INTERFACE)
    set_target_properties(RapidJSON PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${RapidJSON_INCLUDE_DIR}")
endif()
