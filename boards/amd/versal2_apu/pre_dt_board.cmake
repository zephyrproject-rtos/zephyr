
if(EXISTS "${BOARD_DIR}/${BOARD}.overlay")
	list(APPEND EXTRA_DTC_OVERLAY_FILE "${BOARD_DIR}/${BOARD}.overlay")
endif()
