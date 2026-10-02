#include <bits/stdc++.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

int main(){
        const char* filename = "Learn.txt";
        char buffer[1000];
        int fd;
        fd = open(filename,O_RDWR | O_CREAT | O_TRUNC,0644);
        std::cout << "Opened" << std::endl;
        const char* addedtext = "File Created";
        auto result = write(fd,addedtext,12);
        if(result < 0) std::cout << "Write Failed" << std::endl;
        close(fd);
        fd = open(filename,O_RDONLY);
        read(fd,buffer,100);
        for(int i=0;i<12;i++) std::cout << buffer[i];
        close(fd);
        return 0;
}