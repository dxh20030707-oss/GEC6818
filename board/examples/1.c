#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

typedef unsigned short WORD;
typedef unsigned int DWORD;
typedef unsigned int LONG;

// BMP文件头（14字节）
struct tagBITMAPFILEHEADER
{
    WORD bfType;
    DWORD bfSize;
    WORD bfReserved1;
    WORD bfReserved2;
    DWORD bfOffBits;
} __attribute__((packed));

// BMP信息头（40字节）
struct tagBITMAPINFOHEADER
{
    DWORD biSize;
    LONG biWidth;
    LONG biHeight;
    WORD biPlanes;
    WORD biBitCount;
    DWORD biCompression;
    DWORD biSizeImage;
    LONG biXPelsPerMeter;
    LONG biYPelsPerMeter;
    DWORD biClrUsed;
    DWORD biClrImportant;
} __attribute__((packed));

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        printf("用法: %s <bmp文件路径>\n", argv[0]);
        return -1;
    }

    // 1. 打开LCD帧缓冲设备
    int lcdFd = open("/dev/fb0", O_RDWR);
    if (lcdFd == -1)
    {
        perror("打开LCD设备失败");
        return -1;
    }

    // 2. 打开BMP图片
    int bmpFd = open(argv[1], O_RDONLY);
    if (bmpFd == -1)
    {
        perror("打开BMP文件失败");
        close(lcdFd);
        return -1;
    }

    // 读取BMP文件头和信息头
    struct tagBITMAPFILEHEADER bfh;
    struct tagBITMAPINFOHEADER bih;
    read(bmpFd, &bfh, sizeof(bfh));
    read(bmpFd, &bih, sizeof(bih));

    // 检查是否是BMP文件，且为24位真彩色
    if (bfh.bfType != 0x4D42 || bih.biBitCount != 24)
    {
        printf("只支持24位BMP图片\n");
        close(lcdFd);
        close(bmpFd);
        return -1;
    }

    int width = bih.biWidth;
    int height = bih.biHeight;
    printf("图片信息：宽=%d, 高=%d, 位深=%d\n", width, height, bih.biBitCount);

    // 计算BMP行对齐后的字节数（必须是4的倍数）
    int lineByte = (width * 3 + 3) / 4 * 4;

    // 分配动态内存存储BMP像素数据
    unsigned char *bmpData = (unsigned char *)malloc(lineByte * height);
    if (!bmpData)
    {
        perror("内存分配失败");
        close(lcdFd);
        close(bmpFd);
        return -1;
    }

    // 定位到像素数据起始位置
    lseek(bmpFd, bfh.bfOffBits, SEEK_SET);
    read(bmpFd, bmpData, lineByte * height);

    // 分配帧缓冲数据（假设LCD是32位ARGB格式）
    unsigned int *lcdBuf = (unsigned int *)malloc(width * height * 4);
    if (!lcdBuf)
    {
        perror("内存分配失败");
        free(bmpData);
        close(lcdFd);
        close(bmpFd);
        return -1;
    }

    // BMP从下往上存储，转换为LCD从上往下显示，并转换为ARGB格式
    for (int y = 0; y < height; y++)
    {
        for (int x = 0; x < width; x++)
        {
            // BMP数据：BGR顺序，且从下往上存储
            unsigned char *p = bmpData + (height - 1 - y) * lineByte + x * 3;
            unsigned char b = p[0];
            unsigned char g = p[1];
            unsigned char r = p[2];
            // 转换为ARGB格式（A=0x00）
            lcdBuf[y * width + x] = (0x00 << 24) | (r << 16) | (g << 8) | b;
        }
    }

    // 写入LCD帧缓冲
    write(lcdFd, lcdBuf, width * height * 4);

    // 释放资源
    free(bmpData);
    free(lcdBuf);
    close(lcdFd);
    close(bmpFd);

    printf("图片显示完成\n");
    return 0;
}
