#include "ext2_fs.h"
#include "read_ext2.h"
#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

#define KB 1024
#define MIN(a, b) (((a) < (b)) ? a : b)

/*
Returns if the buffer is characteristic of a jpg file
*/
int is_jpg(char * buffer)
{

    return (buffer[0] == (char)0xff &&
        buffer[1] == (char)0xd8 &&
        buffer[2] == (char)0xff &&
        (buffer[3] == (char)0xe0 ||
        buffer[3] == (char)0xe1 ||
        buffer[3] == (char)0xe8));
}

int is_txt(char * buffer) 
{
    return (buffer[0] == (char)0xff && 
                buffer[1] == (char)0xfe) || 
            (buffer[0] == (char)0xfe && 
                buffer[1] == (char)0xff) || 
            (buffer[0] == (char)0xef &&
                buffer[1] == (char)0xbb &&
                buffer[2] == (char)0xbf) || 
            (buffer[0] == (char)0xff && 
                buffer[1] == (char)0xfe && 
                buffer[2] == (char)0x00 && 
                buffer[3] == (char)0x00) ||
            (buffer[0] == (char)0x00 && 
                buffer[1] == (char)0x00 && 
                buffer[2] == (char)0xfe && 
                buffer[3] == (char)0xff) ||
            (buffer[0] == (char)0x0e && 
            buffer[1] == (char)0xfe && 
            buffer[2] == (char)0xff);

}
int is_pdf(char * buffer) {
    return (buffer[0] == (char)0x25 &&
                buffer[1] == (char)0x50 &&
                buffer[2] == (char)0x44 &&
                buffer[3] == (char)0x46 &&
                buffer[4] == (char)0x2d);
}

size_t copy_block(int src_fd, int dst_fd, uint_least32_t block, size_t bytes_to_copy) {
    char buff[KB];
    lseek(src_fd, block*KB, SEEK_SET);
    size_t bytes_read = (size_t)read(src_fd, buff, bytes_to_copy);
    write(dst_fd, buff, bytes_read);
    return bytes_read;
}

void handle_indirect_block(int src_fd, int dst_fd, int n_indirection, uint_least32_t block_addr, uint_least32_t * bytes_left) 
{
    uint_least32_t b_addr, block_start;
    block_start = block_addr * KB;
    if (n_indirection == 1) 
    {
        for (int offset = 0; offset < 256 && *bytes_left > 0; offset++) 
        {
            lseek(src_fd, block_start + offset*4, SEEK_SET);
            read(src_fd, &b_addr, sizeof(uint_least32_t));
            
            *bytes_left -= copy_block(src_fd, dst_fd, b_addr, MIN(KB, *bytes_left));
        }
    } else 
    {
        // stores a block (1024 Bytes) of 256 pointers to 256 indirect blocks each
        for (int i = 0; i < 256 && *bytes_left > 0; i++) 
        {
            lseek(src_fd, block_start + i*4, SEEK_SET);
            read(src_fd, &b_addr, sizeof(uint_least32_t));
            handle_indirect_block(src_fd, dst_fd, n_indirection-1, b_addr, bytes_left);
        }
    }
}

void make_file_entry(int fd, char * dir_name, struct ext2_inode node, int num, char * file_type) 
{
    char entry[256];
    uint_least32_t * node_i_block = node.i_block;
    sprintf(entry, "%s/file-%d.%s", dir_name, num, file_type);
    int new_fd = open(entry, O_WRONLY | O_CREAT);
    uint_least32_t bytes_to_write = node.i_size;
    
    for (int i = 0; i < 12 && bytes_to_write > 0; i++) 
    {
        bytes_to_write -= copy_block(fd, new_fd, node_i_block[i], MIN(KB, bytes_to_write));
    }

    if (bytes_to_write > 0) 
    {
        handle_indirect_block(fd, new_fd, 1, node_i_block[12], &bytes_to_write);
    }

    if (bytes_to_write > 0) 
    {
        handle_indirect_block(fd, new_fd, 2, node_i_block[13], &bytes_to_write);
    }
    
    close(new_fd);
}

int main(int argc, char **argv) 
{
    if (argc != 3) 
    {
        printf("expected usage: ./runscan inputfile outputfile\n");
        exit(0);
    }


    int fd;
    fd = open(argv[1], O_RDONLY);    /* open disk image */
    DIR * dir = opendir(argv[2]);
    if (dir != NULL) 
    {
        closedir(dir);
        printf("Directory %s already exists\n", argv[2]);
        close(fd);
        return 1;
    }
    mkdir(argv[2], 0700);
    ext2_read_init(fd);

    struct ext2_super_block super;
    struct ext2_group_desc group;
    struct ext2_inode inode;

    // example read first the super-block and group-descriptor
    read_super_block(fd, &super);
    read_group_desc(fd, &group);
    for (uint_least32_t i = 1; i < super.s_inodes_count; i++) // traverse through all files and directories from root
    {  
        char buff[KB];
        // get inode
        read_inode(fd, group.bg_inode_table, i, &inode);

        // sample first block of inode
        lseek(fd, inode.i_block[0] * block_size, SEEK_SET); 
        read(fd, buff, KB);

        // check if inode represents jpg
        if (S_ISREG(inode.i_mode)) 
        {
            if (is_jpg(buff)) {
                make_file_entry(fd, argv[2], inode, i, "jpg");
            } else if (is_txt(buff)) {
                make_file_entry(fd, argv[2], inode, i, "txt");
            } else if (is_pdf(buff)) {
                make_file_entry(fd, argv[2], inode, i, "pdf");
            }
        }
    }


    close(fd);
    return 0;
}