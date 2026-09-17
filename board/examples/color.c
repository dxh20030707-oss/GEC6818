#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

// #define LCD_PATHNAME "/dev/fb0"		// 在GEC6818板下的LCD驱动路径
 #define LCD_PATHNAME "/dev/ubuntu_lcd"	// 在ubuntu虚拟机下的LCD驱动路径

// 显示德国国旗
int main()
{
	int color1 = 0x000000;
	int color2 = 0xff0000;
	int color3 = 0xffff00;
	
	// 1 打开lcd屏幕驱动（文件）    
	int fd = open(LCD_PATHNAME, O_RDWR);
	{
	if(fd == -1)
		perror("open lcd failed");
		return -1;
	}
	
	// 2 写入颜色数据
	// for(int i=0; i<800*480; i++)
		// write(fd, &color, sizeof(color));
	
	int y;
	int x;
	for(y=0; y<160; y++)	// 只和循环次数有关，循环160行即可
	{
		for(x=0; x<800; x++)	// 0~800次，80个像素点，一行像素点的数量
		{
			write(fd, &color1, sizeof(color1));
		}	
	}
	for(y=160; y<320; y++)	// 只和循环次数有关，循环160行即可
	{
		for(x=0; x<800; x++)
		{
			write(fd, &color2, sizeof(color2));
		}	
	}
	for(y=0; y<160; y++)	// 只和循环次数有关，循环160行即可
	{
		for(x=0; x<800; x++)
		{
			write(fd, &color3, sizeof(color3));
		}	
	}
	
	// 3 关闭lcd驱动（文件）    
	close(fd);
}
