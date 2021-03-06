cmake_minimum_required(VERSION 2.8.8)
project(libsdr)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${PROJECT_SOURCE_DIR}/cmake)
include(InstallHeadersWithDirectory)
include(LinkResources)

OPTION(BUILD_EXAMPLES "Build examples" OFF)
OPTION(BUILD_COMMANDLINETOOL "Build command line tool" ON)
option(BUILD_DOC "Build documentation" OFF)

SET(libsdr_VERSION_MAJOR "0")
SET(libsdr_VERSION_MINOR "1")
SET(libsdr_VERSION_PATCH "0")

find_package(FFTW)
find_package(FFTWSingle)
find_package(PortAudio)
find_package(RTLSDR)
if(WIN32)
  find_package(PTHREAD REQUIRED)
endif()
if(BUILD_DOC)
  find_package(Doxygen REQUIRED)
endif()

INCLUDE_DIRECTORIES(${PROJECT_SOURCE_DIR}/src)
INCLUDE_DIRECTORIES(${PROJECT_BINARY_DIR}/src)
if(WIN32)
  include_directories(${PTHREAD_INCLUDE_DIR})
endif()
INCLUDE_DIRECTORIES(${GETOPT_INCLUDE_DIRS})

# Set some variables for the configuration file
IF(FFTW_FOUND)
  set(SDR_WITH_FFTW ON)
  include_directories(${FFTW_INCLUDES})
ELSE(FFTW_FOUND)
  set(FFTW_LIBRARIES)
  set(FFTWSingle_LIBRARIES)
ENDIF(FFTW_FOUND)

IF(PORTAUDIO_FOUND)
  set(SDR_WITH_PORTAUDIO ON)
  include_directories(${PORTAUDIO_INCLUDE_DIRS})
ELSE(PORTAUDIO_FOUND)
  set(PORTAUDIO_LIBRARIES)
ENDIF(PORTAUDIO_FOUND)

IF(RTLSDR_FOUND)
  set(SDR_WITH_RTLSDR ON)
  include_directories(${RTLSDR_INCLUDES})
  option(RTLSDR_STATIC "Use static librtlsdr library" OFF)
  if(RTLSDR_STATIC)
    add_definitions(-Drtlsdr_STATIC)
  endif()
ELSE(RTLSDR_FOUND)
  set(RTLSDR_LIBRARIES)
ENDIF(RTLSDR_FOUND)


set(LIBS ${FFTW_LIBRARIES} ${FFTWSingle_LIBRARIES} ${PORTAUDIO_LIBRARIES} ${RTLSDR_LIBRARIES})
if(WIN32)
  list(APPEND LIBS ${PTHREAD_LIBRARY})
else()
  list(APPEND LIBS "pthread")
endif()

# Set compiler flags
if(NOT MSVC)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -fPIC")
  set(CMAKE_CXX_FLAGS_DEBUG   "${CMAKE_CXX_FLAGS_DEBUG} -O0 -ggdb")
  set(CMAKE_CXX_FLAGS_DEBUG   "${CMAKE_CXX_FLAGS_DEBUG} -O0 -ggdb")
  set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -ggdb")
else()
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd4514 /wd4267")
  add_definitions(-D_WINSOCK_DEPRECATED_NO_WARNINGS -D_CRT_SECURE_NO_WARNINGS)
endif()

LINK_DIRECTORIES(${PROJECT_BINARY_DIR}/src)

if(WIN32)
  include (TestBigEndian)
  TEST_BIG_ENDIAN(IS_BIG_ENDIAN)
  if(IS_BIG_ENDIAN)
    add_definitions(-D__BIG_ENDIAN__)
  else()
    add_definitions(-D__LITTLE_ENDIAN__)
  endif()
endif()

# Create config.hh
CONFIGURE_FILE(${CMAKE_CURRENT_SOURCE_DIR}/src/config.hh.in
               ${CMAKE_CURRENT_BINARY_DIR}/src/config.hh)


#
# Get default install directories under Linux
#
IF(UNIX AND NOT APPLE)
 INCLUDE(GNUInstallDirs)
ENDIF(UNIX AND NOT APPLE)
IF(UNIX AND APPLE)
 SET(CMAKE_INSTALL_LIBDIR "lib")
 SET(CMAKE_INSTALL_FULL_LIBDIR "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}")
 SET(CMAKE_INSTALL_INCLUDEDIR "include")
 SET(CMAKE_INSTALL_FULL_INCLUDEDIR "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_INCLUDEDIR}")
 # Set RPATH under MacOS
 SET(CMAKE_SKIP_RPATH FALSE)
 SET(CMAKE_SKIP_BUILD_RPATH FALSE)
 SET(CMAKE_INSTALL_RPATH ${CMAKE_INSTALL_FULL_LIBDIR})
 SET(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE)
 SET(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
ENDIF(UNIX AND APPLE)

# Add core library, and unit tests
add_subdirectory(src)

include(CTest)
IF(BUILD_TESTING)
 enable_testing()
 add_subdirectory(test)
ENDIF(BUILD_TESTING)

IF(BUILD_EXAMPLES)
 add_subdirectory(examples)
ENDIF(BUILD_EXAMPLES)

IF(BUILD_COMMANDLINETOOL)
 add_subdirectory(cmd)
endif(BUILD_COMMANDLINETOOL)

if(BUILD_DOC)
  if(DOXYGEN_DOT_FOUND)
    set(HAVE_DOT "YES")
  else()
    set(HAVE_DOT "NO")
  endif()
  configure_file(${CMAKE_CURRENT_SOURCE_DIR}/cmake/Doxyfile.in ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile)
  add_custom_target(doc_doxygen ALL
        COMMAND ${DOXYGEN_EXECUTABLE} Doxyfile
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        COMMENT "Generating API documentation with Doxygen"
        VERBATIM)
  install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/doc TYPE DATA PATTERN "*.md5" EXCLUDE PATTERN "*.map" EXCLUDE)
endif()

# Source distribution packages:
set(CPACK_PACKAGE_VERSION_MAJOR ${libsdr_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${libsdr_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${libsdr_VERSION_PATCH})
set(CPACK_SOURCE_GENERATOR "TGZ")
set(CPACK_SOURCE_PACKAGE_FILE_NAME
  "${CMAKE_PROJECT_NAME}-${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH}")
set(CPACK_SOURCE_IGNORE_FILES
  "/build/;/doc/;/dist/;/.git/;.dat$;.wav$;~$;.qm$;${CPACK_SOURCE_IGNORE_FILES}")

include(CPack)
