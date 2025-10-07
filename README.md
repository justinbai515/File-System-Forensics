# File System Forensics

# About
This was originally a project I submitted for UIUC's CS 423 Operating System Design course. In the original project description, I was told to recover `jpg` files from a disk image utilizing the `ext2` filesystem. In this rendition of the project, I have added recovery for more file types that might be of interest in forensic analysis. 

# Using
In order to use this script to recover files from a disk image, the disk image must be one that utilizes `ext2`. To compile and run the script, run `make` in the command line, and then `./runscan [ext2 disk image name] [name of folder you want files to be output to]`.

# Entrypoint to File System

I use the `read_super_block` and `read_group_desc` helper functions to retrieve the initial information about the superblock and group structs that I need from the disk. Since we know the superblock is located at offset 1024 and group is located at block 2 (e.g offset 2048), retrieval is trivial.

# Parsing File System

Since I can get the number of inodes from the metadata in superblock, I just iterate over all inodes from 1 to the number of inodes in the file system. This allows me to simplify the program by not having to recurse and search through each directory in the root folder.
For each inode, I check if the inode is indicative of a desirable file (`jpg`, `txt`, `pdf`). If yes, then I start making the entry to the new directory. Otherwise, I continue parsing the inodes.

# Copying File System

My copy process functions by creating the new file to put the data to. 
My function `make_file_entry` then copies each 1024 data block the inode reserves, starting with the 12 direct blocks.
If the file exceeds the ~12KB limit, `make_file_entry` continues by looking at the indirect block, copying the data pointed to by each of the data blocks in the indirect block.
If there is even more data to copy, repeat the same process, but with the double indirect block.

# Differentiating Between Indirection

My indirection handler function `handle_indirect_block` is a combination of recursive and iterative approach to copying the data blocks to the output file.
If it receives a singley indirected block, it parses through the 256 data block pointers to copy data from.
If it receives a doubley indirected block, it passes each of the 256 singley indirected blocks to the `handle_indirect_block` function.
 