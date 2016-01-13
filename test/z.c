#include <stdio.h>
#include <sys/stat.h>
#include <fcntl.h>

int main(int argc, char *argv[])
{
    /*mode_t m = umask(0007);*/
    open("a", O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
    return 0;
}
