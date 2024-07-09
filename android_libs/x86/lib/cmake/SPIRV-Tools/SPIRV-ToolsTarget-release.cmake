#----------------------------------------------------------------
# Generated CMake target import file for configuration "Release".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "SPIRV-Tools-static" for configuration "Release"
set_property(TARGET SPIRV-Tools-static APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(SPIRV-Tools-static PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_RELEASE "CXX"
  IMPORTED_LOCATION_RELEASE "/home/zackmon/.ndk-pkg/installed/android-24-x86/8f9de66bab005362615ace2b526313a7e5c5632bfa9c10246deb3b77d95d5671/lib/libSPIRV-Tools.a"
  )

list(APPEND _cmake_import_check_targets SPIRV-Tools-static )
list(APPEND _cmake_import_check_files_for_SPIRV-Tools-static "/home/zackmon/.ndk-pkg/installed/android-24-x86/8f9de66bab005362615ace2b526313a7e5c5632bfa9c10246deb3b77d95d5671/lib/libSPIRV-Tools.a" )

# Import target "SPIRV-Tools-shared" for configuration "Release"
set_property(TARGET SPIRV-Tools-shared APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(SPIRV-Tools-shared PROPERTIES
  IMPORTED_LOCATION_RELEASE "/home/zackmon/.ndk-pkg/installed/android-24-x86/8f9de66bab005362615ace2b526313a7e5c5632bfa9c10246deb3b77d95d5671/lib/libSPIRV-Tools-shared.so"
  IMPORTED_SONAME_RELEASE "libSPIRV-Tools-shared.so"
  )

list(APPEND _cmake_import_check_targets SPIRV-Tools-shared )
list(APPEND _cmake_import_check_files_for_SPIRV-Tools-shared "/home/zackmon/.ndk-pkg/installed/android-24-x86/8f9de66bab005362615ace2b526313a7e5c5632bfa9c10246deb3b77d95d5671/lib/libSPIRV-Tools-shared.so" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
