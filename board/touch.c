#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/mman.h>
#include <linux/fb.h>
#include <linux/input.h>

// 6818触摸屏坐标校准（根据实际硬件调整）
#define X_MIN 0
#define X_MAX 1023
#define Y_MIN 0
#define Y_MAX 600

int main()
{
    // 1. 打开触摸屏设备（6818开发板通常为event0）
    int tfd = open("/dev/input/event0", O_RDONLY);
    if (tfd == -1)
    {
        perror("open /dev/input/event0 failed");
        return -1;
    }
    printf("触摸屏设备打开成功\n");

    // 存储输入事件和坐标
    struct input_event info;
    int x = -1, y = -1;
    int touch_status = 0; // 0:未触摸 1:触摸中

    // 2. 循环读取触摸屏事件
    while (1)
    {
        // 读取事件数据（阻塞式）
        ssize_t ret = read(tfd, &info, sizeof(info));
        if (ret != sizeof(info))
        {
            perror("read touch event failed");
            break;
        }

        // 3. 解析触摸事件类型
        switch (info.type)
        {
            // 按键事件（按下/松开）
            case EV_KEY:
                if (info.code == BTN_TOUCH)
                {
                    touch_status = info.value; // 1:按下 0:松开
                    
                    if (info.value == 0) // 手指松开，输出最终坐标
                    {
                        if (x != -1 && y != -1)
                        {
                            // 坐标校准（适配6818屏幕分辨率）
                            int cal_x = (x - X_MIN) * 1024 / (X_MAX - X_MIN);
                            int cal_y = (y - Y_MIN) * 600 / (Y_MAX - Y_MIN);
                            
                            printf("点击坐标：X = %d, Y = %d (校准后：X = %d, Y = %d)\n", 
                                   x, y, cal_x, cal_y);
                            
                            // 重置坐标
                            x = -1;
                            y = -1;
                        }
                        printf("屏幕松开\n");
                    }
                    else // 手指按下
                    {
                        printf("屏幕按下\n");
                    }
                }
                break;

            // 绝对坐标事件（X/Y轴）
            case EV_ABS:
                if (touch_status == 1) // 仅在触摸中更新坐标
                {
                    if (info.code == ABS_X)
                    {
                        x = info.value; // 获取X轴坐标
                    }
                    else if (info.code == ABS_Y)
                    {
                        y = info.value; // 获取Y轴坐标
                    }
                }
                break;

            // 忽略其他事件类型
            default:
                break;
        }
    }

    // 3. 关闭设备
    close(tfd);
    printf("触摸屏设备已关闭\n");
    return 0;
}
