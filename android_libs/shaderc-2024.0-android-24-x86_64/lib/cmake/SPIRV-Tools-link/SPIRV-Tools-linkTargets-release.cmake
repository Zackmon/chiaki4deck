#----------------------------------------------------------------
# Generated CMake target import file for configuration "Release".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "SPIRV-Tools-link" for configuration "Release"
set_property(TARGET SPIRV-Tools-link APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(SPIRV-Tools-link PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_RELEASE "CXX"
  IMPORTED_LOCATION_RELEASE "/home/zackmon/.ndk-pkg/installed/android-24-x86_64/14be23cb29cac6b4048d5b495d542657ff0ca9314b94aa9839ac4aa99deb7795/lib/libSPIRV-Tools-link.a"
  )

list(APPEND _cmake_import_check_targets SPIRV-Tools-link )
list(APPEND _cmake_import_check_files_for_SPIRV-Tools-link "/home/zackmon/.ndk-pkg/installed/android-24-x86_64/14be23cb29cac6b4048d5b495d542657ff0ca9314b94aa9839ac4aa99deb7795/lib/libSPIRV-Tools-link.a" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
