/*
 * Copyright (c) 2015 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * @file
 * @brief Stub for C++ virtual tables
 */

/**
 *
 * @brief basic virtual tables required for classes to build
 *
 */
#if !defined(CONFIG_LIB_CPLUSPLUS)

namespace __cxxabiv1 {
	class __class_type_info {
		virtual void dummy();
	};
	class __si_class_type_info {
		virtual void dummy();
	};
	void __class_type_info::dummy() { }  // causes the vtable to get created here
	void __si_class_type_info::dummy() { }  // causes the vtable to get created here
};
#endif // !defined(CONFIG_LIB_CPLUSPLUS)
