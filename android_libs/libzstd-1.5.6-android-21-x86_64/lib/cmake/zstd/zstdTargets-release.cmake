#----------------------------------------------------------------
# Generated CMake target import file for configuration "Release".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "zstd::libzstd_shared" for configuration "Release"
set_property(TARGET zstd::libzstd_shared APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(zstd::libzstd_shared PROPERTIES
  IMPORTED_LOCATION_RELEASE "/home/zackmon/.ndk-pkg/installed/android-21-x86_64/7686580b674107b2be00945b8e0902aa4d1d0d3272edce304e33d99b7474a668/lib/libzstd.so"
  IMPORTED_SONAME_RELEASE "libzstd.so"
  )

list(APPEND _cmake_import_check_targets zstd::libzstd_shared )
list(APPEND _cmake_import_check_files_for_zstd::libzstd_shared "/home/zackmon/.ndk-pkg/installed/android-21-x86_64/7686580b674107b2be00945b8e0902aa4d1d0d3272edce304e33d99b7474a668/lib/libzstd.so" )

# Import target "zstd::libzstd_static" for configuration "Release"
set_property(TARGET zstd::libzstd_static APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(zstd::libzstd_static PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_RELEASE "ASM;C"
  IMPORTED_LOCATION_RELEASE "/home/zackmon/.ndk-pkg/installed/android-21-x86_64/7686580b674107b2be00945b8e0902aa4d1d0d3272edce304e33d99b7474a668/lib/libzstd.a"
  )

list(APPEND _cmake_import_check_targets zstd::libzstd_static )
list(APPEND _cmake_import_check_files_for_zstd::libzstd_static "/home/zackmon/.ndk-pkg/installed/android-21-x86_64/7686580b674107b2be00945b8e0902aa4d1d0d3272edce304e33d99b7474a668/lib/libzstd.a" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
