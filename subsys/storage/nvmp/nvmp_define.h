/*
 * Copyright (c) 2024 Laczen
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef NVMP_DEFINE_H_
#define NVMP_DEFINE_H_

#define NVMP_INFO_DEFINE(inst, _store, _size, _block_size, _write_block_size,   \
			 _open, _read, _write, _erase, _clear, _close)          \
	const struct nvmp_info nvmp_info_##inst = {                             \
		.store = _store,                                                \
		.size = _size,                                                  \
		.block_size = _block_size,                                      \
		.write_block_size = _write_block_size,                          \
		.open = _open,                                                  \
		.read = _read,                                                  \
		.write = _write,                                                \
		.erase = _erase,                                                \
		.clear = _clear,                                                \
		.close = _close,                                                \
	}

#define NVMP_RO(inst)                                                           \
	COND_CODE_1(DT_NODE_HAS_PROP(inst, read_only),                          \
		    (DT_PROP(inst, read_only)), (false))

#define NVMP_SIZE(inst) DT_REG_SIZE(inst)
#define NVMP_OFF(inst)  DT_REG_ADDR(inst)

#define NVMP_PRO(inst) NVMP_RO(inst) || NVMP_RO(DT_GPARENT(inst))

#endif /* NVMP_DEFINE_H_ */
