#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>

// #define LCD_PATHNAME "/dev/fb0"		// 在GEC6818板下的LCD驱动路径
#define LCD_PATHNAME "/dev/ubuntu_lcd"	// 在ubuntu虚拟机下的LCD驱动路径

int fd_lcd = -1;
int *p_lcd = NULL;

int Lcd_Init()
{
	// 1 打开lcd屏幕驱动（文件） 
	fd_lcd = open(LCD_PATHNAME, O_RDWR);
	if(fd_lcd == -1)
	{
		perror("open lcd failed");
		return -1;
	}
	
	// 2 映射lcd屏幕(映射lcd屏幕每个像素点地址，相当于给每个像素点确定编号)
	
	p_lcd = mmap(NULL, 800*480*4, PROT_READ|PROT_WRITE, MAP_SHARED, fd_lcd, 0);
	if(p_lcd == NULL)
	{
		perror("mmap lcd failed");
		return -1;
	}
}

void Show_Sreen_black()
{
	for(int y=0; y<480; y++)	
	{
		for(int x=0; x<800; x++)
		{
			*(p_lcd+x+y*800) = 0x00;
		}	
	}
}

void Lcd_UnInit()
{
	// 3 关闭lcd驱动（文件）    
	close(fd_lcd);
	munmap(p_lcd, 800*480*4);
}

int main()
{
	// 1 初始化lcd屏幕
	Lcd_Init();
	
	// 2.1 像素点映射刷颜色
	Show_Sreen_black();
	
	// 3 解除lcd初始化
	Lcd_UnInit();
	
	return 0;
}