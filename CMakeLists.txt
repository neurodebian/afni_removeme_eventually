PROJECT(AFNI)

cmake_minimum_required(VERSION 2.6)
if(COMMAND cmake_policy)
  cmake_policy(SET CMP0002 NEW)
  cmake_policy(SET CMP0003 NEW)
endif(COMMAND cmake_policy)

##
## Define options to customize the build-process
##

OPTION(AFNI_BUILD_LOCAL_NIFTICLIBS
       "Build and use AFNI's local copy of the NIfTI libs"
       OFF)
OPTION(AFNI_BUILD_LOCAL_3DEGDE3
       "Build and use AFNI's local copy of lib3dedge3"
       OFF)
OPTION(AFNI_BUILD_LOCAL_GIFTI
       "Build and use AFNI's local copy of GIFTI"
       ON)
OPTION(AFNI_BUILD_CORELIBS_ONLY
       "Only build core libraries, no SUMA, plugins or programs"
       OFF)
#
# Install destinations
#
# Use different logical targets, but by default put all of them into
# on directory -- like the old makefiles were doing it
SET(AFNI_INSTALL_BIN_DIR "bin" CACHE PATH
    "Directory to install compiled programs into")
SET(AFNI_INSTALL_LIB_DIR "bin" CACHE PATH
    "Directory to install libraries into")
SET(AFNI_INSTALL_PLUGIN_DIR "bin" CACHE PATH
    "Directory to install plugins into")
SET(AFNI_INSTALL_SCRIPT_DIR "bin" CACHE PATH
    "Directory to install interpreted scripts into")
SET(AFNI_INSTALL_PICS_DIR "bin" CACHE PATH
    "Directory to install images into")
SET(AFNI_INSTALL_POEMS_DIR "bin" CACHE PATH
    "Directory to install poems into")

##
## Check for and configure for external dependencies
##

FIND_PACKAGE(X11 REQUIRED)
FIND_PACKAGE(Motif REQUIRED)
INCLUDE_DIRECTORIES(${MOTIF_INCLUDE_DIR})

#
# NIfTI
#
IF(AFNI_BUILD_LOCAL_NIFTICLIBS)
  INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/nifti/niftilib
                      ${CMAKE_SOURCE_DIR}/nifti/nifticdf
                      ${CMAKE_SOURCE_DIR}/nifti/znzlib)
  MESSAGE(STATUS "Using local NIfTI libs sources")
ELSE(AFNI_BUILD_LOCAL_NIFTICLIBS)
  FIND_PATH(NIFTI_INCLUDE_DIR nifti1.h
            PATHS /usr/include /usr/local/include
            PATH_SUFFIXES nifti niftilib
            DOC "NIfTI headers location")
  INCLUDE_DIRECTORIES(${NIFTI_INCLUDE_DIR})
ENDIF(AFNI_BUILD_LOCAL_NIFTICLIBS)

#
# F2C
#
# need to force linking against a static f2c when static libs are built
# to prevent unresolved symbols
IF(BUILD_SHARED_LIBS)
  SET(F2C_LIBRARY f2c)
ELSE(BUILD_SHARED_LIBS)
  SET(F2C_LIBRARY libf2c.a)
ENDIF(BUILD_SHARED_LIBS)

#
# Volpack
#
FIND_LIBRARY(VOLPACK_LIBRARY volpack)

#
# netcdf
#
FIND_LIBRARY(NETCDF_LIBRARY netcdf)

#
# 3dedge3
#
IF(AFNI_BUILD_LOCAL_3DEGDE3)
  SET(THREEDEDGE_LIBS 3DEdge)
  INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/3DEdge/src)
  MESSAGE(STATUS "Using local 3DEdge3 sources")
ELSE(AFNI_BUILD_LOCAL_3DEGDE3)
  SET(THREEDEDGE_LIBS )
ENDIF(AFNI_BUILD_LOCAL_3DEGDE3)

#
# OpenMP
#
FIND_PACKAGE(OpenMP)
IF(OPENMP_FOUND)
  MESSAGE(STATUS "Using OpenMP")
  SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS} -DUSE_OMP")
ENDIF(OPENMP_FOUND)

#
# Zlib
#
FIND_PACKAGE(ZLIB)
IF(ZLIB_FOUND)
  ADD_DEFINITIONS(-DHAVE_ZLIB)
  INCLUDE_DIRECTORIES(${ZLIB_INCLUDE_DIR})
ENDIF(ZLIB_FOUND)


##
## Platform checks and config
##
IF(UNIX)
  # 32bit or 64bit?
  IF(CMAKE_SIZEOF_VOID_P EQUAL 8)
    MESSAGE(STATUS "Building for a 64bit target system")
    ADD_DEFINITIONS(-DREAD_WRITE_64)
  ENDIF(CMAKE_SIZEOF_VOID_P EQUAL 8)

  IF(APPLE)
    # TODO determine best config
  ELSE(APPLE)
    ADD_DEFINITIONS(-DLINUX2 -DUSE_TRACING -DHAVE_XDBE)
  ENDIF(APPLE)
ELSE(UNIX)
  MESSAGE(FATAL_ERROR "Only UNIX-like platforms are supported.")
ENDIF(UNIX)

# TODO check if that is necessary at all
SET(FILE_PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ)

# core libs pretty much everythings has to be linked against
SET(AFNI_LIBS
   mri mrix coxplot
   ${VOLPACK_LIBRARY} ${NETCDF_LIBRARY} ${MOTIF_LIBRARIES} ${F2C_LIBRARY} m)
SET(WARP_LIBS afni_warp ${AFNI_LIBS})
SET(RICKR_LIBS rickr ${WARP_LIBS} ${AFNI_LIBS})


##
## Sub-projects
##
# always there
INCLUDE(CMakeLists_libs.txt)
ADD_SUBDIRECTORY(coxplot)

# GIFTI upon request
IF(AFNI_BUILD_LOCAL_GIFTI)
  # precharge GIFTI's installation target to sync it with other AFNI libs
  SET(GIFTI_INSTALL_BIN_DIR ${AFNI_INSTALL_BIN_DIR} CACHE PATH
     "Install GIFTI programs into this directory")
  SET(GIFTI_INSTALL_LIB_DIR ${AFNI_INSTALL_LIB_DIR} CACHE PATH
     "Install GIFTI library into this directory")
  MESSAGE(STATUS "Using local GIFTI sources")
  ADD_SUBDIRECTORY(gifti)

  # tell everybody that we have GIFTI
  ADD_DEFINITIONS(-DHAVE_GIFTI)
  INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/gifti)
  SET(AFNI_HAVE_GIFTI TRUE)
# or if there is a system lib
ELSE(AFNI_BUILD_LOCAL_GIFTI)
  FIND_LIBRARY(GIFTI_LIBRARY giftiio)
  IF(GIFTI_LIBRARY)
    MESSAGE(STATUS "Found libgiftiio, compiling with GIFTI support")
    SET(AFNI_HAVE_GIFTI TRUE)
  ELSE(GIFTI_LIBRARY)
    MESSAGE(STATUS "No libgiftiio found, compiling without GIFTI support")
    SET(AFNI_HAVE_GIFTI FALSE)
  ENDIF(GIFTI_LIBRARY)
ENDIF(AFNI_BUILD_LOCAL_GIFTI)

# everything that is not necessary for corelibs
IF(NOT AFNI_BUILD_CORELIBS_ONLY)
  ADD_SUBDIRECTORY(rickr)

  INCLUDE(CMakeLists_apps.txt)
  INCLUDE(CMakeLists_plugins.txt)

  ADD_SUBDIRECTORY(python_scripts)
  ADD_SUBDIRECTORY(R_scripts)
  ADD_SUBDIRECTORY(SUMA)

  ADD_SUBDIRECTORY(faces)
  ADD_SUBDIRECTORY(poems)
ENDIF(NOT AFNI_BUILD_CORELIBS_ONLY)
