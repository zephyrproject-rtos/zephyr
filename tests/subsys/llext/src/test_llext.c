/*
 * Copyright (c) 2023 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/fs/fs.h>
#if defined(CONFIG_FILE_SYSTEM_LITTLEFS)
#include <zephyr/fs/littlefs.h>
#endif
#include <zephyr/llext/elf.h>
#include <zephyr/llext/llext.h>
#include <zephyr/llext/symbol.h>
#include <zephyr/llext/buf_loader.h>
#include <zephyr/llext/fs_loader.h>
#include <zephyr/logging/log.h>
#include <zephyr/storage/flash_map.h>
#include <zephyr/sys/libc-hooks.h>
#include "syscalls_ext.h"
#include "threads_kernel_objects_ext.h"


LOG_MODULE_REGISTER(test_llext);


#ifdef CONFIG_LLEXT_STORAGE_WRITABLE
#define LLEXT_CONST
#else
#define LLEXT_CONST const
#endif

#ifdef CONFIG_LLEXT_EXPORT_BUILTINS_BY_SLID
#define LLEXT_FIND_BUILTIN_SYM(symbol_name) llext_find_sym(NULL, symbol_name ## _SLID)

#ifdef CONFIG_64BIT
#define printk_SLID ((const char *)0x87B3105268827052ull)
#define z_impl_ext_syscall_fail_SLID ((const char *)0xD58BC0E7C64CD965ull)
#else
#define printk_SLID ((const char *)0x87B31052ull)
#define z_impl_ext_syscall_fail_SLID ((const char *)0xD58BC0E7ull)
#endif
#else
#define LLEXT_FIND_BUILTIN_SYM(symbol_name) llext_find_sym(NULL, # symbol_name)
#endif

struct llext_test {
	const char *name;

	LLEXT_CONST uint8_t *buf;
	size_t buf_len;

	bool kernel_only;

	/*
	 * Optional callbacks
	 */

	/* Called in kernel context before each test starts */
	void (*test_setup)(struct llext *ext, struct k_thread *llext_thread);

	/* Called in kernel context after each test completes */
	void (*test_cleanup)(struct llext *ext);
};


K_THREAD_STACK_DEFINE(llext_stack, 1024);
struct k_thread llext_thread;


/* syscalls test */

int z_impl_ext_syscall_ok(int a)
{
	return a + 1;
}

#ifdef CONFIG_USERSPACE
static inline int z_vrfy_ext_syscall_ok(int a)
{
	return z_impl_ext_syscall_ok(a);
}
#include <zephyr/syscalls/ext_syscall_ok_mrsh.c>
#endif /* CONFIG_USERSPACE */


/* threads kernel objects test */

/* For these to be accessible from user space, they must be top-level globals
 * in the Zephyr image. Also, macros that add objects to special linker sections,
 * such as K_THREAD_STACK_DEFINE, do not work properly from extensions code.
 */
K_SEM_DEFINE(my_sem, 1, 1);
EXPORT_SYMBOL(my_sem);
struct k_thread my_thread;
EXPORT_SYMBOL(my_thread);
K_THREAD_STACK_DEFINE(my_thread_stack, MY_THREAD_STACK_SIZE);
EXPORT_SYMBOL(my_thread_stack);

#ifdef CONFIG_USERSPACE
/* Allow the test threads to access global objects.
 * Note: Permissions on objects used in the test by this thread are initialized
 * even in supervisor mode, so that user mode descendant threads can inherit
 * these permissions.
 */
static void threads_objects_test_setup(struct llext *, struct k_thread *llext_thread)
{
	k_object_access_grant(&my_sem, llext_thread);
	k_object_access_grant(&my_thread, llext_thread);
	k_object_access_grant(&my_thread_stack, llext_thread);
#if DT_HAS_CHOSEN(zephyr_console) && DT_NODE_HAS_STATUS_OKAY(DT_CHOSEN(zephyr_console))
	k_object_access_grant(DEVICE_DT_GET(DT_CHOSEN(zephyr_console)), llext_thread);
#endif
}
#else
/* No need to set up permissions for supervisor mode */
#define threads_objects_test_setup NULL
#endif /* CONFIG_USERSPACE */

void load_call_unload(const struct llext_test *test_case)
{
	struct llext_buf_loader buf_loader =
		LLEXT_BUF_LOADER(test_case->buf, test_case->buf_len);
	struct llext_loader *loader = &buf_loader.loader;
	struct llext_load_param ldr_parm = LLEXT_LOAD_PARAM_DEFAULT;
	struct llext *ext = NULL;

	int res = llext_load(loader, test_case->name, &ext, &ldr_parm);

	zassert_ok(res, "load should succeed");

	void (*test_entry_fn)() = llext_find_sym(&ext->exp_tab, "test_entry");

	zassert_not_null(test_entry_fn, "test_entry should be an exported symbol");

#ifdef CONFIG_USERSPACE
	/*
	 * Due to the number of MPU regions on some parts with MPU (USERSPACE)
	 * enabled we need to always call into the extension from a new dedicated
	 * thread to avoid running out of MPU regions on some parts.
	 *
	 * This is part dependent behavior and certainly on MMU capable parts
	 * this should not be needed! This test however is here to be generic
	 * across as many parts as possible.
	 */
	struct k_mem_domain domain;

	k_mem_domain_init(&domain, 0, NULL);

#ifdef Z_LIBC_PARTITION_EXISTS
	k_mem_domain_add_partition(&domain, &z_libc_partition);
#endif

	res = llext_add_domain(ext, &domain);
	if (res == -ENOSPC) {
		TC_PRINT("Too many memory partitions for this particular hardware\n");
		ztest_test_skip();
		return;
	}
	zassert_ok(res, "adding partitions to domain should succeed");

	/* Should be runnable from newly created thread */
	k_thread_create(&llext_thread, llext_stack,
			K_THREAD_STACK_SIZEOF(llext_stack),
			(k_thread_entry_t) &llext_bootstrap,
			ext, test_entry_fn, NULL,
			1, 0, K_FOREVER);

	k_mem_domain_add_thread(&domain, &llext_thread);

	if (test_case->test_setup) {
		test_case->test_setup(ext, &llext_thread);
	}

	k_thread_start(&llext_thread);
	k_thread_join(&llext_thread, K_FOREVER);

	if (test_case->test_cleanup) {
		test_case->test_cleanup(ext);
	}

	/* Some extensions may wish to be tried from the context
	 * of a userspace thread along with the usual supervisor context
	 * tried above.
	 */
	if (!test_case->kernel_only) {
		k_thread_create(&llext_thread, llext_stack,
				K_THREAD_STACK_SIZEOF(llext_stack),
				(k_thread_entry_t) &llext_bootstrap,
				ext, test_entry_fn, NULL,
				1, K_USER, K_FOREVER);

		k_mem_domain_add_thread(&domain, &llext_thread);

		if (test_case->test_setup) {
			test_case->test_setup(ext, &llext_thread);
		}

		k_thread_start(&llext_thread);
		k_thread_join(&llext_thread, K_FOREVER);

		if (test_case->test_cleanup) {
			test_case->test_cleanup(ext);
		}
	}

#else /* CONFIG_USERSPACE */
	/* No userspace support: run the test only in supervisor mode, without
	 * creating a new thread.
	 */
	if (test_case->test_setup) {
		test_case->test_setup(ext, NULL);
	}

#ifdef CONFIG_LLEXT_TYPE_ELF_SHAREDLIB
	/* The ELF specification forbids shared libraries from defining init
	 * entries, so calling llext_bootstrap here would be redundant. Use
	 * this opportunity to test llext_call_fn, even though llext_bootstrap
	 * would have behaved simlarly.
	 */
	zassert_ok(llext_call_fn(ext, "test_entry"),
		   "test_entry call should succeed");
#else /* !USERSPACE && !SHAREDLIB */
	llext_bootstrap(ext, test_entry_fn, NULL);
#endif

	if (test_case->test_cleanup) {
		test_case->test_cleanup(ext);
	}
#endif /* CONFIG_USERSPACE */

	llext_unload(&ext);
}

/*
 * Attempt to load, list, list symbols, call a fn, and unload each
 * extension in the test table. This excercises loading, calling into, and
 * unloading each extension which may itself excercise various APIs provided by
 * Zephyr.
 */
#define LLEXT_LOAD_UNLOAD(_name, extra_args...)			\
	ZTEST(llext, test_load_unload_##_name)			\
	{							\
		const struct llext_test test_case = {		\
			.name = STRINGIFY(_name),		\
			.buf = _name ## _ext,			\
			.buf_len = sizeof(_name ## _ext),	\
			extra_args                              \
		};						\
		load_call_unload(&test_case);			\
	}

/*
 * ELF file should be aligned to at least sizeof(elf_word) to avoid issues. A
 * larger value eases debugging, since it reduces the differences in addresses
 * between similar runs.
 */
#define ELF_ALIGN __aligned(4096)

static LLEXT_CONST uint8_t hello_world_ext[] ELF_ALIGN = {
	#include "hello_world.inc"
};
LLEXT_LOAD_UNLOAD(hello_world,
	.kernel_only = true
)

#ifndef CONFIG_LLEXT_TYPE_ELF_SHAREDLIB
static LLEXT_CONST uint8_t init_fini_ext[] ELF_ALIGN = {
	#include "init_fini.inc"
};

static void init_fini_test_cleanup(struct llext *ext)
{
	/* Make sure fini_fn() was called during teardown.
	 * (see init_fini_ext.c for more details).
	 */
	const int *number = llext_find_sym(&ext->exp_tab, "number");
	const int expected = (((((1 << 4) | 2) << 4) | 3) << 4) | 4; /* 0x1234 */

	zassert_not_null(number, "number should be an exported symbol");
	zassert_equal(*number, expected, "got 0x%x instead of 0x%x during cleanup",
		      *number, expected);
}

LLEXT_LOAD_UNLOAD(init_fini,
	.test_cleanup = init_fini_test_cleanup
)
#endif

static LLEXT_CONST uint8_t logging_ext[] ELF_ALIGN = {
	#include "logging.inc"
};
LLEXT_LOAD_UNLOAD(logging)

static LLEXT_CONST uint8_t relative_jump_ext[] ELF_ALIGN = {
	#include "relative_jump.inc"
};
LLEXT_LOAD_UNLOAD(relative_jump)

static LLEXT_CONST uint8_t object_ext[] ELF_ALIGN = {
	#include "object.inc"
};
LLEXT_LOAD_UNLOAD(object)

static LLEXT_CONST uint8_t syscalls_ext[] ELF_ALIGN = {
	#include "syscalls.inc"
};
LLEXT_LOAD_UNLOAD(syscalls)

static LLEXT_CONST uint8_t threads_kernel_objects_ext[] ELF_ALIGN = {
	#include "threads_kernel_objects.inc"
};
LLEXT_LOAD_UNLOAD(threads_kernel_objects,
	.test_setup = threads_objects_test_setup,
)

#ifndef CONFIG_LLEXT_TYPE_ELF_OBJECT
static LLEXT_CONST uint8_t multi_file_ext[] ELF_ALIGN = {
	#include "multi_file.inc"
};
LLEXT_LOAD_UNLOAD(multi_file)
#endif

#ifndef CONFIG_USERSPACE
static LLEXT_CONST uint8_t export_dependent_ext[] ELF_ALIGN = {
	#include "export_dependent.inc"
};

static LLEXT_CONST uint8_t export_dependency_ext[] ELF_ALIGN = {
	#include "export_dependency.inc"
};

ZTEST(llext, test_inter_ext)
{
	const void *dependency_buf = export_dependency_ext;
	const void *dependent_buf = export_dependent_ext;
	struct llext_buf_loader buf_loader_dependency =
		LLEXT_BUF_LOADER(dependency_buf, sizeof(hello_world_ext));
	struct llext_buf_loader buf_loader_dependent =
		LLEXT_BUF_LOADER(dependent_buf, sizeof(export_dependent_ext));
	struct llext_loader *loader_dependency = &buf_loader_dependency.loader;
	struct llext_loader *loader_dependent = &buf_loader_dependent.loader;
	const struct llext_load_param ldr_parm = LLEXT_LOAD_PARAM_DEFAULT;
	struct llext *ext_dependency = NULL, *ext_dependent = NULL;
	int ret = llext_load(loader_dependency, "inter_ext_dependency", &ext_dependency, &ldr_parm);

	zassert_ok(ret, "dependency load should succeed");

	ret = llext_load(loader_dependent, "export_dependent", &ext_dependent, &ldr_parm);

	zassert_ok(ret, "dependent load should succeed");

	int (*test_entry_fn)() = llext_find_sym(&ext_dependent->exp_tab, "test_entry");

	zassert_not_null(test_entry_fn, "test_entry should be an exported symbol");
	test_entry_fn();

	llext_unload(&ext_dependent);
	llext_unload(&ext_dependency);
}
#endif

#if defined(CONFIG_LLEXT_TYPE_ELF_RELOCATABLE) && defined(CONFIG_XTENSA)
static LLEXT_CONST uint8_t pre_located_ext[] ELF_ALIGN = {
	#include "pre_located.inc"
};

ZTEST(llext, test_pre_located)
{
	struct llext_buf_loader buf_loader =
		LLEXT_BUF_LOADER(pre_located_ext, sizeof(pre_located_ext));
	struct llext_loader *loader = &buf_loader.loader;
	struct llext_load_param ldr_parm = LLEXT_LOAD_PARAM_DEFAULT;
	struct llext *ext = NULL;
	const void *test_entry_fn;
	int res;

	/* load the extension trying to respect the addresses in the ELF */
	ldr_parm.pre_located = true;
	res = llext_load(loader, "pre_located", &ext, &ldr_parm);
	zassert_ok(res, "load should succeed");

	/* check the function address is the expected one */
	test_entry_fn = llext_find_sym(&ext->exp_tab, "test_entry");
	zassert_equal(test_entry_fn, (void *)0xbada110c, "test_entry should be at 0xbada110c");

	llext_unload(&ext);
}
#endif

#if defined(CONFIG_LLEXT_STORAGE_WRITABLE)
static LLEXT_CONST uint8_t find_section_ext[] ELF_ALIGN = {
	#include "find_section.inc"
};

ZTEST(llext, test_find_section)
{
	/* This test exploits the fact that in the STORAGE_WRITABLE cases, the
	 * symbol addresses calculated by llext will be directly inside the ELF
	 * file buffer, so the two methods can be easily compared.
	 */

	int res;
	ssize_t section_ofs;

	struct llext_buf_loader buf_loader =
		LLEXT_BUF_LOADER(find_section_ext, sizeof(find_section_ext));
	struct llext_loader *loader = &buf_loader.loader;
	struct llext_load_param ldr_parm = LLEXT_LOAD_PARAM_DEFAULT;
	struct llext *ext = NULL;
	elf_shdr_t shdr;

	res = llext_load(loader, "find_section", &ext, &ldr_parm);
	zassert_ok(res, "load should succeed");

	section_ofs = llext_find_section(loader, ".data");
	zassert_true(section_ofs > 0, "find_section returned %zd", section_ofs);

	res = llext_get_section_header(loader, ext, ".data", &shdr);
	zassert_ok(res, "get_section_header() should succeed");
	zassert_equal(shdr.sh_offset, section_ofs,
		     "different section offset %zd from get_section_header", shdr.sh_offset);

	uintptr_t symbol_ptr = (uintptr_t)llext_find_sym(&ext->exp_tab, "number");
	uintptr_t section_ptr = (uintptr_t)find_section_ext + section_ofs;

	/*
	 * FIXME on RISC-V, at least for GCC, the symbols aren't always at the beginning
	 * of the section when CONFIG_LLEXT_TYPE_ELF_OBJECT is used, breaking this assertion.
	 * Currently, CONFIG_LLEXT_TYPE_ELF_OBJECT is not supported on RISC-V.
	 */

	zassert_equal(symbol_ptr, section_ptr,
		      "symbol at %p != .data section at %p (%zd bytes in the ELF)",
		      symbol_ptr, section_ptr, section_ofs);

	llext_unload(&ext);
}

static LLEXT_CONST uint8_t test_detached_ext[] ELF_ALIGN = {
	#include "detached_fn.inc"
};

static struct llext_loader *detached_loader;
static struct llext *detached_llext;
static elf_shdr_t detached_shdr;

static bool test_section_detached(const elf_shdr_t *shdr)
{
	if (!detached_shdr.sh_addr) {
		int res = llext_get_section_header(detached_loader, detached_llext,
						   ".detach", &detached_shdr);

		zassert_ok(res, "get_section_header should succeed");
	}

	return shdr->sh_name == detached_shdr.sh_name;
}

ZTEST(llext, test_detached)
{
	struct llext_buf_loader buf_loader =
		LLEXT_BUF_LOADER(test_detached_ext, sizeof(test_detached_ext));
	struct llext_load_param ldr_parm = LLEXT_LOAD_PARAM_DEFAULT;
	int res;

	ldr_parm.section_detached = test_section_detached;
	detached_loader = &buf_loader.loader;

	res = llext_load(detached_loader, "test_detached", &detached_llext, &ldr_parm);
	zassert_ok(res, "load should succeed");

	/*
	 * Verify that the detached section is outside of the .text region.
	 * This only works with the shared ELF type, because with other types
	 * section addresses aren't "real," e.g. they can be 0.
	 */
	elf_shdr_t *text_region = detached_loader->sects + LLEXT_MEM_TEXT;

	zassert_true(text_region->sh_offset >= detached_shdr.sh_offset + detached_shdr.sh_size ||
		     detached_shdr.sh_offset >= text_region->sh_offset + text_region->sh_size);

	void (*test_entry_fn)() = llext_find_sym(&detached_llext->exp_tab, "test_entry");

	zassert_not_null(test_entry_fn, "test_entry should be an exported symbol");
	test_entry_fn();

	test_entry_fn = llext_find_sym(&detached_llext->exp_tab, "detached_entry");

	zassert_not_null(test_entry_fn, "detached_entry should be an exported symbol");
	test_entry_fn();

	llext_unload(&detached_llext);
}
#endif

#if defined(CONFIG_FILE_SYSTEM)
#define LLEXT_FILE "hello_world.llext"

FS_LITTLEFS_DECLARE_DEFAULT_CONFIG(storage);
static struct fs_mount_t mp = {
	.type = FS_LITTLEFS,
	.fs_data = &storage,
	.storage_dev = (void *)FIXED_PARTITION_ID(storage_partition),
	.mnt_point = "/lfs",
};

ZTEST(llext, test_fs_loader)
{
	int res;
	char path[UINT8_MAX];
	struct fs_file_t fd;

	/* File system should be mounted before the testcase. If not mount it now. */
	if (!(mp.flags & FS_MOUNT_FLAG_AUTOMOUNT)) {
		zassert_ok(fs_mount(&mp), "Filesystem should be mounted");
	}

	snprintf(path, sizeof(path), "%s/%s", mp.mnt_point, LLEXT_FILE);
	fs_file_t_init(&fd);

	zassert_ok(fs_open(&fd, path, FS_O_CREATE | FS_O_TRUNC | FS_O_WRITE),
		   "Failed opening file");

	zassert_equal(fs_write(&fd, hello_world_ext, ARRAY_SIZE(hello_world_ext)),
		      ARRAY_SIZE(hello_world_ext),
		      "Full content of the buffer holding ext should be written");

	zassert_ok(fs_close(&fd), "Failed closing file");

	struct llext_fs_loader fs_loader = LLEXT_FS_LOADER(path);
	struct llext_loader *loader = &fs_loader.loader;
	struct llext_load_param ldr_parm = LLEXT_LOAD_PARAM_DEFAULT;
	struct llext *ext = NULL;

	res = llext_load(loader, "hello_world", &ext, &ldr_parm);
	zassert_ok(res, "load should succeed");

	void (*test_entry_fn)() = llext_find_sym(&ext->exp_tab, "test_entry");

	zassert_not_null(test_entry_fn, "test_entry should be an exported symbol");

	llext_unload(&ext);
	fs_unmount(&mp);
}
#endif

/*
 * Ensure that EXPORT_SYMBOL does indeed provide a symbol and a valid address
 * to it.
 */
ZTEST(llext, test_printk_exported)
{
	const void * const printk_fn = LLEXT_FIND_BUILTIN_SYM(printk);

	zassert_equal(printk_fn, printk, "printk should be an exported symbol");
}

/*
 * The syscalls test above verifies that custom syscalls defined by extensions
 * are properly exported. Since `ext_syscalls.h` declares ext_syscall_fail, we
 * know it is picked up by the syscall build machinery, but the implementation
 * for it is missing. Make sure the exported symbol for it is NULL.
 */
ZTEST(llext, test_ext_syscall_fail)
{
	const void * const esf_fn = LLEXT_FIND_BUILTIN_SYM(z_impl_ext_syscall_fail);

	zassert_is_null(esf_fn, "est_fn should be NULL");
}

ZTEST_SUITE(llext, NULL, NULL, NULL, NULL, NULL);
