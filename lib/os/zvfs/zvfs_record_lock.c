/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <limits.h>
#include <stdint.h>

#include <zephyr/fs/fs.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/fdtable.h>
#include <zephyr/sys/internal/fdtable_priv.h>
#include <zephyr/sys/zvfs_fs.h>

/* an l_len of 0 locks to the end of the file, whatever its size */
#define ZVFS_RECORD_LOCK_RANGE_END UINT64_MAX

struct zvfs_record_lock_entry {
	uint64_t start;
	uint64_t end;
	short type;
	pid_t pid;
};

struct zvfs_record_lock_state {
	void *obj;
	struct k_mutex mtx;
	uint8_t count;
	struct zvfs_record_lock_entry locks[CONFIG_ZVFS_FS_RECORD_LOCK_MAX];
};

static struct zvfs_record_lock_state record_lock_states[CONFIG_ZVFS_FS_DESC_MAX];

static pid_t record_lock_pid(void)
{
#if defined(CONFIG_POSIX_MULTI_PROCESS)
	extern pid_t getpid(void);

	return getpid();
#else
	return (pid_t)1;
#endif
}

static struct zvfs_record_lock_state *record_lock_state_get(void *obj, bool create)
{
	struct zvfs_record_lock_state *free_slot = NULL;

	for (int i = 0; i < ARRAY_SIZE(record_lock_states); i++) {
		if (record_lock_states[i].obj == obj) {
			return &record_lock_states[i];
		}

		if (record_lock_states[i].obj == NULL && free_slot == NULL) {
			free_slot = &record_lock_states[i];
		}
	}

	if (!create || free_slot == NULL) {
		return NULL;
	}

	free_slot->obj = obj;
	free_slot->count = 0;
	k_mutex_init(&free_slot->mtx);

	return free_slot;
}

void zvfs_record_lock_drop_obj(void *obj)
{
	struct zvfs_record_lock_state *state = record_lock_state_get(obj, false);

	if (state == NULL) {
		return;
	}

	state->obj = NULL;
	state->count = 0;
}

static int record_lock_file_size(struct zvfs_fs_desc *desc, uint64_t *size_out)
{
	int64_t cur;
	int64_t end;
	int rc;

	cur = fs_tell(&desc->file);
	if (cur < 0) {
		return -1;
	}

	rc = fs_seek(&desc->file, 0, FS_SEEK_END);
	if (rc < 0) {
		return rc;
	}

	end = fs_tell(&desc->file);
	if (end < 0) {
		return -1;
	}

	rc = fs_seek(&desc->file, cur, FS_SEEK_SET);
	if (rc < 0) {
		return rc;
	}

	*size_out = (uint64_t)end;
	return 0;
}

static int record_lock_resolve_range(struct zvfs_fs_desc *desc, const struct zvfs_flock *fl,
				     uint64_t *start_out, uint64_t *end_out)
{
	uint64_t base = 0;
	uint64_t start;
	uint64_t end;
	uint64_t file_size = 0;
	int rc;

	switch (fl->l_whence) {
	case ZVFS_SEEK_SET:
		base = 0;
		break;
	case ZVFS_SEEK_CUR: {
		int64_t cur = fs_tell(&desc->file);

		if (cur < 0) {
			errno = -cur;
			return -1;
		}
		base = cur;
		break;
	}
	case ZVFS_SEEK_END:
		rc = record_lock_file_size(desc, &file_size);
		if (rc < 0) {
			return rc;
		}
		base = file_size;
		break;
	default:
		errno = EINVAL;
		return -1;
	}

	if (UINT64_MAX - base < fl->l_start) {
		errno = EINVAL;
		return -1;
	}
	start = base + fl->l_start;

	if (fl->l_len == 0) {
		end = ZVFS_RECORD_LOCK_RANGE_END;
	} else {
		end = start + fl->l_len;
		if (end < start) {
			errno = EINVAL;
			return -1;
		}
	}

	*start_out = start;
	*end_out = end;
	return 0;
}

static bool record_lock_regions_overlap(uint64_t a_start, uint64_t a_end, uint64_t b_start, uint64_t b_end)
{
	return a_start < b_end && b_start < a_end;
}

static bool record_lock_types_conflict(short new_type, short existing_type)
{
	if (existing_type == ZVFS_F_UNLCK || new_type == ZVFS_F_UNLCK) {
		return false;
	}

	if (new_type == ZVFS_F_RDLCK && existing_type == ZVFS_F_RDLCK) {
		return false;
	}

	return true;
}

static int record_lock_remove_overlapping(struct zvfs_record_lock_state *state, uint64_t start,
					  uint64_t end, pid_t pid)
{
	uint8_t write_idx = 0;

	for (uint8_t read_idx = 0; read_idx < state->count; read_idx++) {
		struct zvfs_record_lock_entry *lock = &state->locks[read_idx];

		if (lock->pid == pid &&
		    record_lock_regions_overlap(lock->start, lock->end, start, end)) {
			continue;
		}

		if (write_idx != read_idx) {
			state->locks[write_idx] = state->locks[read_idx];
		}
		write_idx++;
	}

	state->count = write_idx;
	return 0;
}

static const struct zvfs_record_lock_entry *
record_lock_find_conflict(struct zvfs_record_lock_state *state, uint64_t start, uint64_t end,
			  short type, pid_t pid)
{
	for (uint8_t i = 0; i < state->count; i++) {
		const struct zvfs_record_lock_entry *lock = &state->locks[i];

		if (!record_lock_regions_overlap(lock->start, lock->end, start, end)) {
			continue;
		}

		if (lock->pid == pid) {
			continue;
		}

		if (record_lock_types_conflict(type, lock->type)) {
			return lock;
		}
	}

	return NULL;
}

static int record_lock_add(struct zvfs_record_lock_state *state, uint64_t start, uint64_t end,
			   short type, pid_t pid)
{
	struct zvfs_record_lock_entry *lock;

	if (state->count >= ARRAY_SIZE(state->locks)) {
		errno = ENOLCK;
		return -1;
	}

	lock = &state->locks[state->count++];
	lock->start = start;
	lock->end = end;
	lock->type = type;
	lock->pid = pid;

	return 0;
}

static int record_lock_apply(struct zvfs_record_lock_state *state, struct zvfs_fs_desc *desc,
			     struct zvfs_flock *fl, bool wait)
{
	uint64_t start;
	uint64_t end;
	pid_t pid = record_lock_pid();
	const struct zvfs_record_lock_entry *conflict;
	int rc;

	rc = record_lock_resolve_range(desc, fl, &start, &end);
	if (rc < 0) {
		return -1;
	}

	if (fl->l_type == ZVFS_F_UNLCK) {
		(void)record_lock_remove_overlapping(state, start, end, pid);
		return 0;
	}

	if (fl->l_type != ZVFS_F_RDLCK && fl->l_type != ZVFS_F_WRLCK) {
		errno = EINVAL;
		return -1;
	}

	do {
		(void)record_lock_remove_overlapping(state, start, end, pid);

		conflict = record_lock_find_conflict(state, start, end, fl->l_type, pid);
		if (conflict == NULL) {
			return record_lock_add(state, start, end, fl->l_type, pid);
		}

		if (!wait) {
			errno = EAGAIN;
			return -1;
		}

		k_mutex_unlock(&state->mtx);
		k_sleep(K_MSEC(1));
		(void)k_mutex_lock(&state->mtx, K_FOREVER);
	} while (true);
}

static int record_lock_getlk(struct zvfs_record_lock_state *state, struct zvfs_fs_desc *desc,
			     struct zvfs_flock *fl)
{
	uint64_t start;
	uint64_t end;
	const struct zvfs_record_lock_entry *conflict;
	pid_t pid = record_lock_pid();
	int rc;

	rc = record_lock_resolve_range(desc, fl, &start, &end);
	if (rc < 0) {
		return -1;
	}

	if (fl->l_type == ZVFS_F_UNLCK) {
		errno = EINVAL;
		return -1;
	}

	if (fl->l_type != ZVFS_F_RDLCK && fl->l_type != ZVFS_F_WRLCK) {
		errno = EINVAL;
		return -1;
	}

	conflict = record_lock_find_conflict(state, start, end, fl->l_type, pid);
	if (conflict == NULL) {
		fl->l_type = ZVFS_F_UNLCK;
		fl->l_pid = 0;
		return 0;
	}

	fl->l_type = conflict->type;
	fl->l_whence = ZVFS_SEEK_SET;
	fl->l_start = conflict->start;
	if (conflict->end == ZVFS_RECORD_LOCK_RANGE_END) {
		fl->l_len = 0;
	} else {
		fl->l_len = conflict->end - conflict->start;
	}
	fl->l_pid = conflict->pid;

	return 0;
}

int zvfs_record_lock_fcntl(struct fd_entry *entry, int cmd, struct zvfs_flock *fl)
{
	struct zvfs_fs_desc *desc;
	struct zvfs_record_lock_state *state;
	int rc;

	if (entry == NULL || fl == NULL) {
		errno = EINVAL;
		return -1;
	}

	if ((entry->mode & ZVFS_MODE_IFMT) != ZVFS_MODE_IFREG) {
		errno = EOPNOTSUPP;
		return -1;
	}

	desc = entry->obj;
	if (desc == NULL || desc->is_dir) {
		errno = EBADF;
		return -1;
	}

	state = record_lock_state_get(desc, true);
	if (state == NULL) {
		errno = ENOLCK;
		return -1;
	}

	(void)k_mutex_lock(&state->mtx, K_FOREVER);

	switch (cmd) {
	case ZVFS_F_GETLK:
		rc = record_lock_getlk(state, desc, fl);
		break;
	case ZVFS_F_SETLK:
		rc = record_lock_apply(state, desc, fl, false);
		break;
	case ZVFS_F_SETLKW:
		rc = record_lock_apply(state, desc, fl, true);
		break;
	default:
		errno = EINVAL;
		rc = -1;
		break;
	}

	k_mutex_unlock(&state->mtx);

	return rc;
}
