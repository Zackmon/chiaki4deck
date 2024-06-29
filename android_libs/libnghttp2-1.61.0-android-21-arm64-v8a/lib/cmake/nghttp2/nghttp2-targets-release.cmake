#----------------------------------------------------------------
# Generated CMake target import file for configuration "Release".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "nghttp2::nghttp2" for configuration "Release"
set_property(TARGET nghttp2::nghttp2 APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(nghttp2::nghttp2 PROPERTIES
  IMPORTED_LOCATION_RELEASE "/home/zackmon/.ndk-pkg/installed/android-21-arm64-v8a/a733eacb2154a2e5498dbd00659c1a4426c4e2d04f51faeb0d6ff8a681c8b3a2/lib/libnghttp2.so"
  IMPORTED_SONAME_RELEASE "libnghttp2.so"
  )

list(APPEND _cmake_import_check_targets nghttp2::nghttp2 )
list(APPEND _cmake_import_check_files_for_nghttp2::nghttp2 "/home/zackmon/.ndk-pkg/installed/android-21-arm64-v8a/a733eacb2154a2e5498dbd00659c1a4426c4e2d04f51faeb0d6ff8a681c8b3a2/lib/libnghttp2.so" )

# Import target "nghttp2::nghttp2_static" for configuration "Release"
set_property(TARGET nghttp2::nghttp2_static APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(nghttp2::nghttp2_static PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_RELEASE "C"
  IMPORTED_LOCATION_RELEASE "/home/zackmon/.ndk-pkg/installed/android-21-arm64-v8a/a733eacb2154a2e5498dbd00659c1a4426c4e2d04f51faeb0d6ff8a681c8b3a2/lib/libnghttp2.a"
  )

list(APPEND _cmake_import_check_targets nghttp2::nghttp2_static )
list(APPEND _cmake_import_check_files_for_nghttp2::nghttp2_static "/home/zackmon/.ndk-pkg/installed/android-21-arm64-v8a/a733eacb2154a2e5498dbd00659c1a4426c4e2d04f51faeb0d6ff8a681c8b3a2/lib/libnghttp2.a" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
