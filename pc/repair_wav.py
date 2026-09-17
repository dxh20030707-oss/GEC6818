import struct

def wav_header(pcm_length, sample_rate=16000, channels=1, bit_depth=16):
    """生成 44 字节的标准 WAV 头部数组"""
    wave_header = bytearray(44)
    # 1-4: RIFF 标志
    wave_header[0:4] = b'RIFF'
    # 5-8: 下一个文件总大小 (PCM大小 + 36)
    wave_header[4:8] = struct.pack('<I', pcm_length + 36)
    # 9-12: WAVE 标志
    wave_header[8:12] = b'WAVE'
    # 13-16: fmt 标志
    wave_header[12:16] = b'fmt '
    # 17-20: 过渡字节 (16)
    wave_header[16:20] = struct.pack('<I', 16)
    # 21-22: 格式类别 (1 代表 PCM 裸流)
    wave_header[20:22] = struct.pack('<H', 1)
    # 23-24: 通道数
    wave_header[22:24] = struct.pack('<H', channels)
    # 25-28: 采样率
    wave_header[24:28] = struct.pack('<I', sample_rate)
    # 29-32: 每秒数据字节率
    wave_header[28:32] = struct.pack('<I', int(sample_rate * channels * bit_depth / 8))
    # 33-34: 数据块对齐
    wave_header[32:34] = struct.pack('<H', int(channels * bit_depth / 8))
    # 35-36: 采样位数
    wave_header[34:36] = struct.pack('<H', bit_depth)
    # 37-40: data 标志
    wave_header[36:40] = b'data'
    # 41-44: PCM 纯音频数据总长度
    wave_header[40:44] = struct.pack('<I', pcm_length)
    return wave_header

# 📂 读取刚才拿到那段 78KB 的裸流文件
# (如果你刚才用 nc 传过来的名字叫 real_hardware_voice.wav 或 rec.wav，请在这对应改名)
raw_file = "real_hardware_voice.wav" 

with open(raw_file, "rb") as f:
    pcm_data = f.read()

# 🎯 缝合：44字节头 + 纯音频裸流
header = wav_header(len(pcm_data), sample_rate=16000, channels=1, bit_depth=16)
fixed_file = "perfect_hardware_voice.wav"

with open(fixed_file, "wb") as f:
    f.write(header)
    f.write(pcm_data)

print(f"🎉 修复大成功！已经为您生成了标准的 WAV 音频：{fixed_file}")
print("👉 现在您可以直接在电脑上双击它，听听录进去的声音了！")