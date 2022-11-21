// File-system system calls.

#include <kern/lib/types.h>
#include <kern/lib/debug.h>
#include <kern/lib/pmap.h>
#include <kern/lib/string.h>
#include <kern/lib/trap.h>
#include <kern/lib/syscall.h>
#include <kern/lib/spinlock.h>
#include <kern/thread/PTCBIntro/export.h>
#include <kern/thread/PCurID/export.h>
#include <kern/trap/TSyscallArg/export.h>

#include "dir.h"
#include "path.h"
#include "file.h"
#include "fcntl.h"
#include "log.h"

/**
 * This function is not a system call handler, but an auxiliary function
 * used by sys_open.
 * Allocate a file descriptor for the given file.
 * You should scan the list of open files for the current thread
 * and find the first file descriptor that is available.
 * Return the found descriptor or -1 if none of them is free.
 */
static int fdalloc(struct file *f)
{
    unsigned int pid = get_curid();
    struct file **files = tcb_get_openfiles(pid);
    for (int idx = 0; idx < NOFILE; idx++){
        if (files[idx]==0) {
            tcb_set_openfiles(pid, idx, f);
            return idx;
        }
    }
    return -1;
}

char kernel_buf[10000];
static spinlock_t buf_lk;

void kernel_buf_init(void)
{
    spinlock_init(&buf_lk);
}

/**
 * From the file indexed by the given file descriptor, read n bytes and save them
 * into the buffer in the user. As explained in the assignment specification,
 * you should first write to a kernel buffer then copy the data into user buffer
 * with pt_copyout.
 * Return Value: Upon successful completion, read() shall return a non-negative
 * integer indicating the number of bytes actually read. Otherwise, the
 * functions shall return -1 and set errno E_BADF to indicate the error.
 */
void sys_read(tf_t *tf)
{
    unsigned int pid = get_curid();

    spinlock_acquire(&buf_lk);
    int fd = (int)syscall_get_arg2(tf);
    unsigned int user_buf = syscall_get_arg3(tf);
    unsigned int n = syscall_get_arg4(tf);

    if (fd < 0 || fd > NOFILE || n < 0){
        KERN_INFO("invalid fd\n");
        syscall_set_errno(tf, E_BADF);
        syscall_set_retval1(tf, -1);
        spinlock_release(&buf_lk);
        return;
    }
    if (n > sizeof(kernel_buf) || user_buf < VM_USERLO || user_buf + n > VM_USERHI){
        KERN_INFO("illegal user buffer address range\n");
        // KERN_INFO("user_buf = %u, n = %u, VM_USERLO = %u, VM_USERHI = %u\n", user_buf, n, VM_USERLO, VM_USERHI);
        syscall_set_errno(tf, E_INVAL_ADDR);
        syscall_set_retval1(tf, -1);
        spinlock_release(&buf_lk);
        return;
    }

    struct file *open_file = tcb_get_openfiles(pid)[fd];

    if (open_file == 0 || open_file->ip == 0){
        KERN_INFO("invalid file struct\n");
        syscall_set_retval1(tf, -1);
        syscall_set_errno(tf, E_BADF);
        spinlock_release(&buf_lk);
        return;
    } 
    int read_size = file_read(open_file, kernel_buf, n);
    if (read_size > 0) pt_copyout(kernel_buf, pid, user_buf, read_size); 
    syscall_set_retval1(tf, read_size);
    syscall_set_errno(tf, E_SUCC);
    spinlock_release(&buf_lk);
    return;
}

/**
 * Write n bytes of data in the user's buffer into the file indexed by the file descriptor.
 * You should first copy the data info an in-kernel buffer with pt_copyin and then
 * pass this buffer to appropriate file manipulation function.
 * Upon successful completion, write() shall return the number of bytes actually
 * written to the file associated with f. This number shall never be greater
 * than nbyte. Otherwise, -1 shall be returned and errno E_BADF set to indicate the
 * error.
 */
void sys_write(tf_t *tf)
{
    unsigned int pid = get_curid();

    spinlock_acquire(&buf_lk);
    int fd = (int)syscall_get_arg2(tf);
    unsigned int user_buf = syscall_get_arg3(tf);
    unsigned int n = syscall_get_arg4(tf);

    if (fd < 0 || fd > NOFILE || n < 0){
        KERN_INFO("invalid fd\n");
        syscall_set_errno(tf, E_BADF);
        syscall_set_retval1(tf, -1);
        spinlock_release(&buf_lk);
        return;
    }

    if (n > sizeof(kernel_buf) || user_buf < VM_USERLO || user_buf + n > VM_USERHI){
        KERN_INFO("illegal user buffer address range\n");
        // KERN_INFO("user_buf = %u, n = %u, VM_USERLO = %u, VM_USERHI = %u\n", user_buf, n, VM_USERLO, VM_USERHI);
        syscall_set_errno(tf, E_INVAL_ADDR);
        syscall_set_retval1(tf, -1);
        spinlock_release(&buf_lk);
        return;
    }

    struct file *open_file = tcb_get_openfiles(pid)[fd];
    if (open_file == 0 || open_file->ip == 0){
        KERN_INFO("invalid file struct\n");
        syscall_set_retval1(tf, -1);
        syscall_set_errno(tf, E_BADF);
        spinlock_release(&buf_lk);
        return;
    } 

    pt_copyin(pid, user_buf, kernel_buf, n);
    int write_size = file_write(open_file, kernel_buf, n);
    syscall_set_retval1(tf, write_size);
    syscall_set_errno(tf, E_SUCC);
    spinlock_release(&buf_lk);
    return;
}

/**
 * Return Value: Upon successful completion, 0 shall be returned; otherwise, -1
 * shall be returned and errno E_BADF set to indicate the error.
 */
void sys_close(tf_t *tf)
{
    unsigned int pid = get_curid();

    int fd = (int)syscall_get_arg2(tf);
    if (fd < 0 || fd > NOFILE){
        KERN_INFO("invalid fd\n");
        syscall_set_errno(tf, E_BADF);
        syscall_set_retval1(tf, -1);
        return;
    }

    struct file *open_file = tcb_get_openfiles(pid)[fd];
    if (open_file == 0){
        KERN_INFO("invalid file struct\n");
        syscall_set_retval1(tf, -1);
        syscall_set_errno(tf, E_BADF);
        return;
    }

    file_close(open_file);
    tcb_set_openfiles(pid, fd, 0);
    syscall_set_retval1(tf, 0);
    syscall_set_errno(tf, E_SUCC);
    return;
}

/**
 * Return Value: Upon successful completion, 0 shall be returned. Otherwise, -1
 * shall be returned and errno E_BADF set to indicate the error.
 */
void sys_fstat(tf_t *tf)
{
    unsigned int pid = get_curid();

    int fd = (int)syscall_get_arg2(tf);
    struct file_stat *st = (struct file_stat *)syscall_get_arg3(tf);

    if (fd < 0 || fd > NOFILE){
        KERN_INFO("invalid fd\n");
        syscall_set_errno(tf, E_BADF);
        syscall_set_retval1(tf, -1);
        return;
    }

    if ((unsigned int)st < VM_USERLO || (unsigned int)st + sizeof(struct file_stat) > VM_USERHI){
        KERN_INFO("illegal user pointer address range\n");
        // KERN_INFO("user_buf = %u, n = %u, VM_USERLO = %u, VM_USERHI = %u\n", user_buf, n, VM_USERLO, VM_USERHI);
        syscall_set_errno(tf, E_INVAL_ADDR);
        syscall_set_retval1(tf, -1);
        return;
    }

    struct file *open_file = tcb_get_openfiles(pid)[fd];
    if (open_file == 0){
        KERN_INFO("invalid file struct\n");
        syscall_set_retval1(tf, -1);
        syscall_set_errno(tf, E_BADF);
        return;
    }

    struct file_stat kernel_stat;
    pt_copyin(pid, (unsigned int)st, &kernel_stat, sizeof(struct file_stat));
    int res = file_stat(open_file, &kernel_stat);
    if (res==0) syscall_set_errno(tf, E_SUCC);
    else syscall_set_errno(tf, E_BADF);
    syscall_set_retval1(tf, res);
    return;
}

/**
 * Create the path new as a link to the same inode as old.
 */
void sys_link(tf_t * tf)
{
    char name[DIRSIZ], new[128], old[128];
    struct inode *dp, *ip;
    unsigned int old_len = syscall_get_arg4(tf);
    unsigned int new_len = syscall_get_arg5(tf);
    if (old_len >= 128) old_len = 127;
    if (new_len >= 128) new_len = 127;

    pt_copyin(get_curid(), syscall_get_arg2(tf), old, old_len);
    pt_copyin(get_curid(), syscall_get_arg3(tf), new, new_len);

    old[old_len] = '\0';
    new[new_len] = '\0';

    if ((ip = namei(old)) == 0) {
        syscall_set_errno(tf, E_NEXIST);
        return;
    }

    begin_trans();

    inode_lock(ip);
    if (ip->type == T_DIR) {
        inode_unlockput(ip);
        commit_trans();
        syscall_set_errno(tf, E_DISK_OP);
        return;
    }

    ip->nlink++;
    inode_update(ip);
    inode_unlock(ip);

    if ((dp = nameiparent(new, name)) == 0)
        goto bad;
    inode_lock(dp);
    if (dp->dev != ip->dev || dir_link(dp, name, ip->inum) < 0) {
        inode_unlockput(dp);
        goto bad;
    }
    inode_unlockput(dp);
    inode_put(ip);

    commit_trans();

    syscall_set_errno(tf, E_SUCC);
    return;

bad:
    inode_lock(ip);
    ip->nlink--;
    inode_update(ip);
    inode_unlockput(ip);
    commit_trans();
    syscall_set_errno(tf, E_DISK_OP);
    return;
}

/**
 * Is the directory dp empty except for "." and ".." ?
 */
static int isdirempty(struct inode *dp)
{
    int off;
    struct dirent de;

    for (off = 2 * sizeof(de); off < dp->size; off += sizeof(de)) {
        if (inode_read(dp, (char *) &de, off, sizeof(de)) != sizeof(de))
            KERN_PANIC("isdirempty: readi");
        if (de.inum != 0)
            return 0;
    }
    return 1;
}

void sys_unlink(tf_t *tf)
{
    struct inode *ip, *dp;
    struct dirent de;
    char name[DIRSIZ], path[128];
    uint32_t off;
    unsigned int path_len = syscall_get_arg3(tf);
    if (path_len >= 128) path_len = 127;

    pt_copyin(get_curid(), syscall_get_arg2(tf), path, path_len);
    path[path_len] = '\0';

    if ((dp = nameiparent(path, name)) == 0) {
        syscall_set_errno(tf, E_DISK_OP);
        return;
    }

    begin_trans();

    inode_lock(dp);

    // Cannot unlink "." or "..".
    if (dir_namecmp(name, ".") == 0 || dir_namecmp(name, "..") == 0)
        goto bad;

    if ((ip = dir_lookup(dp, name, &off)) == 0)
        goto bad;
    inode_lock(ip);

    if (ip->nlink < 1)
        KERN_PANIC("unlink: nlink < 1");
    if (ip->type == T_DIR && !isdirempty(ip)) {
        inode_unlockput(ip);
        goto bad;
    }

    memset(&de, 0, sizeof(de));
    if (inode_write(dp, (char *) &de, off, sizeof(de)) != sizeof(de))
        KERN_PANIC("unlink: writei");
    if (ip->type == T_DIR) {
        dp->nlink--;
        inode_update(dp);
    }
    inode_unlockput(dp);

    ip->nlink--;
    inode_update(ip);
    inode_unlockput(ip);

    commit_trans();

    syscall_set_errno(tf, E_SUCC);
    return;

bad:
    inode_unlockput(dp);
    commit_trans();
    syscall_set_errno(tf, E_DISK_OP);
    return;
}

static struct inode *create(char *path, short type, short major, short minor)
{
    uint32_t off;
    struct inode *ip, *dp;
    char name[DIRSIZ];

    if ((dp = nameiparent(path, name)) == 0)
        return 0;
    inode_lock(dp);

    if ((ip = dir_lookup(dp, name, &off)) != 0) {
        inode_unlockput(dp);
        inode_lock(ip);
        if (type == T_FILE && ip->type == T_FILE)
            return ip;
        inode_unlockput(ip);
        return 0;
    }

    if ((ip = inode_alloc(dp->dev, type)) == 0)
        KERN_PANIC("create: ialloc");

    inode_lock(ip);
    ip->major = major;
    ip->minor = minor;
    ip->nlink = 1;
    inode_update(ip);

    if (type == T_DIR) {  // Create . and .. entries.
        dp->nlink++;      // for ".."
        inode_update(dp);
        // No ip->nlink++ for ".": avoid cyclic ref count.
        if (dir_link(ip, ".", ip->inum) < 0
            || dir_link(ip, "..", dp->inum) < 0)
            KERN_PANIC("create dots");
    }

    if (dir_link(dp, name, ip->inum) < 0)
        KERN_PANIC("create: dir_link");

    inode_unlockput(dp);
    return ip;
}

void sys_open(tf_t *tf)
{
    char path[128];
    int fd, omode;
    struct file *f;
    struct inode *ip;
    unsigned int path_len = syscall_get_arg4(tf);
    if (path_len >= 128) path_len = 127;

    pt_copyin(get_curid(), syscall_get_arg2(tf), path, path_len);
    path[path_len] = '\0';
    omode = syscall_get_arg3(tf);

    if (omode & O_CREATE) {
        begin_trans();
        ip = create(path, T_FILE, 0, 0);
        commit_trans();
        if (ip == 0) {
            syscall_set_retval1(tf, -1);
            syscall_set_errno(tf, E_CREATE);
            return;
        }
    } else {
        if ((ip = namei(path)) == 0) {
            syscall_set_retval1(tf, -1);
            syscall_set_errno(tf, E_NEXIST);
            return;
        }
        inode_lock(ip);
        if (ip->type == T_DIR && omode != O_RDONLY) {
            inode_unlockput(ip);
            syscall_set_retval1(tf, -1);
            syscall_set_errno(tf, E_DISK_OP);
            return;
        }
    }

    if ((f = file_alloc()) == 0 || (fd = fdalloc(f)) < 0) {
        if (f)
            file_close(f);
        inode_unlockput(ip);
        syscall_set_retval1(tf, -1);
        syscall_set_errno(tf, E_DISK_OP);
        return;
    }
    inode_unlock(ip);

    f->type = FD_INODE;
    f->ip = ip;
    f->off = 0;
    f->readable = !(omode & O_WRONLY);
    f->writable = (omode & O_WRONLY) || (omode & O_RDWR);
    syscall_set_retval1(tf, fd);
    syscall_set_errno(tf, E_SUCC);
}

void sys_mkdir(tf_t *tf)
{
    char path[128];
    struct inode *ip;
    unsigned int path_len = syscall_get_arg3(tf);
    if (path_len >= 128) path_len = 127;

    pt_copyin(get_curid(), syscall_get_arg2(tf), path, path_len);
    path[path_len] = '\0';

    begin_trans();
    if ((ip = (struct inode *) create(path, T_DIR, 0, 0)) == 0) {
        commit_trans();
        syscall_set_errno(tf, E_DISK_OP);
        return;
    }
    inode_unlockput(ip);
    commit_trans();
    syscall_set_errno(tf, E_SUCC);
}

void sys_chdir(tf_t *tf)
{
    char path[128];
    struct inode *ip;
    int pid = get_curid();
    unsigned int path_len = syscall_get_arg3(tf);
    if (path_len >= 128) path_len = 127;

    pt_copyin(get_curid(), syscall_get_arg2(tf), path, path_len);
    path[path_len] ='\0';

    if ((ip = namei(path)) == 0) {
        syscall_set_errno(tf, E_DISK_OP);
        return;
    }
    inode_lock(ip);
    if (ip->type != T_DIR) {
        inode_unlockput(ip);
        syscall_set_errno(tf, E_DISK_OP);
        return;
    }
    inode_unlock(ip);
    inode_put(tcb_get_cwd(pid));
    tcb_set_cwd(pid, ip);
    syscall_set_errno(tf, E_SUCC);
}

void sys_pwd(tf_t *tf)
{
    int pid = get_curid();
    dprintf("%d\n", pid);
    struct inode *ip = tcb_get_cwd(pid);
    dprintf("%d, %d, %d\n", ip, ip->inum, ip->size);
    // uint32_t off;

    char *buf = (char*)syscall_get_arg2(tf);
    unsigned int buf_len = syscall_get_arg3(tf);
    
    struct dirent de;
    if (inode_read(ip, (char*)&de, 0, sizeof(de)) != sizeof(de)){
        KERN_PANIC("dir_lookup fail");
    }
    dprintf("%s\n", de.name);

    pt_copyin(pid, de.name, buf, strlen(de.name));
    syscall_set_errno(tf, E_SUCC);
}

void sys_ls(tf_t *tf)
{
    spinlock_acquire(&buf_lk);

    unsigned int buf = syscall_get_arg2(tf);
    unsigned int buf_len = syscall_get_arg3(tf);
    char path[128];
    int pid = get_curid();
    struct inode *ip;
    unsigned int path_len = syscall_get_arg6(tf);
    
    if (path_len > 0) {
        pt_copyin(pid, syscall_get_arg4(tf), path, path_len);
        if ((ip = namei(path)) == 0) {
            syscall_set_errno(tf, E_NEXIST);
        return;
        }
    } else {
        ip = tcb_get_cwd(pid);
    }

    int idx = 0;
    int len = 0;
    uint32_t off;
    struct dirent de;

    for (off = 0; off < ip->size; off+=sizeof(de)){
        if (inode_read(ip, (char*)&de, off, sizeof(de)) != sizeof(de)){
            KERN_PANIC("dir_lookup fail");
        }
        if (de.inum != 0 && idx+strlen(de.name)<10000){
            strncpy(kernel_buf+idx, de.name, strlen(de.name));
            idx += strlen(de.name);
            kernel_buf[idx++] = '\t';
            len += 1;
        }

    }

    pt_copyout(kernel_buf, pid, buf, idx);
    memset(kernel_buf, 0, 10000);
    syscall_set_errno(tf, E_SUCC);
    syscall_set_retval1(tf, len);
    spinlock_release(&buf_lk);
    return;

}

void sys_touch(tf_t *tf)
{
    char path[128];
    struct inode *ip;
    unsigned int path_len = syscall_get_arg3(tf);
    if (path_len >= 128) path_len = 127;

    pt_copyin(get_curid(), syscall_get_arg2(tf), path, path_len);
    path[path_len] = '\0';

    begin_trans();
    if ((ip = (struct inode *) create(path, T_FILE, 0, 0)) == 0) {
        commit_trans();
        syscall_set_errno(tf, E_DISK_OP);
        return;
    }
    inode_unlockput(ip);
    commit_trans();
    syscall_set_errno(tf, E_SUCC);
}

void sys_is_dir(tf_t *tf) {
    struct file *fp;
    int type;
    unsigned int curid = get_curid();
    int fd = syscall_get_arg2(tf);
    fp = tcb_get_openfiles(curid)[fd];
    if (!fp || !fp->ip) {
        KERN_INFO("sys_is_dir: fp or fp->ip is NULL");
        syscall_set_errno(tf, E_BADF);
        syscall_set_retval1(tf, -1);
        return;
    }
    type = fp->ip->type;
    syscall_set_errno(tf, E_SUCC);
    syscall_set_retval1(tf, (type == T_DIR));
}