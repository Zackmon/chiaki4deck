#----------------------------------------------------------------
# Generated CMake target import file for configuration "Release".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "SDL2::SDL2" for configuration "Release"
set_property(TARGET SDL2::SDL2 APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(SDL2::SDL2 PROPERTIES
  IMPORTED_LOCATION_RELEASE "/home/zackmon/.ndk-pkg/installed/android-21-x86_64/4dc19fcc84173fe86adf54619d529445d8fe1c932ec387561183e9e792df76c2/lib/libSDL2.so"
  IMPORTED_SONAME_RELEASE "libSDL2.so"
  )

list(APPEND _cmake_import_check_targets SDL2::SDL2 )
list(APPEND _cmake_import_check_files_for_SDL2::SDL2 "/home/zackmon/.ndk-pkg/installed/android-21-x86_64/4dc19fcc84173fe86adf54619d529445d8fe1c932ec387561183e9e792df76c2/lib/libSDL2.so" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
