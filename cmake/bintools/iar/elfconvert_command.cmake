# For iar rh850 the elfconvert command is made into a script.
# Reason for that is because not a single command covers all use cases,
# and it must therefore be possible to call individual commands, depending
# on the arguments used.
cmake_minimum_required(VERSION 3.20.0)

# Unknown support of stripping
# Unknown support of --srec-len in bintools
# Unknown support of gap-fill
# Unknown support of remove-section

# Handle Input and Output target types
if(DEFINED OUTTARGET)
  if(${OUTTARGET} STREQUAL "srec")
    set(obj_copy_target_output "--srec")
  elseif(${OUTTARGET} STREQUAL "ihex")
    set(obj_copy_target_output "--ihex")
  elseif(${OUTTARGET} STREQUAL "binary")
    set(obj_copy_target_output "--bin")
  endif()
endif()

if(DEFINED ONLY_SECTION AND "${OUTTARGET}" STREQUAL "binary")
  set(obj_copy_target_output "--bin")
  set(outfile_dir .dir)
  string(REGEX REPLACE "^[\.]" "" only_section_clean "${ONLY_SECTION}")
endif()

# Note: ielftool is a little special regarding bin output, as each section gets
#       its own file. This means that when only a specific section is required
#       then that section must be moved to correct location.
execute_process(
    COMMAND ${IELFTOOL} ${obj_copy_target_output}
    --verbose ${INFILE} ${OUTFILE}
)

if(DEFINED ONLY_SECTION AND "${OUTTARGET}" STREQUAL "binary")
  execute_process(
    COMMAND ${CMAKE_COMMAND} -E copy
      ${OUTFILE}${outfile_dir}/${only_section_clean} ${OUTFILE}
  )
endif()
