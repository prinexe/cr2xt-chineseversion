cmake_minimum_required(VERSION 3.14 FATAL_ERROR)
project(crqt-ng)

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules/")


#  VERSION_DEV_A/CODE description:
# alphaX - 0X; beta X - 1X; rcX - 2X; releaseX - 3X (or empty)
set(VERSION_MAJOR 1)
set(VERSION_MINOR 0)
set(VERSION_PATCH 15)
set(VERSION_DEV_A "")
set(VERSION_DEV_CODE 30)
set(VERSION "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}")
if (VERSION_DEV_A)
  set(VERSION "${VERSION}-${VERSION_DEV_A}")
endif()
# Use CR2XT_VERSION from umbrella project if available
if(CR2XT_VERSION)
  set(VERSION "${CR2XT_VERSION}")
endif()
set(PACKAGE_VERSION ${VERSION})
# for windows rc file
set(PRODUCTVERSION_V "${VERSION_MAJOR},${VERSION_MINOR},${VERSION_PATCH},${VERSION_DEV_CODE}")
set(FILEVERSION_V ${PRODUCTVERSION_V})

set(PACKAGE_DATADIR	${CMAKE_INSTALL_PREFIX}/share)

option(ADD_DEBUG_EXTRA_OPTS "Add extra debug flags and technique" OFF)
set(USE_QT QT5 CACHE STRING "Qt version to use")
set_property(CACHE USE_QT PROPERTY STRINGS QT5 QT6)
option(STATIC_CRENGINE_NG "Link with the static version of crengine-ng" OFF)

if (APPLE)
  if( ${CMAKE_SYSTEM} MATCHES "Darwin" )
    set(MACOS 1)
  endif()
endif()

configure_file(config.h.cmake ${CMAKE_BINARY_DIR}/config.h)

if( ${USE_QT} STREQUAL QT6 )
  # Find the Qt6 libraries
  find_package(Qt6Core REQUIRED)
  find_package(Qt6Gui REQUIRED)
  find_package(Qt6Widgets REQUIRED)
  include(Qt6TranslationToolsMacros)
elseif( ${USE_QT} STREQUAL QT5 )
  # Find the Qt5 libraries
  find_package(Qt5Core REQUIRED)
  find_package(Qt5Gui REQUIRED)
  find_package(Qt5Widgets REQUIRED)
  include(Qt5TranslationToolsMacros)
endif()

# crengine-ng - support both umbrella build and standalone build
if(NOT TARGET crengine-ng::crengine-ng AND NOT TARGET crengine-ng::crengine-ng_static)
  # Standalone build: find installed package
  find_package(crengine-ng 0.9.12 REQUIRED)
  include_directories(${CRENGINE_NG_INCLUDE_DIR})
else()
  # Umbrella build: targets already exist with PUBLIC include directories
  message(STATUS "Using crengine-ng from umbrella build")
endif()

# Add include path for config.h
include_directories(${CMAKE_BINARY_DIR})

# force use exceptions
set(CMAKE_CXX_FLAGS         "${CMAKE_CXX_FLAGS} -fexceptions")
# add debug defines
set(CMAKE_C_FLAGS_DEBUG     "${CMAKE_C_FLAGS_DEBUG}     -D_DEBUG")
set(CMAKE_CXX_FLAGS_DEBUG   "${CMAKE_CXX_FLAGS_DEBUG}   -D_DEBUG")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -DNDEBUG")

if(ADD_DEBUG_EXTRA_OPTS)
	add_definitions(-D_GLIBCXX_DEBUG -D_GLIBCXX_DEBUG_PEDANTIC)
	set(DEBUG_EXTRA_FLAGS "-fstack-check -fstack-protector -fno-omit-frame-pointer -Wshift-overflow=2")
	if ("${CMAKE_SYSTEM_NAME}" STREQUAL "Linux")
		set (DEBUG_EXTRA_FLAGS "${DEBUG_EXTRA_FLAGS} -fsanitize=address -fsanitize=undefined -fno-sanitize-recover -fno-sanitize=alignment")
		set(CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG} -fsanitize=address -fsanitize=undefined -fno-sanitize-recover -fstack-check -fstack-protector")
		set(CMAKE_SHARED_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG} -fsanitize=address -fsanitize=undefined -fno-sanitize-recover -fstack-check -fstack-protector")
	endif ("${CMAKE_SYSTEM_NAME}" STREQUAL "Linux")
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${DEBUG_EXTRA_FLAGS}")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${DEBUG_EXTRA_FLAGS}")
endif(ADD_DEBUG_EXTRA_OPTS)

if(UNIX)
  add_definitions(-DCRUI_DATA_DIR="${CMAKE_INSTALL_PREFIX}/share/crqt/")
else()
  add_definitions(-DCRUI_DATA_DIR="")
endif()

if (WIN32)
	set(CMAKE_RC_COMPILER windres)
	# set rc syntax
	set(CMAKE_RC_COMPILE_OBJECT "<CMAKE_RC_COMPILER> <FLAGS> <DEFINES> -O coff -o <OBJECT> -i <SOURCE>")
	set(CMAKE_RC_SOURCE_FILE_EXTENSIONS rc)
	set(CMAKE_RC_FLAGS "-I${CMAKE_BINARY_DIR}")
	# enable resource language for this project
	enable_language(RC)
endif(WIN32)

add_subdirectory(src)
