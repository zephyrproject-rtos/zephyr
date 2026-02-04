/**
 * @file
 * @brief MUX Devicetree macro public API header file.
 */

#ifndef ZEPHYR_INCLUDE_DEVICETREE_MUX_H_
#define ZEPHYR_INCLUDE_DEVICETREE_MUX_H_

#include <zephyr/devicetree.h>

#define DT_MUX_PROP_STRING(node_id) \
	COND_CODE_1(DT_NODE_HAS_PROP(node_id, mux_states), (mux_states), (mux_controls))


#define DT_MUX_CTLR_BY_IDX(node_id, idx) \
	DT_PHANDLE_BY_IDX(node_id, DT_MUX_PROP_STRING(node_id), idx)

#define DT_MUX_CTLR(node_id) \
	DT_MUX_CTLR_BY_IDX(node_id, 0)

#define DT_MUX_CTLR_BY_NAME(node_id, name)						\
	DT_MUX_CTLR_BY_IDX(node_id,							\
			   DT_PHA_ELEM_IDX_BY_NAME(node_id,				\
						   DT_MUX_PROP_STRING(node_id),		\
						   name))

#endif /* ZEPHYR_INCLUDE_DEVICETREE_MUX_H_ */
