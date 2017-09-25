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

int
nffs_path_parse_next(struct nffs_path_parser *parser)
{
    const char *slash_start;
    int token_end;
    int token_len;

    if (parser->npp_token_type == NFFS_PATH_TOKEN_LEAF) {
        return FS_EINVAL;
    }

    slash_start = strchr(parser->npp_path + parser->npp_off, '/');
    if (slash_start == NULL) {
        if (parser->npp_token_type == NFFS_PATH_TOKEN_NONE) {
            return FS_EINVAL;
        }
        parser->npp_token_type = NFFS_PATH_TOKEN_LEAF;
        token_len = strlen(parser->npp_path + parser->npp_off);
    } else {
        parser->npp_token_type = NFFS_PATH_TOKEN_BRANCH;
        token_end = slash_start - parser->npp_path;
        token_len = token_end - parser->npp_off;
    }

    if (token_len > NFFS_FILENAME_MAX_LEN) {
        return FS_EINVAL;
    }

    parser->npp_token = parser->npp_path + parser->npp_off;
    parser->npp_token_len = token_len;
    parser->npp_off += token_len + 1;

    return 0;
}

void
nffs_path_parser_new(struct nffs_path_parser *parser, const char *path)
{
    parser->npp_token_type = NFFS_PATH_TOKEN_NONE;
    parser->npp_path = path;
    parser->npp_off = 0;
}

static int
nffs_path_find_child(struct nffs_inode_entry *parent,
                     const char *name, int name_len,
                     struct nffs_inode_entry **out_inode_entry)
{
    struct nffs_inode_entry *cur;
    struct nffs_inode inode;
    int cmp;
    int rc;

    SLIST_FOREACH(cur, &parent->nie_child_list, nie_sibling_next) {
        rc = nffs_inode_from_entry(&inode, cur);
        if (rc != 0) {
            return rc;
        }

        rc = nffs_inode_filename_cmp_ram(&inode, name, name_len, &cmp);
        if (rc != 0) {
            return rc;
        }

        if (cmp == 0) {
            *out_inode_entry = cur;
            return 0;
        }
        if (cmp > 0) {
            break;
        }
    }

    return FS_ENOENT;
}

/*
 * Return the inode and optionally it's parent associated with the input path
 * nffs_path_parser struct used to track location in hierarchy
 */
int
nffs_path_find(struct nffs_path_parser *parser,
               struct nffs_inode_entry **out_inode_entry,
               struct nffs_inode_entry **out_parent)
{
    struct nffs_inode_entry *parent;
    struct nffs_inode_entry *inode_entry;
    int rc;

    *out_inode_entry = NULL;
    if (out_parent != NULL) {
        *out_parent = NULL;
    }

    inode_entry = NULL;
    while (1) {
        parent = inode_entry;
        rc = nffs_path_parse_next(parser);
        if (rc != 0) {
            return rc;
        }

        switch (parser->npp_token_type) {
        case NFFS_PATH_TOKEN_BRANCH:
            if (parent == NULL) {
                /* First directory must be root. */
                if (parser->npp_token_len != 0) {
                    return FS_ENOENT;
                }

                inode_entry = nffs_root_dir;
            } else {
                /* Ignore empty intermediate directory names. */
                if (parser->npp_token_len == 0) {
                    break;
                }

                rc = nffs_path_find_child(parent, parser->npp_token,
                                          parser->npp_token_len, &inode_entry);
                if (rc != 0) {
                    goto done;
                }
            }
            break;
        case NFFS_PATH_TOKEN_LEAF:
            if (parent == NULL) {
                /* First token must be root directory. */
                return FS_ENOENT;
            }

            if (parser->npp_token_len == 0) {
                /* If the path ends with a slash, the leaf is the parent, not
                 * the trailing empty token.
                 */
                inode_entry = parent;
                rc = 0;
            } else {
                rc = nffs_path_find_child(parent, parser->npp_token,
                                          parser->npp_token_len, &inode_entry);
            }
            goto done;
        }
    }

done:
    *out_inode_entry = inode_entry;
    if (out_parent != NULL) {
        *out_parent = parent;
    }
    return rc;
}

int
nffs_path_find_inode_entry(const char *filename,
                           struct nffs_inode_entry **out_inode_entry)
{
    struct nffs_path_parser parser;
    int rc;

    nffs_path_parser_new(&parser, filename);
    rc = nffs_path_find(&parser, out_inode_entry, NULL);

    return rc;
}

/**
 * Unlinks the file or directory at the specified path.  If the path refers to
 * a directory, all the directory's descendants are recursively unlinked.  Any
 * open file handles refering to an unlinked file remain valid, and can be
 * read from and written to.
 *
 * @path                    The path of the file or directory to unlink.
 *
 * @return                  0 on success; nonzero on failure.
 */
int
nffs_path_unlink(const char *path)
{
    struct nffs_inode_entry *inode_entry;
    struct nffs_inode inode;
    int rc;

    rc = nffs_path_find_inode_entry(path, &inode_entry);
    if (rc != 0) {
        return rc;
    }

    rc = nffs_inode_from_entry(&inode, inode_entry);
    if (rc != 0) {
        return rc;
    }

    rc = nffs_inode_unlink(&inode);
    if (rc != 0) {
        return rc;
    }

    return 0;
}

/**
 * Performs a rename and / or move of the specified source path to the
 * specified destination.  The source path can refer to either a file or a
 * directory.  All intermediate directories in the destination path must
 * already have been created.  If the source path refers to a file, the
 * destination path must contain a full filename path (i.e., if performing a
 * move, the destination path should end with the same filename in the source
 * path).  If an object already exists at the specified destination path, this
 * function causes it to be unlinked prior to the rename (i.e., the destination
 * gets clobbered).
 *
 * @param from              The source path.
 * @param to                The destination path.
 *
 * @return                  0 on success;
 *                          nonzero on failure.
 */
int
nffs_path_rename(const char *from, const char *to)
{
    struct nffs_path_parser parser;
    struct nffs_inode_entry *from_inode_entry;
    struct nffs_inode_entry *to_inode_entry;
    struct nffs_inode_entry *from_parent;
    struct nffs_inode_entry *to_parent;
    struct nffs_inode inode;
    int rc;

    nffs_path_parser_new(&parser, from);
    rc = nffs_path_find(&parser, &from_inode_entry, &from_parent);
    if (rc != 0) {
        return rc;
    }

    nffs_path_parser_new(&parser, to);
    rc = nffs_path_find(&parser, &to_inode_entry, &to_parent);
    switch (rc) {
    case 0:
        /* The user is clobbering something with the rename. */
        if (nffs_hash_id_is_dir(from_inode_entry->nie_hash_entry.nhe_id) ^
            nffs_hash_id_is_dir(to_inode_entry->nie_hash_entry.nhe_id)) {

            /* Cannot clobber one type of file with another. */
            return FS_EINVAL;
        }

        rc = nffs_inode_from_entry(&inode, to_inode_entry);
        if (rc != 0) {
            return rc;
        }

        /*
         * Don't allow renames if the inode has been deleted
         * Side-effect is that we've restored the inode as needed.
         */
        if (nffs_inode_is_deleted(from_inode_entry)) {
            assert(0);
            return FS_ENOENT;
        }

        rc = nffs_inode_unlink(&inode);
        if (rc != 0) {
            return rc;
        }
        break;

    case FS_ENOENT:
        assert(to_parent != NULL);
        if (parser.npp_token_type != NFFS_PATH_TOKEN_LEAF) {
            /* Intermediate directory doesn't exist. */
            return FS_EINVAL;
        }
        break;

    default:
        return rc;
    }

    rc = nffs_inode_rename(from_inode_entry, to_parent, parser.npp_token);
    if (rc != 0) {
        return rc;
    }

    return 0;
}

/**
 * Creates a new directory at the specified path.
 *
 * @param path                  The path of the directory to create.
 *
 * @return                      0 on success;
 *                              FS_EEXIST if there is another file or
 *                                  directory at the specified path.
 *                              FS_ENONT if a required intermediate directory
 *                                  does not exist.
 */
int
nffs_path_new_dir(const char *path, struct nffs_inode_entry **out_inode_entry)
{
    struct nffs_path_parser parser;
    struct nffs_inode_entry *inode_entry;
    struct nffs_inode_entry *parent;
    int rc;

    nffs_path_parser_new(&parser, path);
    rc = nffs_path_find(&parser, &inode_entry, &parent);
    if (rc == 0) {
        return FS_EEXIST;
    }
    if (rc != FS_ENOENT) {
        return rc;
    }
    if (parser.npp_token_type != NFFS_PATH_TOKEN_LEAF || parent == NULL) {
        return FS_ENOENT;
    }

    rc = nffs_file_new(parent, parser.npp_token, parser.npp_token_len, 1, 
                       &inode_entry);
    if (rc != 0) {
        return rc;
    }

    if (out_inode_entry != NULL) {
        *out_inode_entry = inode_entry;
    }

    return 0;
}
