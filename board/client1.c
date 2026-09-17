#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <linux/input.h>
#include <sys/wait.h>

#define SERVER_IP "192.168.2.10"  // 🎯 你的 Windows 电脑 IP
#define SERVER_PORT 8888          // 🎯 你的 Python 服务器端口

pid_t recording_pid = -1;

// 🎙️ 录音子进程执行的函数
void start_hardware_record() {
    printf("[开发板] 🔴 捕获到按下！开始底层录音...\n");
    // 强行往声卡驱动注入 8000Hz 8bit 参数（对齐刚才测试成功的参数）
    system("echo \"format=u8\" > /dev/dsp 2>/dev/null");
    system("echo \"channels=1\" > /dev/dsp 2>/dev/null");
    system("echo \"rate=8000\" > /dev/dsp 2>/dev/null");

    // 使用 execlp 跑 dd 命令，无限期录音，直到被主进程杀掉
    execlp("dd", "dd", "if=/dev/dsp", "of=/tmp/safe_raw.pcm", "bs=8000", NULL);
    exit(0);
}

// 🚀 发射网络大炮函数
void send_audio_to_windows() {
    printf("[开发板] ⚡ 正在将刚刚录好的原始音频发射到 Windows 电脑...\n");
    int client_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (client_fd < 0) return;

    struct sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(SERVER_PORT);
    inet_pton(AF_INET, SERVER_IP, &serv_addr.sin_addr);

    // 强行恢复网卡并连接 Windows
    system("ifconfig eth0 192.168.2.55 netmask 255.255.255.0 up");
    if (connect(client_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        printf("❌ 连接电脑失败，请检查网络！\n");
        close(client_fd);
        return;
    }

    // 二进制流读取并发送
    FILE *fp = fopen("/tmp/safe_raw.pcm", "rb");
    if (fp) {
        char buf[1024];
        int len;
        while ((len = fread(buf, 1, sizeof(buf), fp)) > 0) {
            send(client_fd, buf, len, 0);
        }
        fclose(fp);
    }
    close(client_fd);
    printf("🎉 发射完毕！等待 Windows 讯飞云端回传文字！\n");
}

int main() {
    // 1. 打开触摸屏设备
    int touch_fd = open("/dev/input/event0", O_RDONLY);
    if (touch_fd < 0) {
        perror("❌ 无法打开触摸屏驱动");
        return -1;
    }

    struct input_event ev;
    printf("\n========= 🚀 自动化语音交互系统已就位 =========\n");
    printf("👉 请在开发板触摸屏上：【长按屏幕】录音 / 【松开屏幕】停止并识别\n\n");

    while (1) {
        int ret = read(touch_fd, &ev, sizeof(struct input_event));
        if (ret < sizeof(struct input_event)) continue;

        // 2. 识别触摸屏的压力/按压事件
        if (ev.type == EV_KEY && ev.code == BTN_TOUCH) {
            if (ev.value == 1) { // 🎯 按下屏幕
                if (recording_pid == -1) {
                    recording_pid = fork(); // 开辟子进程去录音
                    if (recording_pid == 0) {
                        start_hardware_record(); // 子进程进去录音，不出来了
                    }
                }
            } 
            else if (ev.value == 0) { // 🎯 松开屏幕
                if (recording_pid != -1) {
                    printf("[开发板] 🟢 捕获到松开！停止录音。\n");
                    kill(recording_pid, SIGINT); // 物理强制杀死录音进程，停止录音
                    waitpid(recording_pid, NULL, 0); // 回收尸体
                    recording_pid = -1;

                    // 录音一停，立刻连招把音频砸向电脑
                    send_audio_to_windows();
                }
            }
        }
    }

    close(touch_fd);
    return 0;
}