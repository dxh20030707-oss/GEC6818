#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/soundcard.h> // 🎯 引入 Linux 标准音频控制结构

#define AUDIO_DEV "/dev/snd/pcmC0D0c" // 你的开发板录音节点

// 🎯 标准 WAV 44字节头结构体定义
struct WAV_HEADER {
    char chunk_id[4];          // "RIFF"
    int chunk_size;            // 文件总大小 - 8
    char format[4];            // "WAVE"
    char sub_chunk1_id[4];     // "fmt "
    int sub_chunk1_size;       // 16
    short audio_format;        // 1 (PCM)
    short num_channels;        // 1 (单声道)
    int sample_rate;           // 16000
    int byte_rate;             // 16000 * 1 * 2 = 32000
    short block_align;         // 1 * 2 = 2
    short bits_per_sample;     // 16
    char sub_chunk2_id[4];     // "data"
    int sub_chunk2_size;       // 纯音频数据大小
};

int main() {
    int pcm_fd, file_fd;
    int sample_rate = 16000;
    int channels = 1;
    int format = AFMT_S16_LE; // 16位小端格式
    int duration = 5;         // 🎯 测试录音 5 秒

    printf("🎬 [HARDWARE] 开始初始化 GEC6818 声卡芯片...\n");

    // 1. 打开录音节点
    pcm_fd = open(AUDIO_DEV, O_RDONLY);
    if (pcm_fd < 0) {
        perror("❌ 无法打开录音节点 /dev/snd/pcmC0D0c");
        return -1;
    }

    // 2. 强行通过 ioctl 配置硬件参数（防止硬件走默认高采样率）
    if (ioctl(pcm_fd, SNDCTL_DSP_SETFMT, &format) < 0) { perror("设置位深失败"); }
    if (ioctl(pcm_fd, SNDCTL_DSP_CHANNELS, &channels) < 0) { perror("设置声道失败"); }
    if (ioctl(pcm_fd, SNDCTL_DSP_SPEED, &sample_rate) < 0) { perror("设置采样率失败"); }

    // 3. 创建本地完美的 WAV 文件
    file_fd = open("6818_real.wav", O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (file_fd < 0) {
        perror("无法创建本地文件");
        close(pcm_fd);
        return -1;
    }

    // 4. 预留 44 字节用来填 WAV 头
    struct WAV_HEADER header = {
        {'R','I','F','F'}, 0, {'W','A','V','E'}, {'f','m','t',' '},
        16, 1, 1, 16000, 32000, 2, 16, {'d','a','t','a'}, 0
    };
    write(file_fd, &header, sizeof(header));

    // 5. 开始疯狂吸取硬件数据
    printf("🎤 [RECORD] >>> 硬件录音通道已稳固！请对着板子 MIC 大声说话 (限时5秒) ...\n");
    
    int total_bytes_to_read = sample_rate * channels * 2 * duration; // 32000 * 5 = 160000 字节
    int bytes_read_total = 0;
    char buffer[1024];

    while (bytes_read_total < total_bytes_to_read) {
        int len = read(pcm_fd, buffer, sizeof(buffer));
        if (len > 0) {
            write(file_fd, buffer, len);
            bytes_read_total += len;
        }
    }

    // 6. 录音倒计时结束，回填真正的文件大小
    header.chunk_size = bytes_read_total + 44 - 8;
    header.sub_chunk2_size = bytes_read_total;
    lseek(file_fd, 0, SEEK_SET);
    write(file_fd, &header, sizeof(header)); // 焊回正确的头部

    printf("🎵 [SUCCESS] 5秒录音完毕！生成文件: 6818_real.wav (大小: %d 字节)\n", bytes_read_total + 44);

    close(file_fd);
    close(pcm_fd);
    return 0;
}