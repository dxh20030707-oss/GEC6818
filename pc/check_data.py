import struct

with open("real_hardware_voice.wav", "rb") as f:
    data = f.read()

# 🎯 打印前 50 个采样点（16位有符号数，2字节一个）
print("📊 正在查看原始采样点的物理幅值：")
try:
    for i in range(0, 100, 2):
        val = struct.unpack('<h', data[i:i+2])[0]
        print(f"采样点 {i//2}: {val}")
except Exception as e:
    print("读取出错:", e)