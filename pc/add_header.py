import struct

def upgrade_and_denoise_8bit(raw_pcm_path, output_wav_path):
    with open(raw_pcm_path, "rb") as f:
        raw_data = f.read()
    
    if len(raw_data) == 0:
        print("❌ 错误：读取到的原始数据为空，请检查文件路径！")
        return

    # 1. 第一遍循环：计算直流均值（用于消除直流分量引起的低频电流声）
    total_val = 0
    for b in raw_data:
        total_val += b
    mean_dc = total_val / len(raw_data)

    upgraded_pcm = bytearray()
    last_s16_val = 0  # 用于软件低通滤波的缓存

    # 2. 第二遍循环：去直流 + 16位映射 + 一阶低通滤波（强行滤除高频尖锐电流声）
    for i in range(len(raw_data)):
        # 去除直流偏移，并归一化到 -1.0 到 1.0
        normalized = (raw_data[i] - mean_dc) / 128.0
        
        # 映射到 16位有符号有符号数
        current_s16 = int(normalized * 32767)
        current_s16 = max(-32768, min(32767, current_s16))
        
        # 🎯 一阶低通数字滤波器算法 (系数 0.4 可调)
        # 作用：让平滑的人声通过，强行压制突变的尖锐高频电流杂音
        filtered_s16 = int(0.4 * current_s16 + 0.6 * last_s16_val)
        last_s16_val = filtered_s16
        
        # 打包成字节（小端）
        packed_bytes = struct.pack('<h', filtered_s16)
        
        # 2倍线性插值（8000Hz -> 16000Hz）
        upgraded_pcm.extend(packed_bytes)
        upgraded_pcm.extend(packed_bytes)
        
    # 3. 缝合标准的 44 字节 WAV 头
    pcm_len = len(upgraded_pcm)
    header = bytearray(44)
    header[0:4] = b'RIFF'
    header[4:8] = struct.pack('<I', pcm_len + 36)
    header[8:12] = b'WAVE'
    header[12:16] = b'fmt '
    header[16:20] = struct.pack('<I', 16)
    header[20:22] = struct.pack('<H', 1)      # PCM格式
    header[22:24] = struct.pack('<H', 1)      # 单声道
    header[24:28] = struct.pack('<I', 16000)  # 16000Hz
    header[28:32] = struct.pack('<I', 32000)  # 字节率
    header[32:34] = struct.pack('<H', 2)      # 块对齐
    header[34:36] = struct.pack('<H', 16)     # 16位
    header[36:40] = b'data'
    header[40:44] = struct.pack('<I', pcm_len)
    
    with open(output_wav_path, "wb") as f:
        f.write(header)
        f.write(upgraded_pcm)
        
    print(f"✨ 降噪与越级拉伸成功！标准音频已生成：{output_wav_path}")

# 🚀 盘它！
upgrade_and_denoise_8bit("real_hardware_voice.wav", "final_xfyun_voice.wav")