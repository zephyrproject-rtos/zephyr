/**
 * @file
 * @brief MUX Devicetree macro public API header file.
 */

#ifndef ZEPHYR_INCLUDE_DEVICETREE_MUX_H_
#define ZEPHYR_INCLUDE_DEVICETREE_MUX_H_

#include <zephyr/devicetree.h>

/** @cond INTERNAL_HIDDEN */
#define ZPRIV_MUX_PROP_STRING(node_id) \
	COND_CODE_1(DT_NODE_HAS_PROP(node_id, mux_states), (mux_states), (mux_controls))
/** @endcond */

#define DT_MUX_CTLR_BY_IDX(node_id, idx) \
	DT_PHANDLE_BY_IDX(node_id, ZPRIV_MUX_PROP_STRING(node_id), idx)

#endif /* ZEPHYR_INCLUDE_DEVICETREE_MUX_H_ */
