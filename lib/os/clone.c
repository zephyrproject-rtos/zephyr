/*
 * Copyright (c) 2026, Friedt Professional Engineering Services, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * The clone dispatch: one creation primitive with a thread tier and process
 * tiers. The thread tier lives here so it exists
 * without CONFIG_SYS_PROCESS; the process tiers are provided by process.c.
 */

#include <errno.h>
#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/internal/syscall_handler.h>
#include <zephyr/sys/process.h>
#include <zephyr/sys/thread.h>
#include <ksched.h>

/* process-tier providers (process.c); this dispatch is their only consumer */
#ifdef CONFIG_SYS_PROCESS
int z_sys_clone_process(const struct sys_clone_args *args, k_pid_t *child);
int z_sys_clone_vm_copy(const struct sys_clone_args *args, k_pid_t *child);
#else
static inline int z_sys_clone_process(const struct sys_clone_args *args, k_pid_t *child)
{
	ARG_UNUSED(args);
	ARG_UNUSED(child);
	return -ENOTSUP;
}

static inline int z_sys_clone_vm_copy(const struct sys_clone_args *args, k_pid_t *child)
{
	ARG_UNUSED(args);
	ARG_UNUSED(child);
	return -ENOTSUP;
}
#endif /* CONFIG_SYS_PROCESS */

/* pooled creation (thread.c); stubbed when there is no pool to draw from */
#ifdef CONFIG_SYS_THREAD
int z_sys_thread_create_common(struct k_thread **th, k_thread_stack_t *stack, bool trusted,
			       uint32_t stack_size, uint32_t guard_size, k_thread_entry_t entry,
			       void *p1, void *p2, void *p3, int prio, uint32_t options,
			       k_timeout_t delay);
#else
static inline int z_sys_thread_create_common(struct k_thread **th, k_thread_stack_t *stack,
					     bool trusted, uint32_t stack_size,
					     uint32_t guard_size, k_thread_entry_t entry, void *p1,
					     void *p2, void *p3, int prio, uint32_t options,
					     k_timeout_t delay)
{
	return -EINVAL;
}
#endif /* CONFIG_SYS_THREAD */

static int clone_thread(const struct sys_clone_args *args, k_pid_t *child)
{
	int ret;
	k_tid_t tid;

	if (args->thread == NULL) {
		ret = z_sys_thread_create_common(&tid, args->stack, false, args->stack_size,
						 args->guard_size, args->entry, args->p1, args->p2,
						 args->p3, args->prio, args->options, K_FOREVER);
		if (ret < 0) {
			return ret;
		}
	} else {
		tid = k_thread_create(args->thread, args->stack, args->stack_size, args->entry,
				      args->p1, args->p2, args->p3, args->prio, args->options,
				      K_FOREVER);
#ifdef CONFIG_USERSPACE
		if (k_is_in_user_syscall()) {
			k_object_access_grant(tid, tid);
			k_object_access_grant(tid->stack_obj, tid);
		}
#endif /* CONFIG_USERSPACE */
	}

#ifdef CONFIG_SIGNAL
	if (args->sigmask != NULL) {
		bool enforce = IS_ENABLED(CONFIG_PROCESS_STOP);

		tid->base.sig.mask = *args->sigmask;
#ifdef CONFIG_USERSPACE
		enforce = enforce || k_is_in_user_syscall();
#endif /* CONFIG_USERSPACE */
		if (enforce) {
			/* as k_sig_mask(): requests cannot mask KILL or STOP */
			tid->base.sig.mask.sig[0] &=
				~(BIT64(K_SIG_KILL - 1) | BIT64(K_SIG_STOP - 1));
		}
#ifdef CONFIG_THREAD_CANCEL
		/* the reserved cancel bit in a requested mask is the cancellation state */
		if (k_sig_ismember(&tid->base.sig.mask, K_SIG_CANCEL) != 0) {
			tid->base.user_options |= K_CANCEL_DISABLED;
		} else {
			tid->base.user_options &= ~K_CANCEL_DISABLED;
		}
#endif /* CONFIG_THREAD_CANCEL */
	}
#endif /* CONFIG_SIGNAL */

	if (child != NULL) {
		*child = tid;
	}

	if ((args->flags & SYS_CLONE_PAUSED) == 0U) {
		k_thread_start(tid);
	}

	return 0;
}

int z_impl_sys_clone(const struct sys_clone_args *args, k_pid_t *child)
{
	if ((args == NULL) ||
	    ((args->flags & ~(SYS_CLONE_VM_COPY | SYS_CLONE_PAUSED | SYS_CLONE_THREAD)) != 0U) ||
	    ((args->flags & SYS_CLONE_THREAD) && (args->flags & SYS_CLONE_VM_COPY))) {
		return -EINVAL;
	}
	if ((args->sigmask != NULL) &&
	    (!IS_ENABLED(CONFIG_SIGNAL) || ((args->flags & SYS_CLONE_VM_COPY) != 0U))) {
		return -EINVAL;
	}
	if ((args->argv == NULL) && (args->envp != NULL)) {
		return -EINVAL;
	}
	/* staged vectors are for a plain process clone only */
	if ((args->argv != NULL) && ((args->flags & ~SYS_CLONE_PAUSED) != 0U)) {
		return -EINVAL;
	}
	if ((args->flags & SYS_CLONE_VM_COPY) != 0U) {
		return z_sys_clone_vm_copy(args, child);
	}
	/* an argument-staged child may take its entry from sys_exec_load() later */
	if ((args->entry == NULL) && (args->argv == NULL)) {
		return -EINVAL;
	}
	if ((args->thread != NULL) && ((args->stack == NULL) || (args->stack_size == 0))) {
		return -EINVAL;
	}
	if ((args->flags & SYS_CLONE_THREAD) != 0U) {
		/* a pool-drawn thread may still use a caller-provided stack */
		if ((args->thread == NULL) && !IS_ENABLED(CONFIG_SYS_THREAD)) {
			return -EINVAL;
		}
		return clone_thread(args, child);
	}
	if ((args->thread == NULL) &&
	    (!IS_ENABLED(CONFIG_SYS_THREAD) || (args->stack != NULL))) {
		/* a pool-drawn leader takes a pool stack */
		return -EINVAL;
	}

	return z_sys_clone_process(args, child);
}

#ifdef CONFIG_USERSPACE
#ifdef CONFIG_SYS_PROCESS
/* kernel copies of a user vector: the table and each string, budget-bounded */
static void clone_vec_free(char **vec)
{
	for (int i = 0; (vec != NULL) && (vec[i] != NULL); i++) {
		k_free(vec[i]);
	}
	k_free(vec);
}

static int clone_vec_from_user(char *const *uvec, char ***out, size_t *budget)
{
	char **vec;
	int n = 0;

	*out = NULL;
	if (uvec == NULL) {
		return 0;
	}
	if (K_HEAP_MEM_POOL_SIZE == 0) {
		/* nowhere to stage the vectors until the leader claims them */
		return -ENOTSUP;
	}
	for (;; n++) {
		char *p;

		K_OOPS(K_SYSCALL_MEMORY_READ(&uvec[n], sizeof(p)));
		p = uvec[n];
		if (p == NULL) {
			break;
		}
		if ((n + 1) * sizeof(char *) > *budget) {
			return -E2BIG;
		}
	}
	vec = k_calloc((size_t)n + 1, sizeof(char *));
	if (vec == NULL) {
		return -ENOMEM;
	}
	*budget -= (size_t)(n + 1) * sizeof(char *);
	for (int i = 0; i < n; i++) {
		vec[i] = k_usermode_string_alloc_copy(uvec[i], *budget);
		if (vec[i] == NULL) {
			clone_vec_free(vec);
			return -E2BIG;
		}
		*budget -= strlen(vec[i]) + 1;
	}
	*out = vec;

	return 0;
}

static int clone_user_vectors(struct sys_clone_args *copy)
{
	/* one budget for both vectors, matching the staging bound */
	size_t budget = CONFIG_SYS_PROCESS_ARG_BYTES;
	char **argv = NULL;
	char **envp = NULL;
	int ret;

	ret = clone_vec_from_user(copy->argv, &argv, &budget);
	if (ret == 0) {
		ret = clone_vec_from_user(copy->envp, &envp, &budget);
	}
	if (ret < 0) {
		clone_vec_free(argv);
		return ret;
	}
	copy->argv = argv;
	copy->envp = envp;

	return 0;
}

#endif /* CONFIG_SYS_PROCESS */

static inline int z_vrfy_sys_clone(const struct sys_clone_args *args, k_pid_t *child)
{
	struct sys_clone_args copy;
	size_t total_size, stack_obj_size;
	struct k_object *stack_object;
	int ret;
#ifdef CONFIG_SIGNAL
	struct k_sig_set mask_copy;
#endif /* CONFIG_SIGNAL */

	K_OOPS(k_usermode_from_copy(&copy, args, sizeof(copy)));
	if (child != NULL) {
		K_OOPS(K_SYSCALL_MEMORY_WRITE(child, sizeof(*child)));
	}
#ifdef CONFIG_SIGNAL
	if (copy.sigmask != NULL) {
		K_OOPS(k_usermode_from_copy(&mask_copy, copy.sigmask, sizeof(mask_copy)));
		copy.sigmask = &mask_copy;
	}
#endif /* CONFIG_SIGNAL */

	if (copy.thread == NULL) {
		/* pool-drawn thread: no caller objects to validate */
		K_OOPS(K_SYSCALL_VERIFY(copy.stack == NULL));
		K_OOPS(K_SYSCALL_VERIFY(copy.options & K_USER));
		K_OOPS(K_SYSCALL_VERIFY(!(copy.options & K_ESSENTIAL)));
		K_OOPS(K_SYSCALL_VERIFY(_is_valid_prio(copy.prio, NULL)));
		K_OOPS(K_SYSCALL_VERIFY(z_is_prio_lower_or_equal(copy.prio, _current->base.prio)));

#ifdef CONFIG_SYS_PROCESS
		if (copy.argv != NULL) {
			ret = clone_user_vectors(&copy);
			if (ret < 0) {
				return ret;
			}
		}
		ret = z_impl_sys_clone(&copy, child);
		clone_vec_free((char **)copy.argv);
		clone_vec_free((char **)copy.envp);

		return ret;
#else
		return z_impl_sys_clone(&copy, child);
#endif /* CONFIG_SYS_PROCESS */
	}

	/* mirror z_vrfy_k_thread_create(): the thread is created via k_thread_create() */
	K_OOPS(K_SYSCALL_OBJ_NEVER_INIT(copy.thread, K_OBJ_THREAD));
	stack_object = k_object_find(copy.stack);
	K_OOPS(K_SYSCALL_VERIFY_MSG(k_object_validation_check(stack_object, copy.stack,
							      K_OBJ_THREAD_STACK_ELEMENT,
							      _OBJ_INIT_FALSE) == 0,
				    "bad stack object"));
	K_OOPS(K_SYSCALL_VERIFY_MSG(!size_add_overflow(K_THREAD_STACK_RESERVED,
						       copy.stack_size, &total_size),
				    "stack size overflow (%zu+%zu)", copy.stack_size,
				    K_THREAD_STACK_RESERVED));
#ifdef CONFIG_GEN_PRIV_STACKS
	stack_obj_size = stack_object->data.stack_data->size;
#else
	stack_obj_size = stack_object->data.stack_size;
#endif /* CONFIG_GEN_PRIV_STACKS */
	K_OOPS(K_SYSCALL_VERIFY_MSG(total_size <= stack_obj_size,
				    "stack size %zu is too big, max is %zu", total_size,
				    stack_obj_size));
	K_OOPS(K_SYSCALL_VERIFY(copy.options & K_USER));
	K_OOPS(K_SYSCALL_VERIFY(!(copy.options & K_ESSENTIAL)));
	K_OOPS(K_SYSCALL_VERIFY(_is_valid_prio(copy.prio, NULL)));
	K_OOPS(K_SYSCALL_VERIFY(z_is_prio_lower_or_equal(copy.prio, _current->base.prio)));

#ifdef CONFIG_SYS_PROCESS
	if (copy.argv != NULL) {
		ret = clone_user_vectors(&copy);
		if (ret < 0) {
			return ret;
		}
	}
	ret = z_impl_sys_clone(&copy, child);
	clone_vec_free((char **)copy.argv);
	clone_vec_free((char **)copy.envp);

	return ret;
#else
	ARG_UNUSED(ret);
	return z_impl_sys_clone(&copy, child);
#endif /* CONFIG_SYS_PROCESS */
}
#include <zephyr/syscalls/sys_clone_mrsh.c>
#endif /* CONFIG_USERSPACE */
