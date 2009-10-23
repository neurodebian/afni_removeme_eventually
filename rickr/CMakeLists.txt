PROJECT(rickr)

SET(SOURCES r_idisp.c r_misc.c r_new_resam_dset.c)

SET(PROGRAMS 3dresample file_tool Imon Dimon serial_helper)

ADD_LIBRARY(rickr ${SOURCES})
TARGET_LINK_LIBRARIES(rickr mri afni_warp)

ADD_EXECUTABLE(3dresample 3dresample.c)
TARGET_LINK_LIBRARIES(3dresample ${AFNI_LIBS} ${WARP_LIBS} ${RICKR_LIBS})

ADD_EXECUTABLE(file_tool file_tool.c fields.c)
TARGET_LINK_LIBRARIES(file_tool ${AFNI_LIBS}) # Just for ge4_header

SET(IMON_OBS l_mcw_glob.c realtime.c)
ADD_EXECUTABLE(Imon Imon.c ${IMON_OBS})
TARGET_LINK_LIBRARIES(Imon ${AFNI_LIBS}) # For thd_iochan

ADD_EXECUTABLE(Dimon Dimon.c dimon_afni.c ${CMAKE_SOURCE_DIR}/mri_dicom_hdr.c ${IMON_OBS})
SET_SOURCE_FILES_PROPERTIES(${CMAKE_SOURCE_DIR}/mri_dicom_hdr.c PROPERTIES COMPILE_FLAGS "-DDONT_USE_MCW_MALLOC -DFOR_DIMON")
TARGET_LINK_LIBRARIES(Dimon ${AFNI_LIBS}) # For thd_iochan

ADD_EXECUTABLE(serial_helper serial_helper.c)

INSTALL(
 TARGETS rickr
 DESTINATION ${INSTALLDIR}
 PERMISSIONS ${FILE_PERMISSIONS}
)
INSTALL(
 TARGETS ${PROGRAMS}
 DESTINATION ${INSTALLDIR}
 PERMISSIONS ${EXECUTABLE_PERMISSIONS}
)
