#include<stdio.h>
#include<sys/stat.h>
#include<sys/types.h>
#include<fcntl.h>
#include<stdlib.h>
#include<sys/ioctl.h>
#include<linux/fs.h>
#include<sys/uio.h>

#define BLOCK_SZ 4096

off_t get_file_size(int file_fd) {
    struct stat sb;
    if (fstat(file_fd, &sb) == -1){
        perror("error while fstat");
        return -1;
    }
    if(S_ISREG(sb.st_mode)){
        return sb.st_size;
    } else if (S_ISBLK(sb.st_mode)) {
        unsigned long long bytes;
        if(ioctl(file_fd, BLKGETSIZE64, &bytes) == -1){
            perror("error while ioctl");
            return -1;
        }
        return bytes;
    }
    return -1;
}

void output_to_console(char *buf, size_t len){
    fwrite(buf, len, 1, stdout);
}


int read_and_print_file_content(char *filename){
    struct iovec *iovecs;
    int file_fd = open(filename, O_RDONLY);
    if (file_fd < 0) {
        perror("error while open");
        return 1;
    }
    off_t file_size = get_file_size(file_fd);
    off_t bytes_remaining = file_size;
    unsigned long blocks = (unsigned long) file_size / BLOCK_SZ;
    if(file_size % BLOCK_SZ) blocks++;
    iovecs = (struct iovec *) malloc(sizeof(struct iovec) * blocks);

    int current_block = 0;

    while(bytes_remaining) {
        off_t bytes_to_read = bytes_remaining;
        if (bytes_to_read > BLOCK_SZ) {
            bytes_to_read = BLOCK_SZ;
        }

        void *buf;

        if(posix_memalign(&buf, BLOCK_SZ, BLOCK_SZ)){
            perror("error while posix_memalign");
            return 1;
        }

        iovecs[current_block].iov_base = buf;
        iovecs[current_block].iov_len = bytes_to_read;
        current_block++;
        bytes_remaining -= bytes_to_read;
    }

    int ret = readv(file_fd, iovecs, blocks);
    if(ret < 0) {
        perror("error while readv");
        return 1;
    }

    for(int i=0;i<blocks;i++){
        output_to_console((char *) iovecs[i].iov_base, iovecs[i].iov_len);
    }

    free(iovecs);
    return 0;
}

int main(int argc, char **argv) {
    if (argc < 2){
        fprintf(stderr, "Usage: %s filename1 filename2 ..\n", argv[0]);
        return 1;
    }
    for(int i=1;i<argc;i++){
        if(read_and_print_file_content(argv[i])){
            fprintf(stderr, "Error reading file\n");
            return 1;
        }
    }
    return 0;
}
