PROJECT(AFNI)

cmake_minimum_required(VERSION 2.6)
if(COMMAND cmake_policy)
  cmake_policy(SET CMP0002 NEW)
  cmake_policy(SET CMP0003 NEW)
endif(COMMAND cmake_policy)

##
## Define options to customize the build-process
##

OPTION(AFNI_BUILD_DEBUG "Build afni in debug mode" OFF)
OPTION(AFNI_BUILD_LOCAL_NIFTICLIBS
       "Build and use AFNI's local copy of the NIfTI libs"
       OFF)
OPTION(AFNI_BUILD_LOCAL_3DEGDE3
       "Build and use AFNI's local copy of lib3dedge3"
       OFF)
#
# Install destinations
#
# Use different logical targets, but by default put all of them into
# on directory -- like the old makefiles were doing it
OPTION(AFNI_INSTALL_BIN_DIR
       "Directory to install compiled programs into"
       ${CMAKE_INSTALL_PREFIX}/bin)
OPTION(AFNI_INSTALL_LIB_DIR
       "Directory to install libraries into"
       ${CMAKE_INSTALL_PREFIX}/bin)
OPTION(AFNI_INSTALL_PLUGIN_DIR
       "Directory to install plugins into"
       ${CMAKE_INSTALL_PREFIX}/bin)
OPTION(AFNI_INSTALL_SCRIPT_DIR
       "Directory to install interpreted scripts into"
       ${CMAKE_INSTALL_PREFIX}/bin)
OPTION(AFNI_INSTALL_PICS_DIR
       "Directory to install images into"
       ${CMAKE_INSTALL_PREFIX}/bin)
OPTION(AFNI_INSTALL_POEMS_DIR
       "Directory to install poems into"
       ${CMAKE_INSTALL_PREFIX}/bin)

##
## Check for and configure for external dependencies
##

FIND_PACKAGE(X11 REQUIRED)
FIND_PACKAGE(Motif REQUIRED)

#
# NIfTI
#
FIND_PATH(NIFTI_INCLUDES nifti1.h
          PATHS /usr/include /usr/local/include ${CMAKE_SOURCE_DIR}/nifti
          PATH_SUFFIXES nifti niftilib
          DOC "NIfTI headers location")
INCLUDE_DIRECTORIES(${NIFTI_INCLUDES})
IF(AFNI_BUILD_LOCAL_NIFTICLIBS)
  INCLUDE_DIRECTORIES(${NIFTI_INCLUDES}/../znzlib ${NIFTI_INCLUDES}/../nifticdf)
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
ELSE(AFNI_BUILD_LOCAL_3DEGDE3)
  SET(THREEDEDGE_LIBS )
ENDIF(AFNI_BUILD_LOCAL_3DEGDE3)

#
# OpenMP
#
FIND_PACKAGE(OpenMP)
IF(OPENMP_FOUND)
  SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS} -DUSE_OMP")
ENDIF(OPENMP_FOUND)


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

SET(AFNI_INCLUDES ${CMAKE_SOURCE_DIR} ${CMAKE_SOURCE_DIR}/rickr
 ${CMAKE_SOURCE_DIR}/niml ${CMAKE_SOURCE_DIR}/nifti/niftilib
 ${CMAKE_SOURCE_DIR}/nifti/nifticdf ${CMAKE_SOURCE_DIR}/nifti/znzlib
 ${CMAKE_SOURCE_DIR}/3DEdge/src)

SET(AFNI_LIBPATH /usr/lib)

SET(NETCDFLIBS netcdf)

SET(AFNI_LIBS mri mrix coxplot ${VOLPACK_LIBRARY} ${NETCDFLIBS} ${MOTIF_LIBRARIES} ${F2C_LIBRARY} m)

FIND_PACKAGE(ZLIB)
if (ZLIB_FOUND)
 LIST(APPEND CEXTRA -DHAVE_ZLIB)
 LIST(APPEND AFNI_LIBS ${ZLIB_LIBRARIES})
 FIND_PACKAGE(EXPAT)
 if (EXPAT_FOUND)
  LIST(APPEND CEXTRA -DHAVE_GIFTI)
  LIST(APPEND AFNI_LIBS ${EXPAT_LIBRARIES})
  LIST(APPEND AFNI_INCLUDES ${CMAKE_SOURCE_DIR}/gifti)
  SET(HAVE_GIFTI TRUE)
 endif (EXPAT_FOUND)
endif (ZLIB_FOUND)

SET(WARP_LIBS afni_warp ${AFNI_LIBS})

SET(RICKR_LIBS rickr ${WARP_LIBS} ${AFNI_LIBS})

# TODO I believe there are some missing
SET(EXTRAS myget Xphace rmz aiv ${EXPROGS})

# Programs using only AFNI_LIBS:
SET(PROGRAMS to3d from3d abut 3dclust nsize 3dinfo 3dproject 3dmerge count 
 sfim tfim ftosh 3dttest 3ddup imrotate imreg imstat 3dnvals 
 imand sqwave immask imdump imaver 3dhistog 
 p2t cdf 2swap 4swap mritopgm 3dANOVA 3dANOVA2 3dANOVA3 plugout_tta 
 waver 3dnewid 3dcalc ccalc imcalc 1dmatcalc 3drefit 3dbucket 
 AlphaSim 3dFWHM plugout_tt 3dnoise plugout_ijk 
 3dMannWhitney 3dWilcoxon 3dKruskalWallis 3dFriedman 
 3dmaskave 3dbuc2fim byteorder imstack 3dTcat 
 3drotate 3dvolreg 3dpc 3dfractionize 1dplot imupsam 
 3dIntracranial 24swap 3dTsmooth float_scan 1dtranspose 
 3dFourier 3dNotes 3dROIstats 1deval 3dTstat 3dmaskdump 
 3dTshift 3dDetrend 1dfft 1dcat 3drename 1dnorm afni_vcheck 3ddot 
 3dWavelets imcutup imcat 3dWinsor 3dZeropad 3dTagalign 
 3dMean 3dAttribute cat_matvec 3dOverlap 3dClipLevel 3dZregrid 
 3dEntropy ent16 3dRowFillin 1dgrayplot 3dToutcount 1dsum 
 3dExtrema strblast 3dZcutup 3dZcat 3dTqual 3dGetrow 
 3dTcorrelate 3dAnatNudge 3dcopy Vecwarp 3dMINCtoAFNI 3dCM fdrval
 3dAFNItoANALYZE siemens_vision ge_header mayo_analyze 3dAFNItoNIFTI 
 3dAutoTcorrelate 3dFDR rtfeedme 3dAutomask 3dAFNItoMINC 3dBrickStat 
 3dThreetoRGB Ifile 3dAutobox 3dLRflip 3dANALYZEtoAFNI 
 dicom_hdr 3dDespike dicom_to_raw rotcom 1ddot 1dsvd 
 3dAnhist 3dAFNIto3D 3dWarp nicat 
 fftest 3dDTeig 3dWarpDrive 
 plugout_drive 
 3dMedianFilter 3dAFNItoNIML 3dAFNItoRaw im2niml 
 DTIStudioFibertoSegments 3dLocalstat 
 3dTwotoComplex 3dInvFMRI 3dmatcalc 3dAcost 3dLocalBistat 3dFWHMx 
 3dBlurToFWHM 3dDFT 3dSynthesize 1dMarry 3dEmpty
 1dFlagMotion 3dTsort 1dTsort 3dTfitter 1dUpsample #1dREMLfit
 3dLocalSVD niml_feedme 3dErrtsCormat 3dUndump 3dREMLfit
 3dUpsample 3dTcorrMap 3dmatmult 3dmaskSVD 1dBandpass
 3dBlurInMask 3dRank
 3dDeconvolve 1dGentimes
 2dImReg 3dStatClust 3dDWItoDT 3dRegAna 3dTSgen 3dUniformize 3ddelay 3dfim+
 3dNLfim 3dNeocon 3dConvolve RSFgen
 ${EXTRAS})
SET(SCRIPTS @4Daverage @CommandGlobb @GetAfniOrient @DTI_studio_reposition
 @GetAfniPrefix @NoExt @UpdateAfni @RenamePanga @2dwarper
 @GetAfniView @SUMA_AlignToExperiment @SUMA_Make_Spec_FS @Purify_1D
 @SUMA_Make_Spec_SF @make_stim_file @Align_Centers suma_change_spec
 @CheckForAfniDset @clip_volume @AfniOrient2RAImap @parse_afni_name
 @auto_align @auto_tlrc @FromRAI @ToRAI @AfniOrientSign @VolCenter
 @Center_Distance @fix_FSsphere @parse_name @align_partial_oblique
 @np 3dMax @IsoMasks @SUMA_Make_Spec_Caret @FindAfniDsetPath @GetAfniID
 @float_fix @escape- 3dPAR2AFNI.pl @DriveAfni @DriveSuma @AddEdge
 @SurfSmooth.HEAT_07.examples @SUMA_FSvolToBRIK @isOblique afni_run_R
 @GetAfniDims @GetAfniRes @NoisySkullStrip @statauxcode @ScaleVolume
 @ShowDynamicRange AFNI_Batch_R @DO.examples demo.fixed.niml.do
 demo.mobile.niml.do @Spharm.examples @GetAfniBin @fast_roi
 @make_plug_diff @build_afni_Xlib @update.afni.binaries
 @ROI_Corr_Mat @FS_roi_label @demo_prompt @ScriptCheck)

SET(CMAKE_C_FLAGS_DEBUG "-g -Wall")
SET(CMAKE_LINK_FLAGS_DEBUG "-g")
SET(CMAKE_LINK_FLAGS_RELEASE "-s")

IF(CMAKE_BUILD_TYPE STREQUAL Debug)
 ADD_DEFINITIONS(${OPTIMIZEFLAGS})
 # Include files multiple times for backwards dependencies
 SET(AFNI_LIBS ${AFNI_LIBS} mri)
 SET(WARP_LIBS ${WARP_LIBS} ${AFNI_LIBS})
 SET(RICKR_LIBS ${RICKR_LIBS} ${AFNI_LIBS})
ELSE(CMAKE_BUILD_TYPE STREQUAL Debug)
 ADD_DEFINITIONS(${OPTIMIZEFLAGS})
ENDIF(CMAKE_BUILD_TYPE STREQUAL Debug)
ADD_DEFINITIONS(${WARNFLAGS} ${CEXTRA})

INCLUDE_DIRECTORIES(${MOTIF_INCLUDE_DIR} ${ZLIB_INCLUDE_DIR} ${AFNI_INCLUDES})
LINK_DIRECTORIES(${AFNI_LIBPATH})

# Sub-projects
ADD_SUBDIRECTORY(gifti)
ADD_SUBDIRECTORY(coxplot)
ADD_SUBDIRECTORY(rickr)
ADD_SUBDIRECTORY(faces)
ADD_SUBDIRECTORY(poems)
ADD_SUBDIRECTORY(python_scripts)
ADD_SUBDIRECTORY(R_scripts)
ADD_SUBDIRECTORY(SUMA)


##
## Targets
##
INCLUDE(CMakeLists_libmri.txt)
INCLUDE(CMakeLists_Plugins.txt)

FOREACH(program ${PROGRAMS})
 ADD_EXECUTABLE(${program} ${program}.c)
 TARGET_LINK_LIBRARIES(${program} ${AFNI_LIBS})
ENDFOREACH(program)

ADD_EXECUTABLE(1dSEM 1dSEM.c sqrmat.c)
TARGET_LINK_LIBRARIES(1dSEM ${AFNI_LIBS})

ADD_EXECUTABLE(3dAllineate 3dAllineate.c)
TARGET_LINK_LIBRARIES(3dAllineate ${AFNI_LIBS} ${WARP_LIBS} ${RICKR_LIBS})

ADD_EXECUTABLE(3dSpatNorm 3dSpatNorm.c)
TARGET_LINK_LIBRARIES(3dSpatNorm ${AFNI_LIBS} ${WARP_LIBS} ${RICKR_LIBS})

ADD_EXECUTABLE(whereami whereami.c)
TARGET_LINK_LIBRARIES(whereami ${AFNI_LIBS} ${WARP_LIBS} ${RICKR_LIBS})

ADD_EXECUTABLE(3dSegment 3dSegment.c)
TARGET_LINK_LIBRARIES(3dSegment ${AFNI_LIBS} ${WARP_LIBS} ${RICKR_LIBS})

ADD_EXECUTABLE(3dABoverlap 3dABoverlap.c)
TARGET_LINK_LIBRARIES(3dABoverlap ${AFNI_LIBS} ${WARP_LIBS} ${RICKR_LIBS})

ADD_EXECUTABLE(3danisosmooth 3danisosmooth.c DWIstructtensor.c)
TARGET_LINK_LIBRARIES(3danisosmooth ${AFNI_LIBS})

ADD_EXECUTABLE(3daxialize 3daxialize.c)
TARGET_LINK_LIBRARIES(3daxialize ${AFNI_LIBS} ${WARP_LIBS})

ADD_EXECUTABLE(3dfim 3dfim.c afni_pcor_update.c ts.c)
# This is for compiling to an object that was called afni_pcor_float.o
SET_SOURCE_FILES_PROPERTIES(afni_pcor_update.c PROPERTIES COMPILE_FLAGS "-DDTYPE=float")
TARGET_LINK_LIBRARIES(3dfim ${AFNI_LIBS})

ADD_EXECUTABLE(3dmaxima 3dmaxima.c maxima.c)
TARGET_LINK_LIBRARIES(3dmaxima ${AFNI_LIBS})

ADD_EXECUTABLE(3dretroicor 3dretroicor.c retroicor.c)
TARGET_LINK_LIBRARIES(3dretroicor ${AFNI_LIBS})

ADD_EXECUTABLE(FD2 FD2.c ts.c mcw.c overfim.c pcor.c csfft_AJ.c)
# This is for compiling pcor.c to an object that was called pcorsh.o
SET_SOURCE_FILES_PROPERTIES(pcor.c PROPERTIES COMPILE_FLAGS "-DREF_FLOAT_SINGLE -DVOX_SHORT")
TARGET_LINK_LIBRARIES(FD2 ${AFNI_LIBS})

ADD_EXECUTABLE(fim2 fim2.c ts.c pcor.c)
TARGET_LINK_LIBRARIES(fim2 ${AFNI_LIBS})

ADD_EXECUTABLE(adwarp adwarp.c)
TARGET_LINK_LIBRARIES(adwarp ${AFNI_LIBS} ${WARP_LIBS})

ADD_EXECUTABLE(3dDeconvolve_f 3dDeconvolve.c matrix_f.c)
# This is for compiling 3dDeconvolve.c and matrix_f.c to objects that were called 3dDeconvolve_f.o and matrix_f.o
SET_TARGET_PROPERTIES(3dDeconvolve_f PROPERTIES COMPILE_FLAGS "-DFLOATIZE")
TARGET_LINK_LIBRARIES(3dDeconvolve_f ${AFNI_LIBS})

ADD_EXECUTABLE(help_format help_format.c)

ADD_EXECUTABLE(afni_history afni_history.c 
 afni_history_bpittman.c afni_history_christip.c afni_history_dglen.c
 afni_history_gangc.c afni_history_rickr.c afni_history_rwcox.c
 afni_history_ziad.c)
TARGET_LINK_LIBRARIES(afni_history ${AFNI_LIBS})

#
# AFNI
#
SET(AFSRC afni.c afni_func.c afni_widg.c afni_fimmer.c 
 afni_pcor.c afni_pcor_update.c afni_transforms.c 
 pbar.c afni_graph.c afni_plugin.c afni_cluster.c
 parser.c parser_int.c afni_plugout.c afni_fimfunc.c 
 afni_setup.c afni_receive.c mcw_graf.c afni_splash.c 
 afni_pplug_env.c afni_pplug_2dfunc.c afni_friends.c 
 afni_ttren.c afni_pplug_1dfunc.c afni_driver.c 
 afni_niml.c afni_sumafunc.c 
 afni_version.c afni_lock.c afni_vol2surf.c
 afni_pplug_instacorr.c afni_pplug_instacalc.c)
# This is for compiling to an object that was called afni_pcor_float.o
SET_SOURCE_FILES_PROPERTIES(afni_pcor_update.c
                            PROPERTIES COMPILE_FLAGS "-DDTYPE=float")
ADD_EXECUTABLE(afni ${AFSRC})
TARGET_LINK_LIBRARIES(afni ${AFNI_LIBS} ${WARP_LIBS} ${RICKR_LIBS})


SET(ADDPROGRAMS
  afni 1dSEM 3dAllineate 3dSpatNorm whereami 3dSegment 3dABoverlap
  3danisosmooth 3daxialize 3dfim 3dmaxima 3dretroicor FD2 fim2 adwarp
  3dDeconvolve_f help_format afni_history)

# add local nifti apps if desired
IF(AFNI_BUILD_LOCAL_NIFTICLIBS)
  ADD_EXECUTABLE(nifti1_test nifti/utils/nifti1_test.c)
  TARGET_LINK_LIBRARIES(nifti1_test ${AFNI_LIBS})

  ADD_EXECUTABLE(nifti_tool nifti/utils/nifti_tool.c)
  TARGET_LINK_LIBRARIES(nifti_tool ${AFNI_LIBS})

  ADD_EXECUTABLE(nifti_stats nifti/utils/nifti_stats.c)
  TARGET_LINK_LIBRARIES(nifti_stats ${AFNI_LIBS})

  LIST(APPEND ADDPROGRAMS nifti1_test nifti_tool nifti_stats)
ENDIF(AFNI_BUILD_LOCAL_NIFTICLIBS)

# add local 3dedge3 if desired
IF(AFNI_BUILD_LOCAL_3DEGDE3)
  ADD_EXECUTABLE(3dedge3 3dedge3.c)
  TARGET_LINK_LIBRARIES(3dedge3 ${AFNI_LIBS} ${THREEDEDGE_LIBS})
  LIST(APPEND ADDPROGRAMS 3dedge3)
ENDIF(AFNI_BUILD_LOCAL_3DEGDE3)


##
## Install targets
##

INSTALL(TARGETS ${PROGRAMS} ${ADDPROGRAMS}
  RUNTIME DESTINATION ${AFNI_INSTALL_BIN_DIR} COMPONENT Runtime
  LIBRARY DESTINATION ${AFNI_INSTALL_LIB_DIR} COMPONENT Runtime
  ARCHIVE DESTINATION ${AFNI_INSTALL_LIB_DIR} COMPONENT Development)

INSTALL(PROGRAMS ${SCRIPTS}
  RUNTIME DESTINATION ${AFNI_INSTALL_SCRIPT_DIR} COMPONENT Runtime)
