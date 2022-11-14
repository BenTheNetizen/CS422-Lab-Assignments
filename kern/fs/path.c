// File system implementation.  Five layers:
//   + Blocks: allocator for raw disk blocks.
//   + Log: crash recovery for multi-step updates.
//   + Files: inode allocator, reading, writing, metadata.
//   + Directories: inode with special contents (list of other inodes!)
//   + Names: paths like /usr/fs.c for convenient naming.
//
// This file contains the low-level file system manipulation
// routines.  The (higher-level) system call implementations
// are in sysfile.c.

#include <kern/lib/types.h>
#include <kern/lib/debug.h>
#include <kern/lib/spinlock.h>
#include <thread/PTCBIntro/export.h>
#include <thread/PCurID/export.h>
#include "inode.h"
#include "dir.h"
#include "log.h"

#define STATE_SLASH 0
#define STATE_NAME 1 
#define STATE_END 2

// Paths

/**
 * Copy the next path element from path into name.
 * If the length of name is larger than or equal to DIRSIZ, then only
 * (DIRSIZ - 1) # characters should be copied into name.
 * This is because you need to save '\0' in the end.
 * You should still skip the entire string in this case.
 * Return a pointer to the element following the copied one.
 * The returned path has no leading slashes,
 * so the caller can check *path == '\0' to see if the name is the last one.
 * If no name to remove, return 0.
 *
 * Examples :
 *   skipelem("a/bb/c", name) = "bb/c", setting name = "a"
 *   skipelem("///a//bb", name) = "bb", setting name = "a"
 *   skipelem("a", name) = "", setting name = "a"
 *   skipelem("", name) = skipelem("////", name) = 0
 */
static char *skipelem(char *path, char *name)
{
    // TODO
    unsigned int size = 0;
    char *curr_name = name;
    unsigned int state = STATE_SLASH;

    while (1) {
        switch (state) {
            case STATE_SLASH:
                if (*path == '/') {
                    path++;
                } else if (*path == '\0') {
                    // no name to remove, return 0
                    return 0;
                } else {
                    // encountered a character other than '/', append it to curr_name
                    *curr_name = *path;
                    path++;
                    curr_name++;
                    size++;
                    state = STATE_NAME;
                }
                break;
            case STATE_NAME:
                if (*path == '/') {
                    // path element ends
                    *curr_name = '\0';
                    state = STATE_END;
                } else if (*path == '\0') {
                    *curr_name = '\0';
                    return "";
                } else {
                    if (size < 14 - 1) {
                        *curr_name = *path;
                        curr_name++;
                        size++;
                    }
                    path++;
                }
                break;
            case STATE_END:
                // we have already copied the path element into name, need to return the path
                if (*path == '/') {
                    path++;
                } else if (*path == '\0') {
                    return "";
                } else {
                    return path;
                }
                break;
        }
        
    }

    return 0;
}

/**
 * Look up and return the inode for a path name.
 * If nameiparent is true, return the inode for the parent and copy the final
 * path element into name, which must have room for DIRSIZ bytes.
 * Returns 0 in the case of error.
 */
static struct inode *namex(char *path, bool nameiparent, char *name)
{
    struct inode *ip;

    // If path is a full path, get the pointer to the root inode. Otherwise get
    // the inode corresponding to the current working directory.
    if (*path == '/') {
        ip = inode_get(ROOTDEV, ROOTINO);
    } else {
        ip = inode_dup((struct inode *) tcb_get_cwd(get_curid()));
    }

    while ((path = skipelem(path, name)) != 0) {
        // TODO
    }
    if (nameiparent) {
        inode_put(ip);
        return 0;
    }
    return ip;
}

/**
 * Return the inode corresponding to path.
 */
struct inode *namei(char *path)
{
    char name[DIRSIZ];
    return namex(path, FALSE, name);
}

/**
 * Return the inode corresponding to path's parent directory and copy the final
 * element into name.
 */
struct inode *nameiparent(char *path, char *name)
{
    return namex(path, TRUE, name);
}
