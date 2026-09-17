#define _GNU_SOURCE  // 🎯 核心：必须放在最顶行，开启 GNU 扩展以完美支持 memmem 函数
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <linux/input.h>
#include <string.h>    // memmem 函数及字符串处理核心头文件
#include <pthread.h>

#define SERVER_IP "192.168.2.55"  // Ubuntu 虚拟机 IP
#define SERVER_PORT 8888          // 接收开发板音频的 Ubuntu 端口

// 🚀 发射网络大炮：将开发板录制好的 PCM 音频裸流发射给 Ubuntu
void send_audio_to_ubuntu() {
    printf("[开发板] ⚡ 正在将有效原始音频发射到 Ubuntu 虚拟机...\n");
    int client_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (client_fd < 0) return;

    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(SERVER_PORT);
    inet_pton(AF_INET, SERVER_IP, &serv_addr.sin_addr);

    if (connect(client_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        printf("❌ 连接 Ubuntu 失败！请检查 Ubuntu 的 Python 服务器是否拉起。\n");
        close(client_fd);
        return;
    }

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
    printf("🎉 发射完毕！等待双端路由回传并播报...\n");
}

// 📡 满血双全工后台监听子线程：死守 9999 端口，分割文本与声音大炮，实现硬件语音播报
void *back_server_thread(void *arg) {
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    struct sockaddr_in address;
    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(9999); 

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("❌ 线程内绑定 9999 端口失败");
        return NULL;
    }
    listen(server_fd, 5);

    // 准备一个 500KB 的大缓存区，用来全量吞入来自 Ubuntu 的文本+声音复合大包
    static char big_buffer[1024 * 500]; 

    while (1) {
        int client_socket = accept(server_fd, NULL, NULL);
        if (client_socket >= 0) {
            int total_bytes = 0;
            int n;
            
            // 循环接收所有下发的数据数据流
            while ((n = read(client_socket, big_buffer + total_bytes, 2048)) > 0) {
                total_bytes += n;
                if (total_bytes >= sizeof(big_buffer) - 1) break; // 边界安全保护
            }
            close(client_socket);

            if (total_bytes > 0) {
                // 🔍 精准寻找音视频分割符 [AUDIO_START]
                char *divider = memmem(big_buffer, total_bytes, "[AUDIO_START]", 13);
                if (divider != NULL) {
                    
                    // 1. 物理截断文本区域并打印在控制台
                    int text_len = divider - big_buffer;
                    printf("\n==================================================");
                    printf("\n🎉 [SecureCRT 收到 AI 答复文本]:\n");
                    fwrite(big_buffer, 1, text_len, stdout);
                    printf("\n==================================================\n");

                    // 2. 剥离出隔离符屁股后面的纯 PCM 音频裸流
                    char *audio_start = divider + 13;
                    int audio_len = total_bytes - text_len - 13;
                    
                    if (audio_len > 0) {
                        printf("📢 [物理播报引擎] 成功收到分级音频流 (%d 字节)，强行灌入喇叭...\n", audio_len);
                        FILE *audio_fp = fopen("/tmp/reply.pcm", "wb");
                        if (audio_fp) {
                            fwrite(audio_start, 1, audio_len, audio_fp);
                            fclose(audio_fp);
                            
                            // 3. 物理级释放声卡设备，配对 8000Hz U8 喇叭最喜欢的清爽参数
                            system("killall -9 dd 2>/dev/null");
                            system("echo \"format=u8\" > /dev/dsp 2>/dev/null");
                            system("echo \"channels=1\" > /dev/dsp 2>/dev/null");
                            system("echo \"rate=8000\" > /dev/dsp 2>/dev/null");
                            
                            // 4. 利用 cat 同步阻塞将声音倾倒进声卡，喇叭瞬间开嗓！
                            system("cat /tmp/reply.pcm > /dev/dsp");
                            printf("🏁 [播报完毕] 喇叭硬件通道恢复静默。\n\n");
                        }
                    }
                }
            }
        }
    }
    return NULL;
}

int main() {
    // 启动多线程双全工接收、播报看门狗
    pthread_t thread_id;
    pthread_create(&thread_id, NULL, back_server_thread, NULL);

    int touch_fd = open("/dev/input/event0", O_RDONLY);
    if (touch_fd < 0) {
        perror("❌ 无法打开触摸屏驱动设备");
        return -1;
    }

    struct input_event ev;
    printf("\n========= 🚀 音频全双工·智能语音播报交互系统已全面就位 =========");
    printf("\n👉 使用方法：【轻轻点按一下屏幕】开启录音 -> 【再点按一下屏幕】停止并播报答复");
    printf("\n💡 优点：彻底免除长按干扰，一根手指单点两次，彻底避开任何电磁抢电闪断！\n\n");

    int system_state = 0; // 0-闲置状态，1-录音状态状态机自锁锁

    while (1) {
        int ret = read(touch_fd, &ev, sizeof(struct input_event));
        if (ret < sizeof(struct input_event)) continue;

        // 🎯 核心自锁触发状态机：只认手指按下去（value == 1）的那一发点射，物理上彻底扔掉抬起信号
        if (ev.type == EV_KEY && ev.code == BTN_TOUCH && ev.value == 1) {
            
            if (system_state == 0) {
                // ========= 🟢 动作：从闲置切入录音模式 =========
                system_state = 1;
                printf("\n🎙️  [自锁启动] -> 🔴 开启录音！请对着麦克风说话，完事后再轻轻点一次屏幕...\n");
                
                // 彻底强杀可能抢占声卡的内鬼
                system("killall -9 dd 2>/dev/null");
                unlink("/tmp/safe_raw.pcm");
                
                // 刷入板子物理录音硬件参数
                system("echo \"format=u8\" > /dev/dsp 2>/dev/null");
                system("echo \"channels=1\" > /dev/dsp 2>/dev/null");
                system("echo \"rate=8000\" > /dev/dsp 2>/dev/null");
                
                // 启动后台独占录音进程，平稳运行
                system("dd if=/dev/dsp of=/tmp/safe_raw.pcm bs=8000 2>/dev/null &");
                
                // 强行冷冻 500ms 避开抬手震荡，防止电容屏敏感连击
                usleep(500000);
            } 
            else if (system_state == 1) {
                // ========= 🟢 动作：从录音模式切入发送阶段 =========
                system_state = 0;
                printf("\n🛑 [自锁终止] -> 🟢 终止录音！正在打包音频大炮向 Ubuntu 发射...\n");
                
                // 掐死进程，稳固文件落盘
                system("killall -9 dd 2>/dev/null");
                usleep(100000); // 留 100ms 缓冲
                
                // 砸向网络
                send_audio_to_ubuntu();
                
                // 同样冷冻 500ms 保护
                usleep(500000);
                printf("\n🔄 系统自锁复位就位，等待下一次轻触开火...\n\n");
            }
        }
    }

    close(touch_fd);
    return 0;
}