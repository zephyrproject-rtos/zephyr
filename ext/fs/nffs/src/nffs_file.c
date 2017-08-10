/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 * 
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

#include <assert.h>
#include <string.h>
#include <nffs/nffs.h>
#include <nffs/os.h>

static struct nffs_file *
nffs_file_alloc(void)
{
    struct nffs_file *file;

    file = nffs_os_mempool_get(&nffs_file_pool);
    if (file != NULL) {
        memset(file, 0, sizeof *file);
    }

    return file;
}

static int
nffs_file_free(struct nffs_file *file)
{
    int rc;

    if (file != NULL) {
        rc = nffs_os_mempool_free(&nffs_file_pool, file);
        if (rc != 0) {
            return FS_EOS;
        }
    }

    return 0;
}

/**
 * Creates a new empty file and writes it to the file system.  If a file with
 * the specified path already exists, the behavior is undefined.
 *
 * @param parent                The parent directory to insert the new file in.
 * @param filename              The name of the file to create.
 * @param filename_len          The length of the filename, in characters.
 * @param is_dir                1 if this is a directory; 0 if it is a normal
 *                                  file.
 * @param out_inode_entry       On success, this points to the inode
 *                                  corresponding to the new file.
 *
 * @return                      0 on success; nonzero on failure.
 */
int
nffs_file_new(struct nffs_inode_entry *parent, const char *filename,
              uint8_t filename_len, int is_dir,
              struct nffs_inode_entry **out_inode_entry)
{
    struct nffs_disk_inode disk_inode;
    struct nffs_inode_entry *inode_entry;
    uint32_t offset;
    uint8_t area_idx;
    int rc;

    rc = nffs_inode_entry_reserve(&inode_entry);
    if (rc != 0) {
        goto err;
    }

    rc = nffs_misc_reserve_space(sizeof disk_inode + filename_len,
                                 &area_idx, &offset);
    if (rc != 0) {
        goto err;
    }

    memset(&disk_inode, 0, sizeof disk_inode);
    if (is_dir) {
        disk_inode.ndi_id = nffs_hash_next_dir_id++;
    } else {
        disk_inode.ndi_id = nffs_hash_next_file_id++;
    }
    disk_inode.ndi_seq = 0;
    disk_inode.ndi_lastblock_id = NFFS_ID_NONE;
    if (parent == NULL) {
        disk_inode.ndi_parent_id = NFFS_ID_NONE;
    } else {
        disk_inode.ndi_parent_id = parent->nie_hash_entry.nhe_id;
    }
    disk_inode.ndi_filename_len = filename_len;
    disk_inode.ndi_flags = 0;
    nffs_crc_disk_inode_fill(&disk_inode, filename);

    rc = nffs_inode_write_disk(&disk_inode, filename, area_idx, offset);
    if (rc != 0) {
        goto err;
    }

    inode_entry->nie_hash_entry.nhe_id = disk_inode.ndi_id;
    inode_entry->nie_hash_entry.nhe_flash_loc =
        nffs_flash_loc(area_idx, offset);
    inode_entry->nie_refcnt = 1;
    inode_entry->nie_last_block_entry = NULL;

    if (parent != NULL) {
        rc = nffs_inode_add_child(parent, inode_entry);
        if (rc != 0) {
            goto err;
        }
    } else {
        assert(disk_inode.ndi_id == NFFS_ID_ROOT_DIR);
        nffs_inode_setflags(inode_entry, NFFS_INODE_FLAG_INTREE);
    }

    nffs_hash_insert(&inode_entry->nie_hash_entry);
    *out_inode_entry = inode_entry;

    return 0;

err:
    nffs_inode_entry_free(inode_entry);
    return rc;
}

/**
 * Performs a file open operation.
 *
 * @param out_file          On success, a pointer to the newly-created file
 *                              handle gets written here.
 * @param path              The path of the file to open.
 * @param access_flags      Flags controlling file access; see nffs_open() for
 *                              details.
 *
 * @return                  0 on success; nonzero on failure.
 */
int
nffs_file_open(struct nffs_file **out_file, const char *path,
               uint8_t access_flags)
{
    struct nffs_path_parser parser;
    struct nffs_inode_entry *parent;
    struct nffs_inode_entry *inode;
    struct nffs_file *file;
    int rc;

    file = NULL;

    /* Reject invalid access flag combinations. */
    if (!(access_flags & (FS_ACCESS_READ | FS_ACCESS_WRITE))) {
        rc = FS_EINVAL;
        goto err;
    }
    if (access_flags & (FS_ACCESS_APPEND | FS_ACCESS_TRUNCATE) &&
        !(access_flags & FS_ACCESS_WRITE)) {

        rc = FS_EINVAL;
        goto err;
    }
    if (access_flags & FS_ACCESS_APPEND &&
        access_flags & FS_ACCESS_TRUNCATE) {

        rc = FS_EINVAL;
        goto err;
    }

    file = nffs_file_alloc();
    if (file == NULL) {
        rc = FS_ENOMEM;
        goto err;
    }

    nffs_path_parser_new(&parser, path);
    rc = nffs_path_find(&parser, &inode, &parent);
    if (rc == FS_ENOENT && parser.npp_token_type == NFFS_PATH_TOKEN_LEAF) {
        /* The path is valid, but the file does not exist.  This is an error
         * for read-only opens.
         */
        if (!(access_flags & FS_ACCESS_WRITE)) {
            goto err;
        }

        /* Make sure the parent directory exists. */
        if (parent == NULL) {
            goto err;
        }

        /* Create a new file at the specified path. */
        rc = nffs_file_new(parent, parser.npp_token, parser.npp_token_len, 0,
                           &file->nf_inode_entry);
        if (rc != 0) {
            goto err;
        }
    } else if (rc == 0) {
        /* The file already exists. */

        /* Reject an attempt to open a directory. */
        if (nffs_hash_id_is_dir(inode->nie_hash_entry.nhe_id)) {
            rc = FS_EINVAL;
            goto err;
        }

        if (access_flags & FS_ACCESS_TRUNCATE) {
            /* The user is truncating the file.  Unlink the old file and create
             * a new one in its place.
             */
            nffs_path_unlink(path);
            rc = nffs_file_new(parent, parser.npp_token, parser.npp_token_len,
                               0, &file->nf_inode_entry);
            if (rc != 0) {
                goto err;
            }
        } else {
            /* The user is not truncating the file.  Point the file handle to
             * the existing inode.
             */
            file->nf_inode_entry = inode;
        }
    } else {
        /* Invalid path. */
        goto err;
    }

    if (access_flags & FS_ACCESS_APPEND) {
        rc = nffs_inode_data_len(file->nf_inode_entry, &file->nf_offset);
        if (rc != 0) {
            goto err;
        }
    } else {
        file->nf_offset = 0;
    }
    nffs_inode_inc_refcnt(file->nf_inode_entry);
    file->nf_access_flags = access_flags;

    *out_file = file;

    return 0;

err:
    nffs_file_free(file);
    return rc;
}

/**
 * Positions a file's read and write pointer at the specified offset.  The
 * offset is expressed as the number of bytes from the start of the file (i.e.,
 * seeking to 0 places the pointer at the first byte in the file).
 *
 * @param file              The file to reposition.
 * @param offset            The offset from the start of the file to seek to.
 *
 * @return                  0 on success; nonzero on failure.
 */
int
nffs_file_seek(struct nffs_file *file, uint32_t offset)
{ 
    uint32_t len;
    int rc;

    rc = nffs_inode_data_len(file->nf_inode_entry, &len);
    if (rc != 0) {
        return rc;
    }

    if (offset > len) {
        return FS_EOFFSET;
    }

    file->nf_offset = offset;
    return 0;
}

/**
 * Reads data from the specified file.  If more data is requested than remains
 * in the file, all available data is retrieved.  Note: this type of short read
 * results in a success return code.
 *
 * @param file              The file to read from.
 * @param len               The number of bytes to attempt to read.
 * @param out_data          The destination buffer to read into.
 * @param out_len           On success, the number of bytes actually read gets
 *                              written here.  Pass null if you don't care.
 *
 * @return                  0 on success; nonzero on failure.
 */
int
nffs_file_read(struct nffs_file *file, uint32_t len, void *out_data,
               uint32_t *out_len)
{
    uint32_t bytes_read;
    int rc;

    if (!nffs_misc_ready()) {
        return FS_EUNINIT;
    }

    if (!(file->nf_access_flags & FS_ACCESS_READ)) {
        return FS_EACCESS;
    }

    rc = nffs_inode_read(file->nf_inode_entry, file->nf_offset, len, out_data,
                        &bytes_read);
    if (rc != 0) {
        return rc;
    }

    file->nf_offset += bytes_read;
    if (out_len != NULL) {
        *out_len = bytes_read;
    }

    return 0;
}

/**
 * Closes the specified file and invalidates the file handle.  If the file has
 * already been unlinked, and this is the last open handle to the file, this
 * operation causes the file to be deleted.
 *
 * @param file              The file handle to close.
 *
 * @return                  0 on success; nonzero on failure.
 */
int
nffs_file_close(struct nffs_file *file)
{
    int rc;

    rc = nffs_inode_dec_refcnt(file->nf_inode_entry);
    if (rc != 0) {
        return rc;
    }

    rc = nffs_file_free(file);
    if (rc != 0) {
        return rc;
    }

    return 0;
}
