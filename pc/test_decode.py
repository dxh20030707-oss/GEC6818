import struct

def fix_pcm_to_wav(raw_pcm_path, output_wav_path, sample_rate, channels, bit_depth):
    with open(raw_pcm_path, "rb") as f:
        pcm_data = f.read()
    
    pcm_len = len(pcm_data)
    header = bytearray(44)
    
    # 填充 44 字节标准 WAV 头部
    header[0:4] = b'RIFF'
    header[4:8] = struct.pack('<I', pcm_len + 36)
    header[8:12] = b'WAVE'
    header[12:16] = b'fmt '
    header[16:20] = struct.pack('<I', 16)
    header[20:22] = struct.pack('<H', 1)  # 1 代表纯 PCM 格式
    header[22:24] = struct.pack('<H', channels)
    header[24:28] = struct.pack('<I', sample_rate)
    header[28:32] = struct.pack('<I', int(sample_rate * channels * bit_depth / 8))
    header[32:34] = struct.pack('<H', int(channels * bit_depth / 8))
    header[34:36] = struct.pack('<H', bit_depth)
    header[36:40] = b'data'
    header[40:44] = struct.pack('<I', pcm_len)
    
    with open(output_wav_path, "wb") as f:
        f.write(header)
        f.write(pcm_data)
    print(f"✅ 已生成测试文件: {output_wav_path}")

# 🎯 开始盲解你之前 nc 传过来的 78KB 杂音文件
# (请确保 D:\gongxiang 下有一个叫 real_hardware_voice.wav 的 78KB 文件)
raw_file = "real_hardware_voice.wav"

# 猜测 1：老固件最常见的 8000Hz, 16位, 单声道
fix_pcm_to_wav(raw_file, "guess_8000hz_16bit.wav", sample_rate=8000, channels=1, bit_depth=16)

# 猜测 2：有些奇葩驱动默认的 11025Hz, 16位, 单声道
fix_pcm_to_wav(raw_file, "guess_11025hz_16bit.wav", sample_rate=11025, channels=1, bit_depth=16)

# 猜测 3：排除位数错位，测试 8000Hz, 8位（无符号）
fix_pcm_to_wav(raw_file, "guess_8000hz_8bit.wav", sample_rate=8000, channels=1, bit_depth=8)