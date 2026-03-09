#----------------------------------------------------------------
# Generated CMake target import file for configuration "Release".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "crengine-ng::crengine-ng" for configuration "Release"
set_property(TARGET crengine-ng::crengine-ng APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(crengine-ng::crengine-ng PROPERTIES
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/Library/Frameworks/crengine-ng.framework/Versions/A/crengine-ng"
  IMPORTED_SONAME_RELEASE "@rpath/crengine-ng.framework/Versions/A/crengine-ng"
  )

list(APPEND _cmake_import_check_targets crengine-ng::crengine-ng )
list(APPEND _cmake_import_check_files_for_crengine-ng::crengine-ng "${_IMPORT_PREFIX}/Library/Frameworks/crengine-ng.framework/Versions/A/crengine-ng" )

# Import target "crengine-ng::crengine-ng_static" for configuration "Release"
set_property(TARGET crengine-ng::crengine-ng_static APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(crengine-ng::crengine-ng_static PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_RELEASE "C;CXX"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libcrengine-ng.a"
  )

list(APPEND _cmake_import_check_targets crengine-ng::crengine-ng_static )
list(APPEND _cmake_import_check_files_for_crengine-ng::crengine-ng_static "${_IMPORT_PREFIX}/lib/libcrengine-ng.a" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
