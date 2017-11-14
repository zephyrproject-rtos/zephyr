



execute_process(
	COMMAND
	${PROJECT_SOURCE_DIR}/scripts/board_yaml2h.py
	-f ${BOARD_DIR}/${BOARD}.yaml
	-o ${PROJECT_BINARY_DIR}/include/generated/board_meta.h
	)
