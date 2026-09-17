import socket

# 🎯 建立一个干净的 TCP 监听服务（用 9999 端口）
server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
server.bind(('192.168.2.10', 9999))
server.listen(1)

print("🚀 Windows 接收器已启动，正在 9999 端口死等开发板发送原始音频...")
conn, addr = server.accept()

# 📂 直接在 D:\gongxiang 写入真正的硬件录音
with open("real_hardware_voice.wav", "wb") as f:
    while True:
        data = conn.recv(1024)
        if not data:
            break
        f.write(data)

print("🎉 绝杀成功！录音已安全存入 D:\\gongxiang\\real_hardware_voice.wav")
conn.close()
server.close()