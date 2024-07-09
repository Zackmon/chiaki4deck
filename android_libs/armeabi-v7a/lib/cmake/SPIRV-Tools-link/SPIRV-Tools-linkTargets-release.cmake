#----------------------------------------------------------------
# Generated CMake target import file for configuration "Release".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "SPIRV-Tools-link" for configuration "Release"
set_property(TARGET SPIRV-Tools-link APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(SPIRV-Tools-link PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_RELEASE "CXX"
  IMPORTED_LOCATION_RELEASE "/home/zackmon/.ndk-pkg/installed/android-24-armeabi-v7a/a407f7c8701b1065fcdba8dc3ced7c8ffb87251e227da28e36bf179884ff538d/lib/libSPIRV-Tools-link.a"
  )

list(APPEND _cmake_import_check_targets SPIRV-Tools-link )
list(APPEND _cmake_import_check_files_for_SPIRV-Tools-link "/home/zackmon/.ndk-pkg/installed/android-24-armeabi-v7a/a407f7c8701b1065fcdba8dc3ced7c8ffb87251e227da28e36bf179884ff538d/lib/libSPIRV-Tools-link.a" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
