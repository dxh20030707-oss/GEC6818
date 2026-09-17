#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>

// 🎯 核心对齐：指向你刚刚强行绑定的 Ubuntu 虚拟机 IP
#define UBUNTU_IP "192.168.2.55"  
#define UBUNTU_PORT 8888

void send_test_signal_to_ubuntu() {
    int client_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (client_fd < 0) return;

    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(UBUNTU_PORT);
    inet_pton(AF_INET, UBUNTU_IP, &serv_addr.sin_addr);

    // 尝试跨越网线连向 Ubuntu
    if (connect(client_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        printf("\n❌ [连接失败] 连不上 Ubuntu！快确认 Ubuntu 的 test_link.py 是不是正在运行！\n");
        close(client_fd);
        return;
    }

    // 弹头数据
    char *test_msg = "Hello Ubuntu! Xuhao's network is 100% OK!";
    send(client_fd, test_msg, strlen(test_msg), 0);
    close(client_fd);
    printf("\n📡 [发射成功] 成功向 Ubuntu 送去一枚网络炮弹！\n");
}

int main() {
    printf("\n==================================================");
    printf("\n🚀 [物理层免触控测试] 键盘注入版客户端已就位！");
    printf("\n👉 别管屏幕乱不乱码，直接在电脑键盘上敲 【回车键(Enter)】！");
    printf("\n==================================================\n");

    char ch;
    while (1) {
        // 阻塞死等你在 SecureCRT 终端上敲回车
        int ret = read(STDIN_FILENO, &ch, 1);
        if (ret > 0) {
            if (ch == '\n' || ch == '\r') {
                printf("⌨️  检测到回车键，正在全力打炮...");
                send_test_signal_to_ubuntu();
            }
        }
    }
    return 0;
}