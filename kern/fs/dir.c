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
    /* 
        loop thru all the subdirectory entires of dp by calling inode_read
        find one that is allocated (inum > 0) and has matching 'name'
        write the offset of the found inode to the address poff and return the pointer 
        to the inode number in the found subdirectory entry
        
        if not found, return 0 (null pointer)
    */
    uint32_t off, inum;
    struct dirent de;

    if (dp->type != T_DIR)
        KERN_PANIC("dir_lookup not DIR");

    //TODO (ASK ABOUT THIS)
    // look thru all the subdirectory entries of dp
    for (off = 0; off < dp->size; off += sizeof(de)) {
        if (inode_read(dp, (char *) &de, off, sizeof(de)) != sizeof(de))
            KERN_PANIC("dir_lookup read illegal size");
        if (de.inum == 0) continue; // skip empty entries
        if (dir_namecmp(name, de.name) == 0) {
            // entry matches the name we are looking for
            if (poff != 0) *poff = off;
            inum = de.inum;
            return inode_get(dp->dev, inum); // returns the in-memory copy of the inode
        }
    }

    return 0;
}

// Write a new directory entry (name, inum) into the directory dp.
int dir_link(struct inode *dp, char *name, uint32_t inum)
{
    // TODO: Check that name is not present.
    struct dirent de;
    struct inode *found = dir_lookup(dp, name, 0); 
    // found is nonzero if the subdirectory already exists
    if (found != 0) {
        inode_put(found); // drop the unnecessary reference we have created from dir_lookup
        return -1;
    }
    // TODO: Look for an empty dirent.
    for (uint32_t off = 0; off < dp->size; off += sizeof(de)) {
        if (inode_read(dp, (char *) &de, off, sizeof(de)) != sizeof(de))
            KERN_PANIC("dir_link read illegal size");
        if (de.inum == 0) {
            // copy name and inum to the subdirectory structure
            strncpy(de.name, name, DIRSIZ);
            de.inum = inum;
            // write the subdirectory structure to the disk
            if (inode_write(dp, (char *) &de, off, sizeof(de)) != sizeof(de))
                KERN_PANIC("dir_link write illegal size");
            return 0;
        }
    }
    return -1;
}
