# Install script for directory: /Users/pri/Desktop/cursor/cr2xt/crengine-ng/crengine

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "/usr/local")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "Release")
  endif()
  message(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
endif()

# Set the component getting installed.
if(NOT CMAKE_INSTALL_COMPONENT)
  if(COMPONENT)
    message(STATUS "Install component: \"${COMPONENT}\"")
    set(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  else()
    set(CMAKE_INSTALL_COMPONENT)
  endif()
endif()

# Is this installation the result of a crosscompile?
if(NOT DEFINED CMAKE_CROSSCOMPILING)
  set(CMAKE_CROSSCOMPILING "FALSE")
endif()

# Set path to fallback-tool for dependency-resolution.
if(NOT DEFINED CMAKE_OBJDUMP)
  set(CMAKE_OBJDUMP "/usr/bin/objdump")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "headers" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/crengine-ng" TYPE DIRECTORY FILES "/Users/pri/Desktop/cursor/cr2xt/crengine-ng/crengine/include/")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "headers" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/crengine-ng" TYPE FILE FILES "/Users/pri/Desktop/cursor/cr2xt/crengine-ng/build_standalone/crengine/crengine-ng-config.h")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "libraries" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/Library/Frameworks" TYPE DIRECTORY FILES "/Users/pri/Desktop/cursor/cr2xt/crengine-ng/build_standalone/crengine/crengine-ng.framework" USE_SOURCE_PERMISSIONS)
  if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/Library/Frameworks/crengine-ng.framework/Versions/A/crengine-ng" AND
     NOT IS_SYMLINK "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/Library/Frameworks/crengine-ng.framework/Versions/A/crengine-ng")
    execute_process(COMMAND /usr/bin/install_name_tool
      -delete_rpath "/opt/homebrew/lib"
      "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/Library/Frameworks/crengine-ng.framework/Versions/A/crengine-ng")
    if(CMAKE_INSTALL_DO_STRIP)
      execute_process(COMMAND "/usr/bin/strip" -x "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/Library/Frameworks/crengine-ng.framework/Versions/A/crengine-ng")
    endif()
  endif()
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY FILES "/Users/pri/Desktop/cursor/cr2xt/crengine-ng/build_standalone/crengine/libcrengine-ng.a")
  if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libcrengine-ng.a" AND
     NOT IS_SYMLINK "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libcrengine-ng.a")
    execute_process(COMMAND "/usr/bin/ranlib" "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libcrengine-ng.a")
  endif()
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "headers" OR NOT CMAKE_INSTALL_COMPONENT)
  if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/cmake/crengine-ng/crengine-ng-targets.cmake")
    file(DIFFERENT _cmake_export_file_changed FILES
         "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/cmake/crengine-ng/crengine-ng-targets.cmake"
         "/Users/pri/Desktop/cursor/cr2xt/crengine-ng/build_standalone/crengine/CMakeFiles/Export/ab184a57a2f89a3052ab2d1efc50e3ea/crengine-ng-targets.cmake")
    if(_cmake_export_file_changed)
      file(GLOB _cmake_old_config_files "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/cmake/crengine-ng/crengine-ng-targets-*.cmake")
      if(_cmake_old_config_files)
        string(REPLACE ";" ", " _cmake_old_config_files_text "${_cmake_old_config_files}")
        message(STATUS "Old export file \"$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/cmake/crengine-ng/crengine-ng-targets.cmake\" will be replaced.  Removing files [${_cmake_old_config_files_text}].")
        unset(_cmake_old_config_files_text)
        file(REMOVE ${_cmake_old_config_files})
      endif()
      unset(_cmake_old_config_files)
    endif()
    unset(_cmake_export_file_changed)
  endif()
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/cmake/crengine-ng" TYPE FILE FILES "/Users/pri/Desktop/cursor/cr2xt/crengine-ng/build_standalone/crengine/CMakeFiles/Export/ab184a57a2f89a3052ab2d1efc50e3ea/crengine-ng-targets.cmake")
  if(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Rr][Ee][Ll][Ee][Aa][Ss][Ee])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/cmake/crengine-ng" TYPE FILE FILES "/Users/pri/Desktop/cursor/cr2xt/crengine-ng/build_standalone/crengine/CMakeFiles/Export/ab184a57a2f89a3052ab2d1efc50e3ea/crengine-ng-targets-release.cmake")
  endif()
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "headers" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/cmake/crengine-ng" TYPE FILE FILES
    "/Users/pri/Desktop/cursor/cr2xt/crengine-ng/build_standalone/crengine/crengine-ng-config.cmake"
    "/Users/pri/Desktop/cursor/cr2xt/crengine-ng/build_standalone/crengine/crengine-ng-config-version.cmake"
    )
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "pkgconfig" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/pkgconfig" TYPE FILE FILES "/Users/pri/Desktop/cursor/cr2xt/crengine-ng/build_standalone/crengine/crengine-ng.pc")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "css" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/crengine-ng" TYPE DIRECTORY FILES "/Users/pri/Desktop/cursor/cr2xt/crengine-ng/crengine/data/css/" FILES_MATCHING REGEX "/[^/]*\\.css$")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "hyph" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/crengine-ng/hyph" TYPE DIRECTORY FILES "/Users/pri/Desktop/cursor/cr2xt/crengine-ng/crengine/data/hyph/" FILES_MATCHING REGEX "/[^/]*\\.pattern$")
endif()

string(REPLACE ";" "\n" CMAKE_INSTALL_MANIFEST_CONTENT
       "${CMAKE_INSTALL_MANIFEST_FILES}")
if(CMAKE_INSTALL_LOCAL_ONLY)
  file(WRITE "/Users/pri/Desktop/cursor/cr2xt/crengine-ng/build_standalone/crengine/install_local_manifest.txt"
     "${CMAKE_INSTALL_MANIFEST_CONTENT}")
endif()
