#----------------------------------------------------------------
# Generated CMake target import file for configuration "Release".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "c-ares::cares" for configuration "Release"
set_property(TARGET c-ares::cares APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(c-ares::cares PROPERTIES
  IMPORTED_LOCATION_RELEASE "/home/zackmon/.ndk-pkg/installed/android-21-x86_64/ab7f27eab80642b761e0e7b4346009685bf07e653aacdd0e6202528ba41eca8c/lib/libcares.so"
  IMPORTED_SONAME_RELEASE "libcares.so"
  )

list(APPEND _cmake_import_check_targets c-ares::cares )
list(APPEND _cmake_import_check_files_for_c-ares::cares "/home/zackmon/.ndk-pkg/installed/android-21-x86_64/ab7f27eab80642b761e0e7b4346009685bf07e653aacdd0e6202528ba41eca8c/lib/libcares.so" )

# Import target "c-ares::cares_static" for configuration "Release"
set_property(TARGET c-ares::cares_static APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(c-ares::cares_static PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_RELEASE "C"
  IMPORTED_LOCATION_RELEASE "/home/zackmon/.ndk-pkg/installed/android-21-x86_64/ab7f27eab80642b761e0e7b4346009685bf07e653aacdd0e6202528ba41eca8c/lib/libcares_static.a"
  )

list(APPEND _cmake_import_check_targets c-ares::cares_static )
list(APPEND _cmake_import_check_files_for_c-ares::cares_static "/home/zackmon/.ndk-pkg/installed/android-21-x86_64/ab7f27eab80642b761e0e7b4346009685bf07e653aacdd0e6202528ba41eca8c/lib/libcares_static.a" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
