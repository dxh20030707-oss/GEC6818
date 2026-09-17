#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <linux/input.h>

// 根据GEC6818官方固件，触摸屏设备一般为 /dev/input/event0 或 event1
#define TOUCH_DEV "/dev/input/event0" 
#define RECORD_WAV "/tmp/rec.wav"

pid_t rec_pid = -1; // 用于记录录音进程的PID

// 启动录音的函数
void start_recording() {
    if (rec_pid > 0) {
        return; // 防止重复触发
    }

    printf("[INFO] 检测到按下，开始录音...\n");

    rec_pid = fork();
    if (rec_pid == 0) {
        // 子进程：调用系统 alsa-utils 的 arecord 工具
        // 参数说明：-d 0 (不限时), -c 1 (单声道), -r 16000 (16k采样率), -f S16_LE (16位小端格式，讯飞通用)
        execlp("arecord", "arecord", "-d", "0", "-c", "1", "-r", "16000", "-f", "S16_LE", RECORD_WAV, NULL);
        
        // 如果 execlp 失败则执行下面语句
        perror("execlp arecord failed");
        exit(1);
    } else if (rec_pid < 0) {
        perror("fork failed");
    }
}

// 停止录音的函数
void stop_recording() {
    if (rec_pid > 0) {
        printf("[INFO] 检测到松开，结束录音，正在保存文件...\n");
        
        // 向 arecord 子进程发送中断信号（相当于按了 Ctrl+C），让其完整写入 WAV 文件头
        kill(rec_pid, SIGINT); 
        
        // 回收子进程，避免僵尸进程
        int status;
        waitpid(rec_pid, &status, 0); 
        
        printf("[SUCCESS] 录音已保存至: %s\n\n", RECORD_WAV);
        rec_pid = -1; // 重置PID计数
    }
}

int main() {
    int fd = open(TOUCH_DEV, O_RDONLY);
    if (fd < 0) {
        perror("打开触摸屏设备失败，请检查设备路径");
        return -1;
    }

    struct input_event ev;
    printf("[READY] 请按住开发板屏幕开始录音，松开结束...\n\n");

    while (1) {
        // 阻塞读取输入事件
        if (read(fd, &ev, sizeof(struct input_event)) < (int)sizeof(struct input_event)) {
            continue;
        }

        // 过滤事件类型：EV_KEY (按键/触摸点击事件)，编码为 BTN_TOUCH
        if (ev.type == EV_KEY && ev.code == BTN_TOUCH) {
            if (ev.value == 1) {
                // 值为1表示按下
                start_recording();
            } else if (ev.value == 0) {
                // 值为0表示松开
                stop_recording();
            }
        }
    }

    close(fd);
    return 0;
}