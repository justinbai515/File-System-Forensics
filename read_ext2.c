#include <stdio.h>
#include "read_ext2.h"

/* implementations credit to
 * Smith College
 * http://www.science.smith.edu/~nhowe/Teaching/csc262/oldlabs/ext2.html
 */

unsigned int block_size = 1024;         /* default 1kB block size */
unsigned int inodes_per_block = 0;                      /* number of inodes per block */
unsigned int itable_blocks = 0;                         /* size in blocks of the inode table */
unsigned int blocks_per_group = 0;
unsigned int num_groups = 0;
unsigned int inodes_per_group = 0;


int debug = 1;          //turn on/off debug prints

static struct ext2_super_block r_super_block;

/* read the first super block to initialize common variables */
void ext2_read_init(int fd)
{
     lseek(fd, block_size, SEEK_SET);
     read(fd, &r_super_block, sizeof(struct ext2_super_block));
     blocks_per_group = r_super_block.s_blocks_per_group;
     inodes_per_group = r_super_block.s_inodes_per_group;
     inodes_per_block = inodes_per_group/blocks_per_group;
     num_groups = r_super_block.s_blocks_count/blocks_per_group;
}

/* read the first super block; for this project, you will only deal with the first block group */
int read_super_block( int                      fd,        /* the disk image file descriptor */
                      struct ext2_super_block *super      /* where to put the super block */
                      )
{
     lseek(fd, block_size, SEEK_SET);
     read(fd, super, sizeof(struct ext2_super_block));
     return 0;
}

/* Read the first group-descriptor in the first block group; you will not be tested with a disk image with more than one block group */
void read_group_desc( int                      fd,        /* the disk image file descriptor */
                      struct ext2_group_desc  *group      /* where to put the group-descriptor */
                      )
{
     lseek(fd, 2 * block_size, SEEK_SET);
     
     read(fd, group, sizeof(struct ext2_group_desc));
}

/* calculate the start address of the inode table in the first group */
// off_t locate_inode_table(const struct ext2_group_desc *group      /* the first group-descriptor */
//                                 )
// {
//      return group->bg_inode_table;
// }

// /* calculate the start address of the data blocks in the first group */
// off_t locate_data_blocks(const struct ext2_group_desc *group      /* the first group-descriptor */
//                                 )
// {
// }

void read_inode(fd, offset, inode_no, inode)
     int                            fd;        /* the floppy disk file descriptor */
     off_t 			   		offset;    /* offset to the start of the inode table */
     int                            inode_no;  /* the inode number to read  */
     struct ext2_inode             *inode;     /* where to put the inode */
{
     int index = (inode_no - 1)%inodes_per_group;
     lseek(fd, offset*block_size + index * sizeof(struct ext2_inode), SEEK_SET);
     read(fd, inode, sizeof(struct ext2_inode));
}
