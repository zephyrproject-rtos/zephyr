/*
 * Copyright 2025 Linkedsemi
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdarg.h>
#include <zephyr/fs/fs.h>
#include <zephyr/fs/fs_sys.h>
#include <zephyr/shell/shell.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/fdtable.h>

LOG_MODULE_REGISTER(devfs, LOG_LEVEL_INF);

#define DEVFS_FILE_NUM_MAX              (CONFIG_DEVFS_FILE_NUM_MAX)
#define DEVFS_OPENDIR_NUM_MAX           (16)

struct devfs_dir
{
    struct devfs_node *cur;
};

struct devfs_node
{
    const struct fs_file_system_t *ops;
    struct devfs_node *parent;
    struct devfs_node *child;
    struct devfs_node *brother;
    const char *abs_path;
    int is_dir;
    char name[MAX_FILE_NAME+1];
};

struct devfs_file_object
{
    const struct fs_file_system_t *ops;
    struct fs_file_t filp;
    char file_name[MAX_FILE_NAME+1];
};

struct devfs
{
    struct devfs_node root;
    struct devfs_node anon_root;
    struct k_spinlock lock;
};

static struct devfs devfs;

K_MEM_SLAB_DEFINE_STATIC(file_obj_pool, sizeof(struct devfs_file_object), DEVFS_FILE_NUM_MAX, 4);
K_MEM_SLAB_DEFINE_STATIC(node_pool, sizeof(struct devfs_node), DEVFS_FILE_NUM_MAX, 4);
K_MEM_SLAB_DEFINE_STATIC(dir_pool, sizeof(struct devfs_dir), DEVFS_OPENDIR_NUM_MAX, 4);

static int devfs_path_valid_check(const char *path)
{
    const char *p = path + 5; // skip /dev/

    while (*p)
    {
        size_t len = 0;
        while (*p && *p != '/')
        {
            if (len >= MAX_FILE_NAME)
            {
                LOG_ERR("path too long '%s'", path);
                return -ENAMETOOLONG;
            }
            p++;
            len++;
        }
        if (*p == '/')
            p++;
    }

    return 0;
}

static int devfs_walk_path(struct devfs_node *root, const char *path,
    struct devfs_node **parent,
    struct devfs_node **cur,
    struct devfs_node **prev)
{
    const char *p;
    struct devfs_node *c;
    struct devfs_node *pnode = NULL;
    struct devfs_node *pprev = NULL;
    char name[MAX_FILE_NAME + 1];

    if (!path)
        return -EINVAL;

    if (strncmp(path, "/dev", 4) == 0 &&
        (path[4] == '\0' || (path[4] == '/' && path[5] == '\0')))
    {
        *parent = NULL;
        *cur = root;
        *prev = NULL;
        return 0;
    }

    if (devfs_path_valid_check(path) != 0)
        return -EINVAL;

    p = path + 5;
    c = root;

    while (*p)
    {
        size_t len = 0;

        while (*p && *p != '/')
        {
            name[len++] = *p++;
        }
        name[len] = '\0';

        pnode = c;
        pprev = NULL;
        c = c->child;

        while (c && strcmp(c->name, name) != 0)
        {
            pprev = c;
            c = c->brother;
        }

        if (!c)
            return -ENOENT;

        if (*p == '/')
            p++;
    }

    *parent = pnode;
    *cur = c;
    *prev = pprev;
    return 0;
}

static struct devfs_node *devfs_lookup_node(struct devfs_node *root, const char *path)
{
    struct devfs_node *cur;
    struct devfs_node *parent;
    struct devfs_node *prev;
    int ret;

    k_spinlock_key_t key = k_spin_lock(&devfs.lock);

    ret = devfs_walk_path(root, path, &parent, &cur, &prev);

    k_spin_unlock(&devfs.lock, key);

    return ret == 0 ? cur : NULL;
}

static struct devfs_node* devfs_lookup(const char *path)
{
    struct devfs_node* node = devfs_lookup_node(&devfs.root, path);
    return node ? node : devfs_lookup_node(&devfs.anon_root, path);
}

static int devfs_open(struct fs_file_t *zfp, const char *path, fs_mode_t flags)
{
    int rc = -ENOTSUP;
    struct devfs_node *node = devfs_lookup(path);

    if (!node || node->is_dir || !node->ops->open)
    {
        LOG_ERR("invalid node or not openable for path '%s'", path);
        return -EINVAL;
    }

    struct devfs_file_object *file_obj = NULL;

    int ret = k_mem_slab_alloc(&file_obj_pool, (void **)&file_obj, K_NO_WAIT);
    if (ret != 0 || file_obj == NULL)
    {
        LOG_ERR("failed to allocate file object");
        return -ENOMEM;
    }

    file_obj->ops = node->ops;
    file_obj->filp.filep = NULL;
    file_obj->filp.mp = NULL;
    file_obj->filp.flags = 0;
    strncpy(file_obj->file_name, node->name, MAX_FILE_NAME);
    file_obj->file_name[MAX_FILE_NAME] = '\0';

    rc = file_obj->ops->open(&file_obj->filp, file_obj->file_name, flags);
    if (rc < 0)
    {
        LOG_ERR("file open failed (%d)", rc);
        k_mem_slab_free(&file_obj_pool, file_obj);
        return rc;
    }

    file_obj->filp.flags = flags;
    zfp->filep = file_obj;

    return rc;
}

static int devfs_close(struct fs_file_t *zfp)
{
    int rc = -ENOTSUP;
    struct devfs_file_object *file_obj = zfp->filep;

    if (file_obj->ops->close == NULL)
    {
        LOG_ERR("invalid file pointer or close not supported"); 
        return rc;
    }

    rc = file_obj->ops->close(&file_obj->filp);
    if (rc < 0)
    {
        LOG_ERR("file close error (%d)", rc);
        return rc;
    }
    k_mem_slab_free(&file_obj_pool, file_obj);

    return rc;
}

static ssize_t devfs_read(struct fs_file_t *zfp, void *dest, size_t nbytes)
{
    ssize_t rc = -ENOTSUP;
    struct devfs_file_object *file_obj = zfp->filep;

    if (file_obj->ops->read == NULL)
    {
        LOG_ERR("invalid file pointer or read not supported");
        return rc;
    }

    rc = file_obj->ops->read(&file_obj->filp, dest, nbytes);
    if (rc < 0)
    {
        LOG_ERR("file read error (%d)", rc);
        return rc;
    }

    return rc;
}

static ssize_t devfs_write(struct fs_file_t *zfp, const void *src, size_t nbytes)
{
    ssize_t rc = -ENOTSUP;
    struct devfs_file_object *file_obj = zfp->filep;

    if (file_obj->ops->write == NULL)
    {
        LOG_ERR("invalid file pointer or write not supported");
        return rc;
    }

    rc = file_obj->ops->write(&file_obj->filp, src, nbytes);
    if (rc < 0)
    {
        LOG_ERR("file write error (%d)", rc);
        return rc;
    }

    return rc;
}

static int devfs_ioctl(struct fs_file_t *zfp, unsigned long cmd, va_list args)
{
    int rc = -EINVAL;
    struct devfs_file_object *file_obj = zfp->filep;

    if (file_obj->ops->ioctl == NULL)
    {
        LOG_ERR("invalid file pointer or ioctl not supported");
        return -ENOTSUP;
    }

    if (cmd == ZFD_IOCTL_SET_LOCK)
        return 0;

    rc = file_obj->ops->ioctl(&file_obj->filp, cmd, args);
    if (rc < 0)
    {
        LOG_ERR("file ioctl error (%d)", rc);
        return rc;
    }

    return rc;
}

static int devfs_mount(struct fs_mount_t *mountp)
{
    memset(&devfs, 0, sizeof(struct devfs));
    devfs.root.is_dir = 1;
    devfs.root.abs_path = "/dev";
    strcpy(devfs.root.name, "dev");
    devfs.anon_root = devfs.root;

    return 0;
}

static void devfs_free_node(struct devfs_node *node)
{
    if (!node)
        return;

    devfs_free_node(node->child);
    devfs_free_node(node->brother);

    if (node != &devfs.root)
        k_mem_slab_free(&node_pool, node);
}

static int devfs_unmount(struct fs_mount_t *mountp)
{
    /* free dir tree */
    devfs_free_node(devfs.root.child);
    devfs.root.child = NULL;
    return 0;
}

static int devfs_opendir(struct fs_dir_t *dirp, const char *fs_path)
{
    struct devfs_node *node = devfs_lookup_node(&devfs.root, fs_path);
    if (!node || !node->is_dir)
    {
        LOG_ERR("path not found or not a directory '%s'", fs_path);
        return -EINVAL;
    }

    struct devfs_dir *dir = NULL;
    if (k_mem_slab_alloc(&dir_pool, (void **)&dir, K_NO_WAIT))
    {
        LOG_ERR("dir pool full , opendir '%s' fail", fs_path);
        return -ENOMEM;
    }

    dirp->dirp = dir;
    dir->cur = node->child;

    return 0;
}

static int devfs_readdir(struct fs_dir_t *dirp, struct fs_dirent *entry)
{
    struct devfs_dir *d = dirp->dirp;
    struct devfs_node *cur = d->cur;

    if (cur)
    {
        strncpy(entry->name, cur->name, MAX_FILE_NAME);
        entry->type = cur->is_dir ? FS_DIR_ENTRY_DIR : FS_DIR_ENTRY_FILE;
        d->cur = cur->brother;
    }
    else
    {
        memset(entry, 0, sizeof(*entry));
    }

    return 0;
}

static int devfs_clsoedir(struct fs_dir_t *dirp)
{
    k_mem_slab_free(&dir_pool, dirp->dirp);
    return 0;
}

static int devfs_stat(struct fs_mount_t *mountp, const char *path, struct fs_dirent *entry)
{
    struct devfs_node* node = devfs_lookup_node(&devfs.root, path);
    if (!node)
    {
        LOG_ERR("path not found '%s'", path); 
        return -EINVAL;
    }

    strncpy(entry->name, node->name, MAX_FILE_NAME);
    entry->name[MAX_FILE_NAME] = '\0';
    entry->type = node->is_dir;
    entry->size = 0;

    return 0;
}

static int devfs_statvfs(struct fs_mount_t *mountp, const char *path, struct fs_statvfs *stat)
{
    struct devfs_node* node = devfs_lookup_node(&devfs.root, path);
    if (!node)
    {
        LOG_ERR("path not found '%s'", path); 
        return -EINVAL;
    }

    stat->f_bsize  = 1024;
    stat->f_frsize = 1024;
    stat->f_blocks = 1;
    stat->f_bfree  = 1;

    return 0;
}

static int devfs_node_remove(const char *path, bool is_visible)
{
    struct devfs_node *root = is_visible ? &devfs.root : &devfs.anon_root;
    struct devfs_node *cur;
    struct devfs_node *parent;
    struct devfs_node *prev;
    int ret;

    k_spinlock_key_t key = k_spin_lock(&devfs.lock);

    ret = devfs_walk_path(root, path, &parent, &cur, &prev);
    if (ret)
        goto out;

    if (!parent)
    {
        ret = -EINVAL;
        goto out;
    }

    if (cur->is_dir && cur->child)
    {
        ret = -ENOTEMPTY;
        goto out;
    }

    if (prev)
        prev->brother = cur->brother;
    else
        parent->child = cur->brother;

out:
    k_spin_unlock(&devfs.lock, key);

    if (ret == 0)
        k_mem_slab_free(&node_pool, cur);

    return ret;
}

static int devfs_node_create(const char *path, bool is_visible, const struct fs_file_system_t *ops)
{
    if (!path || strncmp(path, "/dev/", 5) != 0 || devfs_path_valid_check(path))
    {
        LOG_ERR("invalid path '%s'", path);
        return -EINVAL;
    }

    int ret = 0;
    const char *p = path + 5;
    struct devfs_node *cur = is_visible ? &devfs.root : &devfs.anon_root;

    char name[MAX_FILE_NAME + 1];
    k_spinlock_key_t key = k_spin_lock(&devfs.lock);

    while (*p)
    {
        size_t len = 0;

        while (*p && *p != '/')
            name[len++] = *p++;
        name[len] = '\0';

        bool is_last = (*p == '\0');

        struct devfs_node *child = cur->child;
        struct devfs_node *prev  = NULL;

        while (child && strcmp(child->name, name) != 0)
        {
            prev = child;
            child = child->brother;
        }

        if (child)
        {
            if (is_last)
            {
                LOG_ERR("file already exists '%s'", path);
                ret = -EEXIST;
                goto out;
            }

            if (!child->is_dir)
            {
                LOG_ERR("'%s' is not a directory", child->name);
                ret = -ENOTDIR;
                goto out;
            }
        }
        else
        {
            ret = k_mem_slab_alloc(&node_pool,
                                   (void **)&child,
                                   K_NO_WAIT);
            if (ret || !child)
            {
                LOG_ERR("node pool full, create '%s' failed", path);
                ret = -ENOMEM;
                goto out;
            }

            memset(child, 0, sizeof(*child));
            memcpy(child->name, name, len + 1);
            child->parent = cur;

            if (prev)
                prev->brother = child;
            else
                cur->child = child;

            if (!is_last)
            {
                child->is_dir = 1;
            }
            else
            {
                child->is_dir = 0;
                child->abs_path = path;
                child->ops = ops;
                break;
            }
        }

        cur = child;
        if (*p == '/')
            p++;
    }

out:
    k_spin_unlock(&devfs.lock, key);
    return ret;
}

static void devfs_print_node(struct devfs_node *node, int depth)
{
    if (!node)
        return;

    for (int i = 0; i < depth; i++)
        printk("  ");

    if (node->is_dir)
        printk("[DIR ] %s\n", node->name);
    else
        printk("[FILE] %s\n", node->name);

    if (node->child)
        devfs_print_node(node->child, depth + 1);

    if (node->brother)
        devfs_print_node(node->brother, depth);
}

static void devfs_print_tree(const struct shell *sh, size_t argc, char **argv)
{
    printk("========== devfs tree (visible) ==========\n");
    k_spinlock_key_t key = k_spin_lock(&devfs.lock);
    devfs_print_node(&devfs.root, 0);
    k_spin_unlock(&devfs.lock, key);
    printk("\n========== devfs tree (anonymous) ==========\n");
    key = k_spin_lock(&devfs.lock);
    devfs_print_node(&devfs.anon_root, 0);
    k_spin_unlock(&devfs.lock, key);
    printk("===========================================\n");
}
SHELL_CMD_REGISTER(devfs_print_tree, NULL, "devfs_print_tree", devfs_print_tree);

int devfs_unregister(const char *path)
{
    return devfs_node_remove(path, 1);
}

int devfs_unregister_anon(const char *path)
{
    return devfs_node_remove(path, 0);
}

int devfs_register(const char *path, const struct fs_file_system_t *devfs_ops)
{
    return devfs_node_create(path, 1, devfs_ops);
}

int devfs_register_anon(const char *path, const struct fs_file_system_t *devfs_ops)
{
    return devfs_node_create(path, 0, devfs_ops);
}

static const struct fs_file_system_t devfs_fs = {
    .mount = devfs_mount,
    .unmount = devfs_unmount,
    .open = devfs_open,
    .read = devfs_read,
    .write = devfs_write,
    .close = devfs_close,
    .ioctl = devfs_ioctl,
    .opendir = devfs_opendir,
    .readdir = devfs_readdir,
    .closedir = devfs_clsoedir,
    .stat = devfs_stat,
    .statvfs = devfs_statvfs
};

static int devfs_fs_init(void)
{
    static struct fs_mount_t devfs_fs_mnt = {
        .type = FS_DEVFS,
        .mnt_point = "/dev",
        .fs_data = &devfs
    };

    int ret = fs_register(FS_DEVFS, &devfs_fs);
    if (!ret)
        ret = fs_mount(&devfs_fs_mnt);

    return ret;
}
SYS_INIT(devfs_fs_init, POST_KERNEL, CONFIG_FILE_SYSTEM_INIT_PRIORITY);
