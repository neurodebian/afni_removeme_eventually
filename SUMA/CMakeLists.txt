PROJECT(SUMA)

##
## Define options to customize the build-process
##

OPTION(AFNI_BUILD_LOCAL_GLW
       "Build and use AFNI's local copy of libGLw"
       OFF)

##
## Check for and configure for external dependencies
##

FIND_PACKAGE(OpenGL REQUIRED)
FIND_PACKAGE(GLUT REQUIRED)
INCLUDE_DIRECTORIES(${OPENGL_INCLUDE_DIR} ${GLUT_INCLUDE_DIR})

# gts (and glib headers) are only needed for SurfMesh
INCLUDE(FindPkgConfig)
pkg_check_modules(GTS gts)
IF(GTS_FOUND)
  # Macro to turn a list into a string (why doesn't CMake have this
  # built-in?)
  MACRO (LIST_TO_STRING _string _list)
  SET (${_string})
    FOREACH (_item ${_list})
      SET (${_string} "${${_string}} ${_item}")
    ENDFOREACH (_item)
  ENDMACRO (LIST_TO_STRING)

  LIST_TO_STRING(XGTS_CFLAGS "${GTS_CFLAGS}")
  MESSAGE(STATUS "GTS library found - compiling SurfMesh")
ELSE(GTS_FOUND)
  MESSAGE(STATUS "GTS library not found - compiling without SurfMesh")
ENDIF(GTS_FOUND)

#
# libSUMA
#
SET(LIBSUMA_SRCS
  SUMA_trackball.c SUMA_SVmanip.c SUMA_input.c SUMA_MiscFunc.c
  SUMA_IV_XYZextract.c SUMA_IV_FaceSetsextract.c SUMA_SurfNorm.c
  SUMA_DOmanip.c SUMA_StripPath.c SUMA_Load_Surface_Object.c SUMA_CreateDO.c
  SUMA_help.c SUMA_display.c SUMA_ParseCommands.c SUMA_Engine.c
  SUMA_Surface_IO.c SUMA_VolData.c SUMA_niml.c SUMA_Color.c SUMA_GeomComp.c
  SUMA_SphericalMapping.c SUMA_DataSets.c SUMA_Homer.c SUMA_xColBar.c
  SUMA_SurfClust.c SUMA_IsoSurface.c SUMA_ConvexHull.c SUMA_BrainWrap.c
  SUMA_global.c SUMA_plot.c SUMA_volume_render.c SUMA_SurfaceToSurface.c
  SUMA_LocalStat.c SUMA_spharm.c SUMA_dot.c PLY/plyfile.c
  MarchingCubes/MarchingCubes.c)

# add libGLw symbols if requested
IF(AFNI_BUILD_LOCAL_GLW)
  LIST(APPEND LIBSUMA_SRCS GLw_local/GLwDrawA.c)
  INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/GLw_local)
ELSE(AFNI_BUILD_LOCAL_GLW)
  FIND_LIBRARY(GLW_LIBRARY GLw)
ENDIF(AFNI_BUILD_LOCAL_GLW)

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/PLY)

# TODO determine what this is for
ADD_DEFINITIONS(-DSUMA_COMPILED -DnoGLwidget)

SET(SUMA_LIBS
  ${AFNI_LIBS}
  ${THREEDEDGE_LIBS}
  ${RICKR_LIBS}
  ${WARP_LIBS}
  SUMA
  ${OPENGL_LIBRARIES}
  ${GLUT_LIBRARIES})

ADD_LIBRARY(SUMA ${LIBSUMA_SRCS})
TARGET_LINK_LIBRARIES(SUMA ${OPENGL_LIBRARIES} ${GLUT_LIBRARIES}
   ${MOTIF_LIBRARIES} mrix mri rickr)
# need to link against GLw if not included in the library
IF(NOT AFNI_BUILD_LOCAL_GLW)
  TARGET_LINK_LIBRARIES(SUMA ${GLW_LIBRARY})
ENDIF(NOT AFNI_BUILD_LOCAL_GLW)

#
# Programs
#
SET(PROGRAMS
  suma prompt_user MakeColorMap ScaleToMap
  inflate_compare SurfaceMetrics ConvertSurface
  ConvertDset ROI2dataset SurfSmooth SurfPatch SurfQual SurfClust IsoSurface
  ConvexHull 3dSkullStrip 3dCRUISEtoAFNI 3dBRAIN_VOYAGERtoAFNI 3dVol2Surf
  SurfMeasures FSread_annot SampBias volume_render 3dSurfMask SurfToSurf
  ROIgrow AnalyzeTrace DriveSuma SurfDist SpharmReco SpharmDeco SurfDsetInfo
  SurfLocalStat SurfFWHM NikoMap SurfInfo 3dSurf2Vol)

# automatically add targets for all programs following to frequently used
# naming scheme
FOREACH(program ${PROGRAMS})
  ADD_EXECUTABLE(${program} SUMA_${program}.c)
ENDFOREACH(program)

# SUMA prefixed programs
FOREACH(program SUMA_glxdino SUMA_paperplane SUMA_pixmap2eps
                SUMA_Load_Surface_Object SUMA_inflate_compare SUMA_Homer)
  ADD_EXECUTABLE(${program} ${program}.c)
  LIST(APPEND PROGRAMS ${program})
ENDFOREACH(program)

#
# Additonal targets
#
ADD_EXECUTABLE(SumaProgramTemplate SUMA_ProgramTemplate.c)
LIST(APPEND PROGRAMS SumaProgramTemplate)

ADD_EXECUTABLE(SUMA_Read_SpecFile SUMA_Load_Surface_Object.c)
LIST(APPEND PROGRAMS SUMA_Read_SpecFile)

ADD_EXECUTABLE(CompareSurfaces SUMA_compare_surfaces.c)
SET_TARGET_PROPERTIES(CompareSurfaces PROPERTIES COMPILE_FLAGS -DSUMA_compare_surfaces_STAND_ALONE)
LIST(APPEND PROGRAMS CompareSurfaces)


# reusing SUMA_surface_IO.c
ADD_EXECUTABLE(SUMA_FreeSurfer SUMA_Surface_IO.c)
LIST(APPEND PROGRAMS SUMA_FreeSurfer)

ADD_EXECUTABLE(SUMA_SureFit SUMA_Surface_IO.c)
LIST(APPEND PROGRAMS SUMA_SureFit)

ADD_EXECUTABLE(SUMA_Ply_Read SUMA_Surface_IO.c)
LIST(APPEND PROGRAMS SUMA_Ply_Read)


# reusing SUMA_SphericalMapping.c
ADD_EXECUTABLE(CreateIcosahedron SUMA_SphericalMapping.c)
LIST(APPEND PROGRAMS CreateIcosahedron)

ADD_EXECUTABLE(SUMA_Map_SurfacetoSurface SUMA_SphericalMapping.c)
TARGET_LINK_LIBRARIES(SUMA_Map_SurfacetoSurface
  ${AFNI_LIBS} ${RICKR_LIBS} ${WARP_LIBS} ${THREEDEDGE_LIBS})
LIST(APPEND PROGRAMS SUMA_Map_SurfacetoSurface)

ADD_EXECUTABLE(MapIcosahedron SUMA_SphericalMapping.c)
LIST(APPEND PROGRAMS MapIcosahedron)


# reusing SUMA_Load_Surface_Object.c
ADD_EXECUTABLE(inspec SUMA_Load_Surface_Object.c)
LIST(APPEND PROGRAMS inspec)

ADD_EXECUTABLE(quickspec SUMA_Load_Surface_Object.c)
LIST(APPEND PROGRAMS quickspec)


# error: too few arguments to function 'SUMA_disp_vecdmat'
#ADD_EXECUTABLE(SUMA_SurfNorm SUMA_SurfNorm.c)
#SET_TARGET_PROPERTIES(SUMA_SurfNorm PROPERTIES COMPILE_FLAGS -DSUMA_SurfNorm_STAND_ALONE)
#LIST(APPEND PROGRAMS SUMA_SurfNorm)

ADD_EXECUTABLE(SUMA_ParseName SUMA_StripPath.c)
LIST(APPEND PROGRAMS SUMA_ParseName)

ADD_EXECUTABLE(ShowCmap SUMA_xColBar.c)
SET_TARGET_PROPERTIES(ShowCmap PROPERTIES COMPILE_FLAGS -DSUMA_SHOW_CMAP_STAND_ALONE)
LIST(APPEND PROGRAMS ShowCmap)

# Source files missing
#ADD_EXECUTABLE(resolvitivity SUMA_resolvitivity.c)
#LIST(APPEND PROGRAMS resolvitivity)

#ADD_EXECUTABLE(3dfilter SUMA_3dfilter.c)
#LIST(APPEND PROGRAMS 3dfilter)

ADD_EXECUTABLE(Surf2VolCoord SUMA_Surf2VolCoord_demo.c)
LIST(APPEND PROGRAMS Surf2VolCoord)

#error: too few arguments to function 'Debug_Move'
#ADD_EXECUTABLE(toy_circle SUMA_toy_circle.c)
#LIST(APPEND PROGRAMS toy_circle)

ADD_EXECUTABLE(path_optimize SUMA_path_optimize.c SUMA_SurfWarp.c)
TARGET_LINK_LIBRARIES(path_optimize ${SUMA_LIBS})
LIST(APPEND PROGRAMS path_optimize)

# Source files missing
#ADD_EXECUTABLE(spharm_test SUMA_spharm.c)
#LIST(APPEND PROGRAMS spharm_test)

IF(GTS_FOUND)
  ADD_EXECUTABLE(SurfMesh SUMA_coarsen.c SUMA_gts.c SUMA_gts_insert.c)
  SET_TARGET_PROPERTIES(SurfMesh PROPERTIES COMPILE_FLAGS ${XGTS_CFLAGS})
  TARGET_LINK_LIBRARIES(SurfMesh ${GTS_LIBRARIES})
  LIST(APPEND PROGRAMS SurfMesh)
ENDIF(GTS_FOUND)

# all programs link against SUMA_LIBS
FOREACH(program ${PROGRAMS})
  TARGET_LINK_LIBRARIES(${program} ${SUMA_LIBS})
ENDFOREACH(program)

#
# Customize some of the targets generated above
#
FOREACH(program
  MakeColorMap ScaleToMap SurfaceMetrics ConvertSurface
  ROI2dataset SurfClust IsoSurface SurfQual ConvexHull FSread_annot SampBias
  CreateIcosahedron MapIcosahedron inspec quickspec)
  SET_TARGET_PROPERTIES(${program} PROPERTIES
                        COMPILE_FLAGS -DSUMA_${program}_STAND_ALONE)
ENDFOREACH(program)

# pretty much the same again but for programms with 'SUMA_' prefix
FOREACH(program
  SUMA_Read_SpecFile SUMA_FreeSurfer SUMA_SureFit SUMA_Ply_Read
  SUMA_Map_SurfacetoSurface SUMA_Load_Surface_Object
  SUMA_ParseName SUMA_Homer SUMA_inflate_compare )
  SET_TARGET_PROPERTIES(${program} PROPERTIES
                        COMPILE_FLAGS -D${program}_STAND_ALONE)
ENDFOREACH(program)

# more customization not following any standard naming scheme
SET_TARGET_PROPERTIES(SurfPatch PROPERTIES COMPILE_FLAGS -DSUMA_getPatch_STANDALONE)
SET_TARGET_PROPERTIES(3dSkullStrip PROPERTIES COMPILE_FLAGS -DSUMA_BrainWrap_STANDALONE)
SET_TARGET_PROPERTIES(volume_render PROPERTIES COMPILE_FLAGS -DDO_VOLUME_MAIN)


#
# Install targets
#
INSTALL(TARGETS SUMA ${PROGRAMS}
  RUNTIME DESTINATION ${AFNI_INSTALL_BIN_DIR} COMPONENT Runtime
  LIBRARY DESTINATION ${AFNI_INSTALL_LIB_DIR} COMPONENT Runtime
  ARCHIVE DESTINATION ${AFNI_INSTALL_LIB_DIR} COMPONENT Development)
