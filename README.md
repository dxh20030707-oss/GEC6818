# GEC6818 云边协同智能语音问答系统 — 代码归档

基于三星 S5P6818（GEC6818 开发板，ARM Cortex-A53）+ Ubuntu 节点的分布式语音问答系统。
板端只做音视频 I/O 与 TCP Socket 传输，大模型问答与语音编解码卸载到 Ubuntu 节点。

## 目录结构

```
GEC6818/
├── board/                      板端（GEC6818，交叉编译 C）
│   ├── client.c                主程序（最终版）
│   ├── client1.c               旧版客户端（已被 client.c 取代）
│   ├── client_test.c           网络连通性测试客户端
│   ├── rec_touch.c             录音 + 触摸联调
│   ├── test_record.c           ALSA 录音测试
│   ├── touch.c                 触摸屏测试
│   ├── examples/               LCD / BMP / Socket 实验例程
│   │   ├── 1.c                 open+mmap 帧缓冲显示 24 位 BMP
│   │   ├── lcd_show_bmp.c      BMP 显示（/dev/fb0，800x480）
│   │   ├── show_bmp.c / color.c / color_mmap.c
│   │   ├── main.c / test.c     早期最小例程
│   │   └── server(2).c         教学用 TCP server（端口 3000，非本项目链路）
│   └── assets/                 板端显示用 BMP 资源
│       ├── Picture.bmp / 11.bmp / real_pic.bmp / real_pic1.bmp
├── pc/                         PC 端（Ubuntu 节点，Python 3）
│   ├── server_ubuntu.py        ★ 服务端核心（唯一成品）
│   ├── test_link.py            8888 端口通路测试
│   ├── pull.py                 9999 端口抓板端原始录音（Windows/PC 端接收器）
│   ├── repair_wav.py           给裸流补 44 字节 WAV 头
│   ├── test_decode.py          批量猜采样率/位深（8000/11025/8bit）
│   ├── add_header.py           去直流 + 一阶低通 + 2 倍插值升采样成 16k WAV
│   ├── check_data.py           打印前 50 个采样幅值
│   ├── legacy/                 已废弃，勿用
│   │   ├── server_test.py      server_ubuntu.py 的旧版：AI_API_KEY 是占位符，
│   │   │                       且 _thread 在 import 之前被引用，运行会抛 NameError
│   │   └── test_tts.py         0 字节空文件
│   └── samples/                调试用录音样本
│       ├── real_hardware_voice.wav   板端原始录音（各修复脚本的输入）
│       ├── rec.wav / rec_clean_16k.wav
│       └── test_8000.wav / test_11025.wav / test_8k_8bit.wav
└── deps/                       离线依赖包（sdist）
    ├── websocket_client-0.58.0.tar.gz   ★ server_ubuntu.py 唯一非标准库依赖
    ├── requests-2.25.1.tar.gz / urllib3-1.26.20.tar.gz
    ├── certifi-2020.6.20.tar.gz / certifi-2026.5.20.tar.gz
    └── chardet-4.0.0.tar.gz / idna-2.10.tar.gz / six-1.17.0.tar.gz
```

## 数据链路

```
板端 client.c                           Ubuntu 端 server_ubuntu.py
触摸 /dev/input/event0 点第一下 → 起录音 (dd 采 /dev/dsp, u8/1ch/8000)
触摸再点一下 → /tmp/safe_raw.pcm  ─8888─▶ 收流存 rec.pcm
                                          convert_8k_u8_to_16k_s16_wav()
                                          → rec_clean_16k.wav
                                          讯飞 IAT 听写 (wss iat-api.xfyun.cn)
                                          → get_ai_answer() 通义千问 qwen-turbo
                                          讯飞 TTS (wss tts-api.xfyun.cn, 16k raw)
                                          ffmpeg 转 u8/8k → reply.pcm
板端后台线程守 9999 ◀──9999──────────────  打包「问/答文本 + [AUDIO_START] + PCM」
memmem 切分文本与 PCM → /tmp/reply.pcm
cat /tmp/reply.pcm > /dev/dsp  物理播报
```

## 部署步骤

1. **PC 端（Ubuntu 节点）**
   ```bash
   sudo apt-get install ffmpeg          # TTS 转码依赖 os.system("ffmpeg ...")
   pip3 install --no-index --find-links=./deps \
        websocket-client==0.58.0 requests==2.25.1
   cd pc && python3 server_ubuntu.py    # 监听 0.0.0.0:8888
   ```
2. **板端（GEC6818）**
   ```bash
   arm-linux-gcc client.c -o client -lpthread   # 按你实际的工具链前缀调整
   ./client
   ```
   点一下屏幕开始录音，说完再点一下，等待喇叭播报答复。

## 需要注意

- **IP 要按现场改**：`board/client.c` 里 `SERVER_IP "192.168.2.55"`（Ubuntu 虚拟机 IP）；
  `pc/pull.py` 里绑的是 `192.168.2.10`（PC 的 IP）。换网络后两处都要改。
- **密钥硬编码**：`pc/server_ubuntu.py` 与 `pc/legacy/server_test.py` 中明文写了讯飞
  APPID / API_KEY / API_SECRET 和通义千问 `sk-` key。此归档目录如需外发共享，务必先换掉或清空。
- **运行期产物**不在此归档内（`rec.pcm`、`reply.pcm`、`xf_temp_16k.pcm`、`rec_clean_16k.wav`），
  它们在 `server_ubuntu.py` 的工作目录里自动生成。
- `pc/samples/` 与 `pc/*.py` 中的几个音频工具均以**本地文件**为输入，属于离线排查工具，
  真正上线只需要 `pc/server_ubuntu.py` 一个文件。
