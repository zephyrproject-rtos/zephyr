/*
 * Copyright (c) 2019 Peter Bigot Consulting, LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <errno.h>
#include <zephyr/sys/__assert.h>
#include <ztest.h>
#include "testfs_util.h"

static const char *path_vextend(struct testfs_path *pp,
				va_list ap)
{
	const char *ep = va_arg(ap, const char *);
	const char *endp = pp->path + sizeof(pp->path);

	while (ep) {
		size_t len = strlen(ep);
		char *eos = pp->eos;
		size_t rem = (endp - eos);

		if (strcmp(ep, "..") == 0) {
			char *sp = strrchr(pp->path, '/');

			if (sp == pp->path) {
				eos = sp + 1;
			} else {
				eos = sp;
			}
		} else if ((1 + len + 1) < rem) { /* /, ep, EOS */
			*eos = '/';
			++eos;
			memcpy(eos, ep, len);
			eos += len;
		} else {
			break;
		}
		*eos = 0;
		pp->eos = eos;

		ep = va_arg(ap, const char *);
	}
	return pp->path;
}

const char *testfs_path_init(struct testfs_path *pp,
			     const struct fs_mount_t *mp,
			     ...)
{
	va_list ap;

	if (mp == NULL) {
		pp->path[0] = '/';
		pp->eos = pp->path + 1;
	} else {
		size_t len = strlen(mp->mnt_point);

		__ASSERT('/' == mp->mnt_point[0], "relative mount point");
		if ((len + 1) >= sizeof(pp->path)) {
			len = sizeof(pp->path) - 1;
		}
		strncpy(pp->path, mp->mnt_point, len);
		pp->eos = pp->path + len;
	}
	*pp->eos = '\0';

	va_start(ap, mp);
	path_vextend(pp, ap);
	va_end(ap);
	return pp->path;
}

const char *testfs_path_extend(struct testfs_path *pp,
			       ...)
{
	va_list ap;

	va_start(ap, pp);
	path_vextend(pp, ap);
	va_end(ap);

	return pp->path;
}

int testfs_write_constant(struct fs_file_t *fp,
			  uint8_t value,
			  unsigned int len)
{
	uint8_t buffer[TESTFS_BUFFER_SIZE];
	unsigned int rem = len;

	memset(buffer, value, sizeof(buffer));

	while (rem > 0) {
		unsigned int count = sizeof(buffer);

		if (count > rem) {
			count = rem;
		}

		ssize_t rc = fs_write(fp, buffer, count);

		if (rc < 0) {
			return rc;
		}

		rem -= count;
	}

	return len;
}

int testfs_verify_constant(struct fs_file_t *fp,
			   uint8_t value,
			   unsigned int len)
{
	uint8_t buffer[TESTFS_BUFFER_SIZE];
	unsigned int match = 0;

	while (len > 0) {
		unsigned int count = sizeof(buffer);

		if (count > len) {
			count = len;
		}

		int rc = fs_read(fp, buffer, count);

		if (rc < 0) {
			return rc;
		}

		if (rc > count) {
			return -EIO;
		}

		for (unsigned int i = 0; i < rc; ++i) {
			if (value != buffer[i]) {
				break;
			}
			++match;
		}

		if (rc < count) {
			break;
		}

		len -= count;
	}
	return match;
}

int testfs_write_incrementing(struct fs_file_t *fp,
			      uint8_t value,
			      unsigned int len)
{
	uint8_t buffer[TESTFS_BUFFER_SIZE];
	unsigned int rem = len;

	while (rem > 0) {
		unsigned int count = sizeof(buffer);

		if (count > rem) {
			count = rem;
		}

		for (unsigned int i = 0; i < count; ++i) {
			buffer[i] = value++;
		}

		ssize_t rc = fs_write(fp, buffer, count);

		if (rc < 0) {
			return rc;
		}

		rem -= count;
	}

	return len;
}

int testfs_verify_incrementing(struct fs_file_t *fp,
			       uint8_t value,
			       unsigned int len)
{
	uint8_t buffer[TESTFS_BUFFER_SIZE];
	unsigned int match = 0;

	while (len > 0) {
		unsigned int count = sizeof(buffer);

		if (count > len) {
			count = len;
		}

		int rc = fs_read(fp, buffer, count);

		if (rc < 0) {
			return rc;
		}

		if (rc > count) {
			return -EIO;
		}

		for (unsigned int i = 0; i < rc; ++i) {
			if (value++ != buffer[i]) {
				break;
			}
			++match;
		}

		if (rc < count) {
			break;
		}

		len -= count;
	}
	return match;
}

int testfs_build(struct testfs_path *root,
		 const struct testfs_bcmd *cp)
{
	int rc;

	while (!TESTFS_BCMD_IS_END(cp)) {

		if (TESTFS_BCMD_IS_FILE(cp)) {
			struct fs_file_t file;

			fs_file_t_init(&file);
			rc = fs_open(&file,
				     testfs_path_extend(root,
							cp->name,
							TESTFS_PATH_END),
				     FS_O_CREATE | FS_O_RDWR);
			TC_PRINT("create at %s with %u from 0x%02x: %d\n",
				 root->path, cp->size, cp->value, rc);
			if (rc == 0) {
				rc = testfs_write_incrementing(&file, cp->value, cp->size);
				(void)fs_close(&file);
			}
			testfs_path_extend(root, "..", TESTFS_PATH_END);

			if (rc < 0) {
				TC_PRINT("FAILED create/write %s: %d\n",
					 root->path, rc);
				return rc;
			}

		} else if (TESTFS_BCMD_IS_ENTER_DIR(cp)) {
			testfs_path_extend(root,
					   cp->name,
					   TESTFS_PATH_END);
			rc = fs_mkdir(root->path);
			TC_PRINT("mkdir %s: %d\n", root->path, rc);
			if (rc < 0) {
				return rc;
			}
		} else if (TESTFS_BCMD_IS_EXIT_DIR(cp)) {
			TC_PRINT("exit directory %s\n", root->path);
			testfs_path_extend(root, "..", TESTFS_PATH_END);
		} else {
			TC_PRINT("ERROR: unexpected build command");
			return -EINVAL;
		}
		++cp;
	}
	return 0;
}

/* Check the found entry against a probably match.  Recurse into
 * matched directories.
 *
 * Sets the matched field of *cp if all tests pass.
 *
 * Return a negative error, or a nonnegative count of foreign
 * files.
 */
static int check_layout_entry(struct testfs_path *pp,
			      const struct fs_dirent *statp,
			      struct testfs_bcmd *cp,
			      struct testfs_bcmd *ecp)
{
	int rc = 0;
	unsigned int foreign = 0;

	/* Create the full path */
	testfs_path_extend(pp, statp->name,
			   TESTFS_PATH_END);

	/* Also check file content */
	if (statp->type == FS_DIR_ENTRY_FILE) {
		struct fs_file_t file;

		fs_file_t_init(&file);
		rc = fs_open(&file, pp->path, FS_O_CREATE | FS_O_RDWR);
		if (rc < 0) {
			TC_PRINT("%s: content check open failed: %d\n",
				 pp->path, rc);
			rc = -ENOENT;
			goto out;
		}
		rc = testfs_verify_incrementing(&file, cp->value, cp->size);
		if (rc != cp->size) {
			TC_PRINT("%s: content check failed: %d\n",
				 pp->path, rc);
			if (rc >= 0) {
				rc = -EIO;
			}
			goto out;
		}
		rc = fs_close(&file);
		if (rc != 0) {
			TC_PRINT("%s: content check close failed: %d\n",
				 pp->path, rc);
		}
	} else if (statp->type == FS_DIR_ENTRY_DIR) {
		rc = testfs_bcmd_verify_layout(pp,
					       cp + 1,
					       testfs_bcmd_exitdir(cp, ecp));
		if (rc >= 0) {
			foreign = rc;
		}
	}

out:
	testfs_path_extend(pp, "..",
			   TESTFS_PATH_END);

	if (rc >= 0) {
		cp->matched = true;
		rc = foreign;
	}

	return rc;
}

int testfs_bcmd_verify_layout(struct testfs_path *pp,
			      struct testfs_bcmd *scp,
			      struct testfs_bcmd *ecp)
{
	struct fs_dir_t dir;
	unsigned int count = 0;
	unsigned int foreign = 0;
	struct testfs_bcmd *cp = scp;

	while (cp < ecp) {
		cp->matched = false;
		++cp;
	}

	fs_dir_t_init(&dir);

	int rc = fs_opendir(&dir, pp->path);

	if (rc != 0) {
		TC_PRINT("%s: opendir failed: %d\n", pp->path, rc);
		if (rc > 0) {
			rc = -EIO;
		}
		return rc;
	}

	TC_PRINT("check %s for %zu entries\n", pp->path, ecp - scp);
	while (rc >= 0) {
		struct fs_dirent stat;

		rc = fs_readdir(&dir, &stat);
		if (rc != 0) {
			TC_PRINT("readdir failed: %d", rc);
			rc = -EIO;
			break;
		}

		if (stat.name[0] == '\0') {
			break;
		}

		++count;

		cp = testfs_bcmd_find(&stat, scp, ecp);

		bool dotdir = ((stat.type == FS_DIR_ENTRY_DIR)
			       && ((strcmp(stat.name, ".") == 0)
				   || (strcmp(stat.name, "..") == 0)));

		TC_PRINT("%s %s%s%s %zu\n", pp->path,
			 stat.name,
			 (stat.type == FS_DIR_ENTRY_FILE) ? "" : "/",
			 dotdir ? " SYNTHESIZED"
			 : (cp == NULL) ? " FOREIGN"
			 : "",
			 stat.size);

		if (dotdir) {
			zassert_true(false,
				     "special directories observed");
		} else if (cp != NULL) {
			rc = check_layout_entry(pp, &stat, cp, ecp);
			if (rc > 0) {
				foreign += rc;
			}
		} else {
			foreign += 1;
		}
	}
	TC_PRINT("%s found %u entries, %u foreign\n", pp->path, count, foreign);

	int rc2 = fs_closedir(&dir);

	if (rc2 != 0) {
		TC_PRINT("%s: closedir failed: %d\n",
			 pp->path, rc2);
		if (rc >= 0) {
			rc = (rc2 >= 0) ? -EIO : rc2;
		}
	}

	if (rc >= 0) {
		rc = foreign;
	}

	return rc;
}

struct testfs_bcmd *testfs_bcmd_exitdir(struct testfs_bcmd *cp,
					struct testfs_bcmd *ecp)
{
	unsigned int level = 1;

	/* Skip to the paired EXIT_DIR */
	while ((level > 0) && (++cp < ecp)) {
		if (TESTFS_BCMD_IS_ENTER_DIR(cp)) {
			++level;
		} else if (TESTFS_BCMD_IS_EXIT_DIR(cp)) {
			--level;
		}
	}
	return cp;
}

struct testfs_bcmd *testfs_bcmd_find(struct fs_dirent *statp,
				     struct testfs_bcmd *scp,
				     struct testfs_bcmd *ecp)
{
	struct testfs_bcmd *cp = scp;

	while (cp < ecp) {
		if ((cp->type == statp->type)
		    && (cp->name != NULL)
		    && (strcmp(cp->name, statp->name) == 0)
		    && (cp->size == statp->size)) {
			return cp;
		}

		if (TESTFS_BCMD_IS_ENTER_DIR(cp)) {
			cp = testfs_bcmd_exitdir(cp, ecp);
		}
		++cp;
	}

	return NULL;
}
