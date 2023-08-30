/*
 * Copyright (c) 2023 Laczen
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef NVMP_DEFINE_H_
#define NVMP_DEFINE_H_

#define NVMP_INFO_DEFINE(inst, _type, _size, _ro, _store, _store_start, _read,	\
			 _write, _erase)					\
	const struct nvmp_info nvmp_info_##inst = {				\
		.type = _type,							\
		.size = _size,							\
		.read_only = _ro,						\
		.store = _store,						\
		.store_start = _store_start,					\
		.read = _read,							\
		.write = _write,						\
		.erase = _erase,						\
	}

#define NVMP_RO(inst)								\
	COND_CODE_1(DT_NODE_HAS_PROP(inst, read_only),				\
		    (DT_PROP(inst, read_only)), (false))

#define NVMP_PSIZE(inst) DT_REG_SIZE(inst)
#define NVMP_POFF(inst) DT_REG_ADDR(inst)
#define NVMP_PRO(inst) NVMP_RO(inst) || NVMP_RO(DT_GPARENT(inst))

#endif /* NVMP_DEFINE_H_ */
