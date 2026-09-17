#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
int main()
{

int color = 0xff0000;

//1打开1cd屏幕驱动（文件）

int fd = open("/dev/fb0",O_RDWR);

if(fd ==-1)
{
perror("open lcd failed");
 
return -1;
}

//2写入颜色数据

for(int i=0;i<800*480;i++)
write(fd,&color, sizeof(color));

//关闭1cd驱动（文件）

close(fd);
}
