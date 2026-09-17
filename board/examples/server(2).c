#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define PORT 3000     // 监听端口
#define BUF_SIZE 1024 // 缓冲区大小

// 服务器在Ubuntu上请使用gcc编译
int main(int argc, char const *argv[])
{
    int server_fd, new_socket; // socket文件描述符，linux网络通讯全部通过socket
    ssize_t valread;
    struct sockaddr_in address; // 结构体用于存储ip地址
    int opt = 1;
    int addrlen = sizeof(address);
    char buffer[BUF_SIZE] = {0}; // 接收数据缓冲区
    

    // 1. 创建 socket 文件描述符 AF_INET 使用IPV4, SOCK_STREAM 使用TCP协议
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // 2. 设置端口复用（防止端口被占用）
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)))
    {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;         // IPv4 
    address.sin_addr.s_addr = INADDR_ANY; // 监听所有网卡 绑定当前电脑自身的ip
    address.sin_port = htons(PORT);       // 端口（转网络字节序）

    // 3. 绑定 ip 和 端口 3000
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0)
    {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    // 4. 开始监听，最多挂起 3 个连接
    if (listen(server_fd, 3) < 0)
    {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port %d...\n", PORT);

    // 5. 等待客户端连接（阻塞）
    if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0)
    {
        perror("accept");
        exit(EXIT_FAILURE);
    }
    // 死循环不断地读取数据
    while (1)
    {
        // 5. 服务器接收到客户端发来的数据
        valread = read(new_socket, buffer, BUF_SIZE);
        printf("Client: %s\n", buffer);
        
        // 6. 服务器向客户端返回数据
        write(new_socket, "收到", strlen("收到"));
        printf("Hello message sent to client\n");
    }

     // 8. 关闭 socket
    close(new_socket);
    close(server_fd);

    return 0;
}
