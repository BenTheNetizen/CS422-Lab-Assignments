#ifndef _KERN_LIB_BUF_H_
#define _KERN_LIB_BUF_H_

#ifdef _KERN_

struct buf {
    int32_t flags;
    int dev;            // device number
    uint32_t sector;    // sector number
    struct buf *prev;   // LRU cache list
    struct buf *next;
    struct buf *qnext;  // disk queue
    uint8_t data[512]; // sectors are of size 512 bytes
};

#define B_BUSY  0x1  // buffer is locked by some process
#define B_VALID 0x2  // buffer has been read from disk
#define B_DIRTY 0x4  // buffer needs to be written to disk

#endif  /* _KERN_ */

#endif  /* !_KERN_LIB_BUF_H_ */
