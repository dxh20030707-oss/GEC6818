#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>


/* ---------------------- 全局变量 ---------------------- */
int *p_lcd = NULL; 
int fd_lcd = -1;

/* ----------------- 函数定义(使用说明) ----------------- */
int Lcd_init();
void Nu_init();
int Show_bmp(char *pathname, int x_strat, int y_start);
void Draw_point(int x, int y, int color); 


/* ---------------------- 函数源码 ---------------------- */
int main(){
    Lcd_init();
    
    Show_bmp("Picture.bmp", 0, 0);
        
    Nu_init();
    
    return 0;
}

// 上颜色(图片颜色数据翻转)
void Draw_point(int x, int y, int color){
    *(p_lcd+x+(479-y)*800) = color;
}

// 显示图片
int Show_bmp(char *pathname, int x_strat, int y_start){
    int x, y;
    unsigned char r, g, b;
    int color;
    int height, width;
    int i = 0;
    int fd_bmp = -1;
    
    // 1 打开图片文件
    fd_bmp = open(pathname, O_RDONLY);
    if(fd_lcd == -1){
        perror("open bmp failed");
        return -1;
    }
    
    // 2 获取宽度、高度
    lseek(fd_bmp, 18, SEEK_SET);
    read(fd_bmp, &width, sizeof(width));
    read(fd_bmp, &height, sizeof(height));
    printf("图片数据信息:\n\
        width:%d\n\
        height:%d\n", width, height);
    
    // 3 定义图片颜色数据暂存数组大小：宽*高*BGR
    char bmp_buf[width*height*3];
    
    //     4.1 调整偏移量：跳过BMP图片的文件信息数据（前54位数据）
    //     4.2 读取图片的B G R数据(图片的第55位之后都是 B G R数据)
    //     4.n 关闭图片文件
    lseek(fd_bmp, 54, SEEK_SET);
        
    read(fd_bmp, bmp_buf, width*height*3);
        
    close(fd_bmp);
    
    // 5 lcd屏上颜色
    // 5.1 遍历lcd屏像素点
    for(y=y_start; y<(height+y_start); y++){
        for(x=x_strat; x<(width+x_strat); x++){
            // 5.2 获取图片的B G R ，
            b = bmp_buf[i++];
            g = bmp_buf[i++];
            r = bmp_buf[i++];
            
            // 5.3 整合成一个像素点color
            color = r<<16 | g<<8 | b;
            
            // 5.3 像素点上颜色
            Draw_point(x, y, color);
        }
    }
    
}

// 关闭文件
void Nu_init(){
    munmap(p_lcd, 800*480*4);
    // n 关闭驱动文件；
    close(fd_lcd);
}

// lcd屏初始化
int Lcd_init(){    
// 1 打开lcd屏驱动文件；
    fd_lcd = open("/dev/ubuntu_lcd", O_RDWR);
    if(fd_lcd == -1){
        perror("open lcd failed");
        return -1;
    }

    // 2 映射空间 mmap()
    p_lcd = mmap(NULL , 800*480*4, PROT_READ|PROT_WRITE, MAP_SHARED, fd_lcd, 0);
    if(p_lcd == NULL){
        perror("mmap lcd failed");
        return -1;
    }
}