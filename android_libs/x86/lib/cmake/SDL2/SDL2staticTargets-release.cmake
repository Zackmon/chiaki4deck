#----------------------------------------------------------------
# Generated CMake target import file for configuration "Release".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "SDL2::SDL2-static" for configuration "Release"
set_property(TARGET SDL2::SDL2-static APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(SDL2::SDL2-static PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_RELEASE "C;CXX"
  IMPORTED_LOCATION_RELEASE "/home/zackmon/.ndk-pkg/installed/android-21-x86/5665a5b89b3c34e58de682ad0a0c945448ffe20a17b65f0b8e103ce6f8aecc64/lib/libSDL2.a"
  )

list(APPEND _cmake_import_check_targets SDL2::SDL2-static )
list(APPEND _cmake_import_check_files_for_SDL2::SDL2-static "/home/zackmon/.ndk-pkg/installed/android-21-x86/5665a5b89b3c34e58de682ad0a0c945448ffe20a17b65f0b8e103ce6f8aecc64/lib/libSDL2.a" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)