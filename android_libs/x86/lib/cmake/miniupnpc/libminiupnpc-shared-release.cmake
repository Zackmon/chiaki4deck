#----------------------------------------------------------------
# Generated CMake target import file for configuration "Release".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "miniupnpc::miniupnpc" for configuration "Release"
set_property(TARGET miniupnpc::miniupnpc APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(miniupnpc::miniupnpc PROPERTIES
  IMPORTED_LOCATION_RELEASE "/home/zackmon/.ndk-pkg/installed/android-21-x86/077fb009f7ce9901ea9c38d5081f595157e936b68e7ca5ecb06815957d1fb490/lib/libminiupnpc.so"
  IMPORTED_SONAME_RELEASE "libminiupnpc.so"
  )

list(APPEND _cmake_import_check_targets miniupnpc::miniupnpc )
list(APPEND _cmake_import_check_files_for_miniupnpc::miniupnpc "/home/zackmon/.ndk-pkg/installed/android-21-x86/077fb009f7ce9901ea9c38d5081f595157e936b68e7ca5ecb06815957d1fb490/lib/libminiupnpc.so" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
