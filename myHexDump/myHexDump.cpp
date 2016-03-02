#include <bits/stdc++.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <fcntl.h>
using namespace std;
void hexdump(const unsigned char *buf, unsigned int start, unsigned int end){
    unsigned int size = end - start + 1;
    printf("address\t");
    for(int i=0;i<16;i++)
        printf("%2X ", i);
    printf(" ");
    for(int i=0;i<16;i++)
        printf("%X", i);
    puts("");
    for(unsigned int i=0;i<size;i+=0x10){
        printf("%07x", start);
        for(int j=0;j<16;j++)
            printf(" %02x", buf[i + j]);
        printf(" |");
        for(int j=0;j<16;j++)
            if(0x20 <= buf[i + j] && buf[i + j] <= 0x7E)
                printf("%c", (char)buf[i + j]);
            else
                printf(".");
        printf("|");
        start += 16;
        puts("");
    }
}
int myhexdumpfunc(const string &img, unsigned int start, unsigned int size){
    start = start & ~0xF;
    unsigned int end = (start + size) | 0xF;
    size = end - start + 1;
    unsigned char *buffer;
    int fd = open(img.c_str(), O_RDONLY | O_NONBLOCK);
    if(fd <= 0){
        perror("open failed");
        return -1;
    }
    buffer = new unsigned char [size];
    if(lseek(fd, start, SEEK_SET) < 0){
        perror("lseek failed");
        return -1;
    }
    read(fd, buffer, size);
    hexdump(buffer, start, end);
    return 0;
}

void mysyncfunction(){
    sync();
    system("echo 3 > /proc/sys/vm/drop_caches");
}

int main(int argc, char **argv){
    if(argc < 4){
        printf("Usage: ./myHexDump IMAGE START OFFSET\n");
        return 0;
    }
    mysyncfunction();
    myhexdumpfunc(argv[1], atoi(argv[2]), atoi(argv[3]));

    return 0;
}
