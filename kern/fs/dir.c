#include <kern/lib/types.h>
#include <kern/lib/debug.h>
#include <kern/lib/string.h>
#include "inode.h"
#include "dir.h"

// Directories

int dir_namecmp(const char *s, const char *t)
{
    return strncmp(s, t, DIRSIZ);
}

/**
 * Look for a directory entry in a directory.
 * If found, set *poff to byte offset of entry.
 */
struct inode *dir_lookup(struct inode *dp, char *name, uint32_t * poff)
{
    uint32_t off;
    struct dirent de;

    if (dp->type != T_DIR)
        KERN_PANIC("not a directory");

    for (off = 0; off < dp->size ; off += sizeof(de)){
        if (inode_read(dp, (char*)&de, off, sizeof(de)) != sizeof(de)){
            KERN_PANIC("dir_lookup fail");
        }
        if (de.inum != 0 && dir_namecmp(name, de.name) == 0){
            if (poff!=0) *poff = off;
            return inode_get(dp->dev, de.inum);
        }
    }

    return 0;
}

// Write a new directory entry (name, inum) into the directory dp.
int dir_link(struct inode *dp, char *name, uint32_t inum)
{
    // TODO: Check that name is not present.
    uint32_t poff;
    struct inode *ip = dir_lookup(dp, name, &poff);
    if (ip!=0){
        inode_put(ip);
        return -1;
    }

    // TODO: Look for an empty dirent.
    uint32_t off;
    struct dirent de;

    for (off = 0; off < dp->size; off += sizeof(de)){
        if (inode_read(dp, (char*)&de, off, sizeof(de)) != sizeof(de)){
            KERN_PANIC("dir_lookup fail");
        }
        if (de.inum == 0) break;
    }
    de.inum = inum;
    strncpy(de.name, name, DIRSIZ);
    if (inode_write(dp, (char*)&de, off, sizeof(de)) != sizeof(de)){
        KERN_PANIC("No unallocated subdirectory entry found");
    }
    return 0;
}
