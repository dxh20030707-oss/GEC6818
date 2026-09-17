# -*- coding:utf-8 -*-
import socket

def start_test_server():
    # 创建一个纯正的 TCP Socket
    server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    
    # 监听 8888 端口
    server_socket.bind(('0.0.0.0', 8888))
    server_socket.listen(5)
    print("🛰️  [Ubuntu 测试端] 已就位，正在死等开发板的触控网络信号（8888端口）...")

    try:
        while True:
            client_socket, client_address = server_socket.accept()
            # 只要板子连上来了，不管发什么，都说明网络通路是通的！
            print("🔥 [网络大通关!!] 成功接收到来自开发板 ({}) 的触控网络握手！".format(client_address[0]))
            
            # 盲读一下板子发过来的数据
            data = client_socket.recv(1024)
            if data:
                print("📩 [板子捎来的话] >>> {}".format(data.decode('utf-8', errors='ignore')))
                
            client_socket.close()
    except KeyboardInterrupt:
        print("\n测试结束。")
    finally:
        server_socket.close()

if __name__ == "__main__":
    start_test_server()