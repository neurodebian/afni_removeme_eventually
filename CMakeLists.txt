PROJECT(AFNI)

# Define options to customize the build-process
OPTION(AFNI_BUILD_DEBUG "Build afni in debug mode" OFF)
OPTION(AFNI_BUILD_LOCAL_NIFTICLIBS
       "Build and use AFNI's local copy of the NIfTI libs"
       OFF)

IF(AFNI_BUILD_DEBUG)
 SET(CMAKE_BUILD_TYPE Debug)
ELSE(AFNI_BUILD_DEBUG)
 SET(CMAKE_BUILD_TYPE Release)
ENDIF(AFNI_BUILD_DEBUG)

cmake_minimum_required(VERSION 2.6)
if(COMMAND cmake_policy)
  cmake_policy(SET CMP0002 NEW)
  cmake_policy(SET CMP0003 NEW)
endif(COMMAND cmake_policy)

IF(CMAKE_BUILD_TYPE STREQUAL Debug)
 SET(BUILD_SHARED_LIBS OFF)
ELSE(CMAKE_BUILD_TYPE STREQUAL Debug)
 SET(BUILD_SHARED_LIBS ON)
ENDIF(CMAKE_BUILD_TYPE STREQUAL Debug)

SET(CMAKE_INSTALL_PREFIX /usr/local/)
EXECUTE_PROCESS(COMMAND ${CMAKE_UNAME} -m
 OUTPUT_VARIABLE UNAME
)
IF(UNAME MATCHES "x86_64")
 SET(INSTALLDIR distrib/afni64)
 SET(OPTIMIZEFLAGS -march=athlon64 -O2)
 SET(CEXTRA -DLINUX2 -DREAD_WRITE_64 -DUSE_TRACING -DHAVE_XDBE)
ELSE(UNAME MATCHES "x86_64")
 SET(INSTALLDIR distrib/afni32)
 SET(OPTIMIZEFLAGS -march=i686 -O2)
 SET(CEXTRA -DLINUX2 -DUSE_TRACING -DHAVE_XDBE)
ENDIF(UNAME MATCHES "x86_64")
SET(WARNFLAGS -Wcomment -Wformat)
SET(AFNI_INCLUDES ${CMAKE_SOURCE_DIR} ${CMAKE_SOURCE_DIR}/rickr
 ${CMAKE_SOURCE_DIR}/niml ${CMAKE_SOURCE_DIR}/nifti/niftilib
 ${CMAKE_SOURCE_DIR}/nifti/nifticdf ${CMAKE_SOURCE_DIR}/nifti/znzlib
 ${CMAKE_SOURCE_DIR}/3DEdge/src)
SET(AFNI_LIBPATH /usr/lib)
INCLUDE(FindMotif)
if (NOT MOTIF_FOUND)
 MESSAGE(FATAL_ERROR "Motif library was not found - cannot continue")
endif (NOT MOTIF_FOUND)
SET(NETCDFLIBS netcdf)
SET(AFNI_LIBS mri mrix coxplot volpack ${NETCDFLIBS} ${MOTIF_LIBRARIES} coxf2c mri m)
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
SET(THREEDEDGE_LIBS 3DEdge)

# Permissons to be set on installed files
SET(EXECUTABLE_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
SET(FILE_PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ)

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

INCLUDE(FindOpenMP)
IF(OPENMP_FOUND)
 SET(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} ${OpenMP_C_FLAGS} -DUSE_OMP")
ENDIF(OPENMP_FOUND)

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
INCLUDE(CMakeLists_libmri.txt)
ADD_SUBDIRECTORY(coxplot)
ADD_SUBDIRECTORY(rickr)
ADD_SUBDIRECTORY(faces)
ADD_SUBDIRECTORY(poems)
ADD_SUBDIRECTORY(python_scripts)
ADD_SUBDIRECTORY(R_scripts)
ADD_SUBDIRECTORY(SUMA)
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

ADD_EXECUTABLE(3dedge3 3dedge3.c)
TARGET_LINK_LIBRARIES(3dedge3 ${AFNI_LIBS} ${THREEDEDGE_LIBS})

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

IF(AFNI_BUILD_LOCAL_NIFTICLIBS)
  ADD_EXECUTABLE(nifti1_test nifti/utils/nifti1_test.c)
  TARGET_LINK_LIBRARIES(nifti1_test ${AFNI_LIBS})

  ADD_EXECUTABLE(nifti_tool nifti/utils/nifti_tool.c)
  TARGET_LINK_LIBRARIES(nifti_tool ${AFNI_LIBS})

  ADD_EXECUTABLE(nifti_stats nifti/utils/nifti_stats.c)
  TARGET_LINK_LIBRARIES(nifti_stats ${AFNI_LIBS})
ENDIF(AFNI_BUILD_LOCAL_NIFTICLIBS)

if (HAVE_GIFTI)
 ADD_EXECUTABLE(gifti_tool gifti/gifti_tool.c)
 TARGET_LINK_LIBRARIES(gifti_tool ${AFNI_LIBS})
 SET(GIFTI_PROGRAMS gifti_tool)
ENDIF (HAVE_GIFTI)

ADD_EXECUTABLE(help_format help_format.c)

ADD_EXECUTABLE(afni_history afni_history.c 
 afni_history_bpittman.c afni_history_christip.c afni_history_dglen.c
 afni_history_gangc.c afni_history_rickr.c afni_history_rwcox.c
 afni_history_ziad.c)
TARGET_LINK_LIBRARIES(afni_history ${AFNI_LIBS})

SET(AFSRC afni.c afni_func.c afni_widg.c afni_fimmer.c 
 afni_pcor.c afni_pcor_update.c afni_transforms.c 
 pbar.c afni_graph.c afni_plugin.c afni_cluster.c
 parser.c parser_int.c afni_plugout.c afni_fimfunc.c 
 afni_setup.c afni_receive.c mcw_graf.c afni_splash.c 
 afni_pplug_env.c afni_pplug_2dfunc.c afni_friends.c 
 afni_ttren.c afni_pplug_1dfunc.c afni_driver.c 
 afni_niml.c afni_sumafunc.c 
 afni_version.c afni_lock.c afni_vol2surf.c
 afni_pplug_instacorr.c)
ADD_EXECUTABLE(afni ${AFSRC})
# This is for compiling to an object that was called afni_pcor_float.o
SET_SOURCE_FILES_PROPERTIES(afni_pcor_update.c PROPERTIES COMPILE_FLAGS "-DDTYPE=float")
TARGET_LINK_LIBRARIES(afni ${AFNI_LIBS} ${WARP_LIBS} ${RICKR_LIBS})

SET(ADDPROGRAMS afni 
 1dSEM 3dAllineate 3dSpatNorm whereami 3dSegment 3dABoverlap 3danisosmooth 3daxialize 3dedge3
 3dfim 3dmaxima 3dretroicor FD2 fim2 adwarp 3dDeconvolve_f
 ${GIFTI_PROGRAMS}
 help_format
 afni_history)

IF(AFNI_BUILD_LOCAL_NIFTICLIBS)
  LIST(APPEND ADDPROGRAMS nifti1_test nifti_tool nifti_stats)
ENDIF(AFNI_BUILD_LOCAL_NIFTICLIBS)

INSTALL(
 TARGETS ${PROGRAMS} ${ADDPROGRAMS}
 DESTINATION ${INSTALLDIR}
 PERMISSIONS ${EXECUTABLE_PERMISSIONS}
)
INSTALL(
 PROGRAMS ${SCRIPTS}
 DESTINATION ${INSTALLDIR}
 PERMISSIONS ${EXECUTABLE_PERMISSIONS}
)
