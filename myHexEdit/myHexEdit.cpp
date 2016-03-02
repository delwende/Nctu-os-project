#include <bits/stdc++.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <fcntl.h>

using namespace std;

int myhexeditfunc(const string &img, unsigned char *buffer, unsigned int start, unsigned int size){
    int fd = open(img.c_str(), O_WRONLY | O_NONBLOCK);
    if(fd <= 0){
        perror("open failed");
        return -1;
    }
    if(lseek(fd, start, SEEK_SET) < 0){
        perror("lseek failed");
        return -1;
    }
    if(size != write(fd, buffer, size)){
        perror("write failed");
        return -1;
    }
    close(fd);
    return 0;
}

void mysyncfunc(){
    sync();
    system("echo 3 > /proc/sys/vm/drop_caches");
}

int main(int argc, char **argv){
    if(argc < 4){
        printf("Usage: ./myHexEdit IMAGE START HEX_VALUE1 [HEX_VALUE2 ..]\n");
        return 0;
    }
    unsigned int start = atoi(argv[2]);
    unsigned int size = argc - 3;
    unsigned char *buffer = new unsigned char[size];
    for(int i=3;i<argc;i++){
        unsigned int tmp;
        sscanf(argv[i], "%d", &tmp);
        buffer[i - 3] = (unsigned char)tmp;
    }
    mysyncfunc();
    myhexeditfunc(argv[1], buffer, start, size);
    mysyncfunc();
    return 0;
}
