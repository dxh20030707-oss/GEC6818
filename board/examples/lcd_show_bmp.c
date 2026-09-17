#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>

int main(int argc, char **argv)
{
    if(argc < 2) {
        printf("使用方法: ./bmp_show <BMP图片路径>\n");
        return -1;
    }

    // 1. 打开开发板的 LCD 液晶屏设备
    int lcd_fd = open("/dev/fb0", O_RDWR);
    if(lcd_fd < 0) {
        perror("打开LCD失败");
        return -1;
    }

    // 2. 将屏幕文件映射到内存盘（mmap），提升刷图速度，告别卡顿
    // 屏幕大小：800 * 480 * 4字节
    int *lcd_mp = (int *)mmap(NULL, 800*480*4, PROT_READ|PROT_WRITE, MAP_SHARED, lcd_fd, 0);
    if(lcd_mp == MAP_FAILED) {
        perror("液晶屏内存映射失败");
        close(lcd_fd);
        return -1;
    }

    // 3. 打开你要渲染的 BMP 图片
    int bmp_fd = open(argv[1], O_RDONLY);
    if(bmp_fd < 0) {
        perror("打开BMP图片失败");
        munmap(lcd_mp, 800*480*4);
        close(lcd_fd);
        return -1;
    }

    // 4. 跳过 BMP 图片前 54 个字节的“文件头”信息，直接去摸像素数据
    lseek(bmp_fd, 54, SEEK_SET);

    // 5. 读取图片的像素数据（假设你的图片刚好是 800x480 分辨率）
    // 24位 BMP 每个像素占 3 字节 (BGR)
    char bmp_buf[800 * 480 * 3];
    read(bmp_fd, bmp_buf, sizeof(bmp_buf));

    // 6. 将图片像素投射到 LCD 内存映射区，同时修正上下颠倒的硬件算法
    int i = 0;
    for(int y=0; y<480; y++) {
        for(int x=0; x<800; x++) {
            // 提取 BMP 里的 B、G、R 三原色
            unsigned char b = bmp_buf[i++];
            unsigned char g = bmp_buf[i++];
            unsigned char r = bmp_buf[i++];
            
            // 组装成开发板认识的 32位 ARGB 像素
            int color = (r << 16) | (g << 8) | b;

            // 核心公式：通过 (479 - y) 强行把底朝天的 BMP 像素拉正过来！
            lcd_mp[(479 - y) * 800 + x] = color;
        }
    }

    // 7. 善后处理，关闭通道
    close(bmp_fd);
    munmap(lcd_mp, 800*480*4);
    close(lcd_fd);

    printf("🎉 图片 %s 渲染成功！\n", argv[1]);
    return 0;
}