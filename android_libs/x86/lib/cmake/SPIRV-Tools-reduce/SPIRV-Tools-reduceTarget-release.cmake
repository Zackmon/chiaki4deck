#----------------------------------------------------------------
# Generated CMake target import file for configuration "Release".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "SPIRV-Tools-reduce" for configuration "Release"
set_property(TARGET SPIRV-Tools-reduce APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(SPIRV-Tools-reduce PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_RELEASE "CXX"
  IMPORTED_LOCATION_RELEASE "/home/zackmon/.ndk-pkg/installed/android-24-x86/8f9de66bab005362615ace2b526313a7e5c5632bfa9c10246deb3b77d95d5671/lib/libSPIRV-Tools-reduce.a"
  )

list(APPEND _cmake_import_check_targets SPIRV-Tools-reduce )
list(APPEND _cmake_import_check_files_for_SPIRV-Tools-reduce "/home/zackmon/.ndk-pkg/installed/android-24-x86/8f9de66bab005362615ace2b526313a7e5c5632bfa9c10246deb3b77d95d5671/lib/libSPIRV-Tools-reduce.a" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
