#----------------------------------------------------------------
# Generated CMake target import file for configuration "Release".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "json-c::json-c" for configuration "Release"
set_property(TARGET json-c::json-c APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(json-c::json-c PROPERTIES
  IMPORTED_LOCATION_RELEASE "/home/zackmon/.ndk-pkg/installed/android-21-x86/9b1c48b57d00c7c7633c24b08291908579834eb2115a2ca3d468c2bee30cb0f0/lib/libjson-c.so"
  IMPORTED_SONAME_RELEASE "libjson-c.so"
  )

list(APPEND _cmake_import_check_targets json-c::json-c )
list(APPEND _cmake_import_check_files_for_json-c::json-c "/home/zackmon/.ndk-pkg/installed/android-21-x86/9b1c48b57d00c7c7633c24b08291908579834eb2115a2ca3d468c2bee30cb0f0/lib/libjson-c.so" )

# Import target "json-c::json-c-static" for configuration "Release"
set_property(TARGET json-c::json-c-static APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(json-c::json-c-static PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_RELEASE "C"
  IMPORTED_LOCATION_RELEASE "/home/zackmon/.ndk-pkg/installed/android-21-x86/9b1c48b57d00c7c7633c24b08291908579834eb2115a2ca3d468c2bee30cb0f0/lib/libjson-c.a"
  )

list(APPEND _cmake_import_check_targets json-c::json-c-static )
list(APPEND _cmake_import_check_files_for_json-c::json-c-static "/home/zackmon/.ndk-pkg/installed/android-21-x86/9b1c48b57d00c7c7633c24b08291908579834eb2115a2ca3d468c2bee30cb0f0/lib/libjson-c.a" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
