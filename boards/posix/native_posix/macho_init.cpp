/*
 * Copyright (c) 2021, Facebook
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <algorithm>
#include <climits>
#include <iostream>
#include <list>
#include <vector>
#include <zephyr.h>
#include <device.h>
#include <soc.h>

using namespace std;

extern "C" {
int __bss_start;
int __bss_end;
const struct device __device_start[] = {};
const struct device __device_end[] = {};
int __init_APPLICATION_start;
int __init_POST_KERNEL_start;
int __init_PRE_KERNEL_1_start;
int __init_PRE_KERNEL_2_start;
int __init_end;
int __native_FIRST_SLEEP_tasks_start;
int __native_ON_EXIT_tasks_start;
int __native_PRE_BOOT_1_tasks_start;
int __native_PRE_BOOT_2_tasks_start;
int __native_PRE_BOOT_3_tasks_start;
int __native_tasks_end;
const struct _static_thread_data __static_thread_data_list_start[] __attribute__((used)) = {};
const struct _static_thread_data __static_thread_data_list_end[] __attribute__((used)) = {};
int _k_heap_list_start;
int _k_heap_list_end;
int _k_mem_slab_list_start;
int _k_mem_slab_list_end;

void __z_native_posix_init_add(const char *name, const void *fn, const struct device *dev, const char *levelstr, int prio);
void __z_native_posix_init_run(int level);
void __z_native_posix_task_add(const char *name, const void *fn, const char *levelstr, int prio);
void __z_native_posix_task_run(int level);
void __z_native_posix_static_thread_add(const char *name, size_t stack_size, void *entry, void *p1, void *p2, void *p3, int prio, int delay);
};

typedef int (*init_fun_t)(const struct device *);
struct init_fun_dev {
	//init_fun_dev() : init_fun_dev(nullptr, nullptr, nullptr, INT_MAX) {}
	init_fun_dev(const char *name, init_fun_t fun, const struct device *dev, int prio) : name(name), fun(fun), dev(dev), prio(prio) {}

	const char *name;
	init_fun_t fun;
	const struct device *dev;
	unsigned prio;
};

static vector<init_fun_dev> *PRE_KERNEL_1_init_objs;
static vector<init_fun_dev> *PRE_KERNEL_2_init_objs;
static vector<init_fun_dev> *POST_KERNEL_init_objs;
static vector<init_fun_dev> *APPLICATION_init_objs;

void __z_native_posix_init_add(const char *name, const void *fn, const struct device *dev, const char *levelstr, int prio)
{
	//cout << "INIT: name: " << name << " fn: " << fn << " device: " << dev << " level: " << levelstr << " prio: " << prio << endl;

	vector<init_fun_dev> *vifd = nullptr;
	string level(levelstr);

	if (PRE_KERNEL_1_init_objs == nullptr) {
		// oddly, these get clobbered before main unless they are dynamically allocated
		PRE_KERNEL_1_init_objs = new vector<init_fun_dev>();
		PRE_KERNEL_2_init_objs = new vector<init_fun_dev>();
		POST_KERNEL_init_objs = new vector<init_fun_dev>();
		APPLICATION_init_objs = new vector<init_fun_dev>();
		__ASSERT_NO_MSG(PRE_KERNEL_1_init_objs);
		__ASSERT_NO_MSG(PRE_KERNEL_2_init_objs);
		__ASSERT_NO_MSG(POST_KERNEL_init_objs);
		__ASSERT_NO_MSG(APPLICATION_init_objs);
	}

	if (level == "PRE_KERNEL_1") {
		vifd = PRE_KERNEL_1_init_objs;
	} else if (level == "PRE_KERNEL_2") {
		vifd = PRE_KERNEL_2_init_objs;
	} else if (level == "POST_KERNEL") {
		vifd = POST_KERNEL_init_objs;
	} else if (level == "APPLICATION") {
		vifd = APPLICATION_init_objs;
	} else {
		__ASSERT(false, "unsupported level");
	}

	vifd->push_back(init_fun_dev(name, (init_fun_t)fn, dev, prio));
}

void __z_native_posix_init_run(int level) {
	(void) level;

	vector<init_fun_dev> *vifd = nullptr;

	switch(level) {
	case _SYS_INIT_LEVEL_PRE_KERNEL_1:
		vifd = PRE_KERNEL_1_init_objs;
		break;
	case _SYS_INIT_LEVEL_PRE_KERNEL_2:
		vifd = PRE_KERNEL_2_init_objs;
		break;
	case _SYS_INIT_LEVEL_POST_KERNEL:
		vifd = POST_KERNEL_init_objs;
		break;
	case _SYS_INIT_LEVEL_APPLICATION:
		vifd = APPLICATION_init_objs;
		break;
	default:
		__ASSERT(false, "unsupported level");
	}

	if (vifd == nullptr || vifd->size() == 0) {
		//cout << "no init functions to execute at init level " << level << endl;
		return;
	}

	// sort initializers by increasing priority
	sort((*vifd).begin(), (*vifd).end(),
		[](const init_fun_dev & a, const init_fun_dev & b) -> bool {
			return b.prio > a.prio;
		});

	for(auto& i: *vifd) {
		//cout << "running " << i.name << "()" << endl;
		int rc = i.fun(i.dev);
		if (i.dev != NULL) {
			i.dev->state->init_res = rc;
			if (i.dev->state->init_res == 0) {
				i.dev->state->initialized = true;
			}
		}
	}
}

typedef void (*task_fun_t)(void);
struct task_fun {
	task_fun(const char *name, task_fun_t fun, int prio) : name(name), fun(fun), prio(prio) {}

	const char *name;
	task_fun_t fun;
	unsigned prio;
};

static vector<task_fun> *PRE_BOOT_1_tasks;
static vector<task_fun> *PRE_BOOT_2_tasks;
static vector<task_fun> *PRE_BOOT_3_tasks;
static vector<task_fun> *FIRST_SLEEP_tasks;
static vector<task_fun> *ON_EXIT_tasks;

void __z_native_posix_task_add(const char *name, const void *fn, const char *levelstr, int prio)
{
	//cout << "TASK: name: " << name << " fn: " << fn << " level: " << levelstr << " prio: " << prio << endl;
	string level(levelstr);
	vector<task_fun> *vtf = nullptr;

	if (PRE_BOOT_1_tasks == nullptr) {
		// oddly, these get clobbered before main unless they are dynamically allocated
		PRE_BOOT_1_tasks = new vector<task_fun>();
		PRE_BOOT_2_tasks = new vector<task_fun>();
		PRE_BOOT_3_tasks = new vector<task_fun>();
		FIRST_SLEEP_tasks = new vector<task_fun>();
		ON_EXIT_tasks = new vector<task_fun>();
		__ASSERT_NO_MSG(PRE_BOOT_1_tasks);
		__ASSERT_NO_MSG(PRE_BOOT_2_tasks);
		__ASSERT_NO_MSG(PRE_BOOT_3_tasks);
		__ASSERT_NO_MSG(FIRST_SLEEP_tasks);
		__ASSERT_NO_MSG(ON_EXIT_tasks);
	}

	if (level == "PRE_BOOT_1") {
		vtf = PRE_BOOT_1_tasks;
	} else if (level == "PRE_BOOT_2") {
		vtf = PRE_BOOT_2_tasks;
	} else if (level == "PRE_BOOT_3") {
		vtf = PRE_BOOT_3_tasks;
	} else if (level == "FIRST_SLEEP") {
		vtf = FIRST_SLEEP_tasks;
	} else if (level == "ON_EXIT") {
		vtf = ON_EXIT_tasks;
	} else {
		__ASSERT(false, "unsupported level");
	}

	vtf->push_back(task_fun(name, (task_fun_t)fn, prio));
}

void __z_native_posix_task_run(int level)
{
	vector<task_fun> *vtf = nullptr;

	switch(level) {
	case _NATIVE_PRE_BOOT_1_LEVEL:
		vtf = PRE_BOOT_1_tasks;
		break;
	case _NATIVE_PRE_BOOT_2_LEVEL:
		vtf = PRE_BOOT_2_tasks;
		break;
	case _NATIVE_PRE_BOOT_3_LEVEL:
		vtf = PRE_BOOT_3_tasks;
		break;
	case _NATIVE_FIRST_SLEEP_LEVEL:
		vtf = FIRST_SLEEP_tasks;
		break;
	case _NATIVE_ON_EXIT_LEVEL:
		vtf = ON_EXIT_tasks;
		break;
	default:
		__ASSERT(false, "unsupported level");
	}

	if (vtf == nullptr || vtf->size() == 0) {
		//cout << "no tasks to execute at native task level " << level << endl;
		return;
	}

	// sort initializers by increasing priority
	sort(vtf->begin(), vtf->end(),
		[](const task_fun & a, const task_fun & b) -> bool {
			return b.prio > a.prio;
		});

	for(auto& t: *vtf) {
		//cout << "running " << t.name << "()" << endl;
		t.fun();
	}
}

void __z_native_posix_static_thread_add(const char *name, size_t stack_size, void *entry, void *p1, void *p2, void *p3, int prio, int delay)
{
	//cout << "THREAD: " << name << " stack_size: " << stack_size << " entry: " << entry << " p1: " << p1 << " p2: " << p2 << " p3: " << p3 << " prio: " << prio << " delay: " << delay << endl;
}
