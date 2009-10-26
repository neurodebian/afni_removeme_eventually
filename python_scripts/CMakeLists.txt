PROJECT(PYTHON_SCRIPTS)

# script being programs on their own
SET(SCRIPTS
  1d_tool.py afni_proc.py align_epi_anat.py @DoPerRoi.py eg_main_chrono.py
  gen_epi_review.py gui_xmat.py lpc_align.py make_random_timing.py
  make_stim_times.py neuro_deconvolve.py python_module_test.py
  realtime_receiver.py timing_tool.py ui_xmat.py xmat_tool.py)

# everything else that is not meant to be a standalone script
SET(MODULES
  afni_base.py afni_util.py afni_xmat.py ask_me.py db_mod.py lib_afni1D.py
  lib_matplot.py lib_realtime.py lib_RR_plot.py lib_timing.py lib_wx.py
  module_test_lib.py option_list.py ui_xmat.py)

INSTALL(FILES ${MODULES}
  DESTINATION ${AFNI_INSTALL_SCRIPT_DIR} COMPONENT Runtime
  PERMISSIONS ${FILE_PERMISSIONS})
INSTALL(PROGRAMS ${SCRIPTS}
  DESTINATION ${AFNI_INSTALL_SCRIPT_DIR} COMPONENT Runtime)
