# Tries to find wex include dir and libraries
#
# Usage of this module as follows:
#
#     find_package(WEX)
#
# Variables defined by this module:
#
#  wex_FOUND          System has wex libraries, include and library dirs found.
#  wex_INCLUDE_DIR    The wex include directory.
#  wex_LIB_DIR        The wex lib directory.
#  wex_LIBRARIES      The wex libraries.

set(Boost_USE_STATIC_LIBS ON)
set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_RUNTIME OFF)

find_package(Boost 1.65.0 COMPONENTS 
  filesystem program_options date_time regex REQUIRED)

find_package(JPEG)
find_package(PNG)
find_package(ZLIB)

find_package(ODBC QUIET)

if (ODBC_FOUND)
  add_definitions(-DwexUSE_OTL)
endif ()

if (WIN32)
  add_definitions(-D__WXMSW__)

  set(PLATFORM "msw")
elseif (APPLE AND IPHONE)
  add_definitions(-D__WXOSX_IPHONE__)

  set(PLATFORM "osx_iphone")
elseif (APPLE)
  add_definitions(-D__WXOSX_COCOA__)

  set(PLATFORM "osx_cocoa")

  set(cpp_LIBRARIES 
    stdc++)

  set(extra_LIBRARIES 
    wx_${PLATFORM}u_media-3.1 
    wxjpeg-3.1 
    wxpng-3.1)
elseif (UNIX)
  add_definitions(-D__WXGTK3__ -D__WXGTK__)

  set(PLATFORM "gtk3")

  if (CENTOS)
    set (cpp_std_LIBRARIES 
      /usr/gnat/lib64/libstdc++.a
      /usr/gnat/lib64/libstdc++fs.a)
  else ()
    set (cpp_std_LIBRARIES 
      stdc++
      stdc++fs)
  endif ()

  set(cpp_LIBRARIES
    ${cpp_std_LIBRARIES}
    ${JPEG_LIBRARIES}
    ${PNG_LIBRARIES}
    ${ZLIB_LIBRARIES}
    /usr/lib64/libSM.so 
    /usr/lib64/libICE.so 
    /usr/lib64/libX11.so 
    /usr/lib64/libXext.so 
    /usr/lib64/libXtst.so
    -lpthread 
    -ldl 
    -lc 
    -lgtk-3 
    -lgdk-3 
    -latk-1.0 
    -lgio-2.0 
    -lpangocairo-1.0 
    -lgdk_pixbuf-2.0 
    -lcairo-gobject 
    -lpango-1.0 
    -lcairo 
    -lgobject-2.0 
    -lglib-2.0)
else()
  message(FATAL_ERROR "Unsupported platform")
endif()

if (APPLE)
  set(CMAKE_EXE_LINKER_FLAGS "\
    -framework AudioToolbox \
    -framework WebKit \
    /usr/lib/libz.dylib \
    /usr/lib/libiconv.dylib \
    -framework CoreFoundation \
    -framework Security \
    -framework Carbon \
    -framework Cocoa \
    -framework IOKit")
endif()
      
if (MSVC)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} \
    /D_CRT_SECURE_NO_WARNINGS /D_CRT_SECURE_NO_DEPRECATE /D_SILENCE_ALL_CXX17_DEPRECATION_WARNINGS \
    /std:c++17 /Zc:__cplusplus")
else()
  set(SEPARATOR "_")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -g")
endif()
      
if (CMAKE_BUILD_TYPE EQUAL "Debug")
  set(USE_DEBUG "d")
endif() 

# these should be the same as in common.cmake
set(wex_INCLUDE_DIR "${CMAKE_INSTALL_PREFIX}/include/wex")
set(wex_LIB_DIR "${CMAKE_INSTALL_PREFIX}/lib/wex")
      
set(wex_LIBRARIES
  wex-report${USE_DEBUG}
  wex-common${USE_DEBUG}
  wex-stc${USE_DEBUG}
  wex-ui${USE_DEBUG}
  wex-vi${USE_DEBUG}
  wex-common${USE_DEBUG}
  wex-stc${USE_DEBUG}
  wex-ui${USE_DEBUG}
  wex-data${USE_DEBUG}
  wex-core${USE_DEBUG}
  wx${SEPARATOR}${PLATFORM}u_aui-3.1
  wx${SEPARATOR}${PLATFORM}u_adv-3.1
  wx${SEPARATOR}${PLATFORM}u_stc-3.1
  wx${SEPARATOR}${PLATFORM}u_html-3.1
  wx${SEPARATOR}${PLATFORM}u_core-3.1
  wx${SEPARATOR}baseu-3.1 
  wx${SEPARATOR}baseu_net-3.1 
  wxscintilla-3.1
  ${extra_LIBRARIES}
  ${Boost_LIBRARIES}
  ${cpp_LIBRARIES})
      
set(wex_FOUND ON)
