
execute_process(
	COMMAND
	${PROJECT_SOURCE_DIR}/scripts/board_yaml2h.py
	-f ${BOARD_DIR}/${BOARD}.yaml
	-H ${PROJECT_BINARY_DIR}/include/generated/board_meta.h
	-c ${PROJECT_BINARY_DIR}/include/generated/board_meta.conf
	)

import_kconfig(${PROJECT_BINARY_DIR}/include/generated/board_meta.conf)
