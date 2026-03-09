
# Find includes in corresponding current build & source directories
set(CMAKE_INCLUDE_CURRENT_DIR ON)

# Instruct CMake to run moc automatically when needed.
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)

option(UPDATE_TRANSLATIONS "Run lupdate to update .ts translation files from sources" OFF)
option(CON_DEBUG "Enable console output for debug logging on Windows (disables -mwindows and WIN32)" OFF)

set(SRC_LIST
  aboutdlg.ui
  aboutdlg.cpp
  bookmarklistdlg.ui
  bookmarklistdlg.cpp
  crqtutil.cpp
  main.cpp
  recentdlg.ui
  recentdlg.cpp
  tocdlg.ui
  tocdlg.cpp
  addbookmarkdlg.ui
  addbookmarkdlg.cpp
  cr3widget.cpp
  filepropsdlg.ui
  filepropsdlg.cpp
  mainwindow.ui
  mainwindow.cpp
  settings.ui
  settings.cpp
  sampleview.cpp
  wolexportdlg.ui
  wolexportdlg.cpp
  exportprogressdlg.ui
  exportprogressdlg.cpp
  searchdlg.ui
  searchdlg.cpp
  fallbackfontsdialog.ui
  fallbackfontsdialog.cpp
  fontfamiliesdialog.ui
  fontfamiliesdialog.cpp
  tabdata.cpp
  tabscollection.cpp
  xtexportprofile.cpp
  xtexportdlg.ui
  xtexportdlg.cpp
  previewwidget.cpp
)

if(WIN32)
  set(SRC_LIST ${SRC_LIST} crqt.rc)
  set_property(SOURCE crqt.rc PROPERTY OBJECT_DEPENDS ${CMAKE_BINARY_DIR}/config.h)
endif(WIN32)

set(LANGUAGES ru uk cs bg hu nl)
message( "Languages: ${LANGUAGES}" )
set(LANGUAGE_TS_FILES)
foreach(LANGUAGE ${LANGUAGES})
  set(TS_FILE "${CMAKE_CURRENT_SOURCE_DIR}/i18n/crqt_${LANGUAGE}.ts")
  list(APPEND LANGUAGE_TS_FILES ${TS_FILE})
endforeach(LANGUAGE ${LANGUAGES})

if( ${USE_QT} STREQUAL QT6 )
  qt6_add_resources(CRQT_RCS crqtres.qrc)
elseif ( ${USE_QT} STREQUAL QT5 )
  qt5_add_resources(CRQT_RCS crqtres.qrc)
endif()

set(QM_FILES)
if(LANGUAGE_TS_FILES)
  if(UPDATE_TRANSLATIONS)
    # Run lupdate to extract strings from sources and update .ts files
    if( ${USE_QT} STREQUAL QT6 )
      qt6_update_translations(QM_FILES ${LANGUAGE_TS_FILES} ${SRC_LIST})
    elseif ( ${USE_QT} STREQUAL QT5 )
      qt5_update_translations(QM_FILES ${LANGUAGE_TS_FILES} ${SRC_LIST})
    endif()
  else()
    # Only compile existing .ts files to .qm (no source scanning)
    if( ${USE_QT} STREQUAL QT6 )
      qt6_add_translation(QM_FILES ${LANGUAGE_TS_FILES})
    elseif ( ${USE_QT} STREQUAL QT5 )
      qt5_add_translation(QM_FILES ${LANGUAGE_TS_FILES})
    endif()
  endif()
endif(LANGUAGE_TS_FILES)

if (WIN32)
  if (NOT CON_DEBUG)
    # -mwindows -> window application (without terminal screen)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -mwindows")
    add_executable(crqt WIN32 ${SRC_LIST} ${CRQT_RCS} ${QM_FILES})
  else()
    # Console debug mode - no -mwindows flag, no WIN32 keyword
    add_executable(crqt ${SRC_LIST} ${CRQT_RCS} ${QM_FILES})
  endif()
else()
  add_executable(crqt ${SRC_LIST} ${CRQT_RCS} ${QM_FILES})
endif (WIN32)

# macOS RPATH configuration
# BUILD_RPATH: allows running from IDE/build directory during development
# INSTALL_RPATH: for cmake --install (framework at Library/Frameworks/)
#   - dist script relocates to Contents/Frameworks/ and rewrites paths
if(MACOS)
  set_target_properties(crqt PROPERTIES
    MACOSX_RPATH ON
    BUILD_RPATH "${CMAKE_BINARY_DIR}/crengine-ng/crengine"
    INSTALL_RPATH "@executable_path/../../Library/Frameworks"
  )
endif()

if("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
  if (CMAKE_HOST_UNIX)
    configure_file(valgrind_check.sh.cmake ${CMAKE_CURRENT_BINARY_DIR}/valgrind_check.sh)
  endif(CMAKE_HOST_UNIX)
endif("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")

if(WIN32)
if(CMAKE_GENERATOR MATCHES "Visual Studio.*")
  message("Visual Studio generator detected")
  if (CON_DEBUG)
    # Console debug mode - always use CONSOLE subsystem
    set_target_properties(crqt PROPERTIES
        LINK_FLAGS_DEBUG "/SUBSYSTEM:CONSOLE")
    set_target_properties(crqt PROPERTIES
        LINK_FLAGS_RELWITHDEBINFO "/SUBSYSTEM:CONSOLE")
    set_target_properties(crqt PROPERTIES
        LINK_FLAGS_RELEASE "/SUBSYSTEM:CONSOLE")
    set_target_properties(crqt PROPERTIES
        LINK_FLAGS_MINSIZEREL "/SUBSYSTEM:CONSOLE")
  else()
    # Normal mode - debug builds use CONSOLE, release builds use WINDOWS
    set_target_properties(crqt PROPERTIES
        LINK_FLAGS_DEBUG "/SUBSYSTEM:CONSOLE")
    set_target_properties(crqt PROPERTIES
        LINK_FLAGS_RELWITHDEBINFO "/SUBSYSTEM:CONSOLE")
    set_target_properties(crqt PROPERTIES
        LINK_FLAGS_RELEASE "/SUBSYSTEM:WINDOWS")
    set_target_properties(crqt PROPERTIES
        LINK_FLAGS_MINSIZEREL "/SUBSYSTEM:WINDOWS")
  endif()
endif(CMAKE_GENERATOR MATCHES "Visual Studio.*")
endif(WIN32)

if(STATIC_CRENGINE_NG)
  set(CRE_NG crengine-ng::crengine-ng_static)
else()
  set(CRE_NG crengine-ng::crengine-ng)
endif()

if( ${USE_QT} STREQUAL QT6 )
  target_link_libraries(crqt ${CRE_NG} Qt6::Core Qt6::Gui Qt6::Widgets)
elseif ( ${USE_QT} STREQUAL QT5 )
  target_link_libraries(crqt ${CRE_NG} Qt5::Core Qt5::Gui Qt5::Widgets)
endif()

if (MACOS)
  install( TARGETS crqt RUNTIME DESTINATION Contents/MacOS )
  #install( DIRECTORY ../data/skins DESTINATION share/crqt/skins )
  install( DIRECTORY ../data/textures DESTINATION Contents/Resources )
  install( DIRECTORY ../data/backgrounds DESTINATION Contents/Resources )
  install( FILES ${QM_FILES} DESTINATION Contents/Resources/i18n )
  install( FILES desktop/crqt.icns DESTINATION Contents/Resources )
  configure_file(desktop/Info.plist.cmake ${CMAKE_CURRENT_BINARY_DIR}/Info.plist)
  install( FILES ${CMAKE_CURRENT_BINARY_DIR}/Info.plist DESTINATION Contents )
  install( FILES desktop/PkgInfo DESTINATION Contents )
  # install crengine-ng data files (css files go directly to Resources, not in css/ subdirectory)
  install( DIRECTORY ${CRENGINE_NG_DATA_DIR}/css/ DESTINATION Contents/Resources FILES_MATCHING PATTERN "*.css" )
  install( DIRECTORY ${CRENGINE_NG_DATA_DIR}/hyph/ DESTINATION Contents/Resources/hyph FILES_MATCHING PATTERN "*.pattern" )
elseif (UNIX)
  install( TARGETS crqt RUNTIME DESTINATION bin )
  #install( DIRECTORY ../data/skins DESTINATION share/crqt/skins )
  install( DIRECTORY ../data/textures DESTINATION share/crqt )
  install( DIRECTORY ../data/backgrounds DESTINATION share/crqt )
  install( FILES ${QM_FILES} DESTINATION share/crqt/i18n )
  install( FILES desktop/crqt.desktop DESTINATION share/applications )
  install( FILES desktop/crqt.appdata.xml DESTINATION share/metainfo )
  install( FILES desktop/crqt.png DESTINATION share/pixmaps )
  install( FILES desktop/crqt.xpm DESTINATION share/pixmaps )
  install( FILES desktop/crqt.png DESTINATION share/icons/hicolor/48x48/apps )
  install( FILES icons/crqt.svg DESTINATION share/icons/hicolor/scalable/apps )
else()
  install( TARGETS crqt RUNTIME DESTINATION . )
  #install( DIRECTORY ../data/skins DESTINATION . )
  install( DIRECTORY ../data/textures DESTINATION . )
  install( DIRECTORY ../data/backgrounds DESTINATION . )
  install( FILES ${QM_FILES} DESTINATION i18n )
endif()
