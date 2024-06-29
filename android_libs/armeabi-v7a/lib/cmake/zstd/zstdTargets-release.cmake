#----------------------------------------------------------------
# Generated CMake target import file for configuration "Release".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "zstd::libzstd_shared" for configuration "Release"
set_property(TARGET zstd::libzstd_shared APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(zstd::libzstd_shared PROPERTIES
  IMPORTED_LOCATION_RELEASE "/home/zackmon/.ndk-pkg/installed/android-21-armeabi-v7a/83c07c7fc85944b4168694a66f0bea61ace0c6caaa0e8c689d036a1b4a78703d/lib/libzstd.so"
  IMPORTED_SONAME_RELEASE "libzstd.so"
  )

list(APPEND _cmake_import_check_targets zstd::libzstd_shared )
list(APPEND _cmake_import_check_files_for_zstd::libzstd_shared "/home/zackmon/.ndk-pkg/installed/android-21-armeabi-v7a/83c07c7fc85944b4168694a66f0bea61ace0c6caaa0e8c689d036a1b4a78703d/lib/libzstd.so" )

# Import target "zstd::libzstd_static" for configuration "Release"
set_property(TARGET zstd::libzstd_static APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(zstd::libzstd_static PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_RELEASE "C"
  IMPORTED_LOCATION_RELEASE "/home/zackmon/.ndk-pkg/installed/android-21-armeabi-v7a/83c07c7fc85944b4168694a66f0bea61ace0c6caaa0e8c689d036a1b4a78703d/lib/libzstd.a"
  )

list(APPEND _cmake_import_check_targets zstd::libzstd_static )
list(APPEND _cmake_import_check_files_for_zstd::libzstd_static "/home/zackmon/.ndk-pkg/installed/android-21-armeabi-v7a/83c07c7fc85944b4168694a66f0bea61ace0c6caaa0e8c689d036a1b4a78703d/lib/libzstd.a" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
