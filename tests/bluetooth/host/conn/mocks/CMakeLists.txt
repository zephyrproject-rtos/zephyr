#
# CMakeLists.txt file for creating of mocks library.
#

add_library(mocks STATIC
	addr_internal.c
	att_internal.c
	bt_str.c
	buf_view.c
	hci_core.c
	id.c
	kernel.c
	l2cap_internal.c
	scan.c
	smp.c
	spinlock.c
	sys_clock.c
)

target_include_directories(mocks PUBLIC
	${ZEPHYR_BASE}/tests/bluetooth/host/conn/mocks
	${ZEPHYR_BASE}/subsys/bluetooth
)

target_compile_options(test_interface INTERFACE -include ztest.h)
target_link_libraries(mocks PRIVATE test_interface)
target_link_options(mocks PUBLIC
	"SHELL:-T ${ZEPHYR_BASE}/tests/bluetooth/host/conn/mocks/mock-sections.ld"
)

target_compile_options(mocks
	PRIVATE
	-DCONFIG_BT_ID_MAX=1
)
