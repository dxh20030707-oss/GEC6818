# -*- coding:utf-8 -*-
import socket
import websocket
import datetime
import hashlib
import base64
import hmac
import json
from urllib.parse import urlencode
import time
import ssl
import struct
import os
import urllib.request
import _thread

# ==================== 🛠️ 密钥配置区 ====================
APPID = "4dea6200"
API_SECRET = "OTlY1ZDA1ZmI3OWFi"
API_KEY = "f"

AI_API_KEY = ""  
AI_URL = "https://dashscope.aliyuncs.com/compatible-mode/v1/chat/completions"
AI_MODEL = "qwen-turbo"
# =======================================================

STATUS_FIRST_FRAME = 0
STATUS_CONTINUE_FRAME = 1
STATUS_LAST_FRAME = 2

# 调用通义千问兼容接口，把语音识别得到的问题转换成简短的文字答案。
def get_ai_answer(question_text): # 调用大模型API获取答案
    global AI_API_KEY, AI_URL, AI_MODEL
    try:
        headers = {"Authorization": "Bearer {}".format(AI_API_KEY), "Content-Type": "application/json"}
        data = {
            "model": AI_MODEL,
            "messages": [
                {"role": "system", "content": "你是一个嵌入式助手。请用最精炼的一句话回答用户，字数严格控制在20字以内。"},
                {"role": "user", "content": question_text}
            ]
        }
        req = urllib.request.Request(AI_URL, data=json.dumps(data).encode('utf-8'), headers=headers, method='POST')
        with urllib.request.urlopen(req, timeout=5) as response:
            res_data = json.loads(response.read().decode('utf-8'))
            return res_data['choices'][0]['message']['content'].strip()
    except Exception as e:
        return "大模型脑部连接超时"

# 保存讯飞语音合成请求所需的认证信息、业务参数和待合成文本。
class TTS_Param(object):
    # 初始化讯飞 TTS 请求的公共参数、业务参数和文本数据。
    def __init__(self, APPID, APIKey, APISecret, Text):
        self.APPID = APPID; self.APIKey = APIKey; self.APISecret = APISecret; self.Text = Text
        self.CommonArgs = {"app_id": self.APPID}
        self.BusinessArgs = {"aue": "raw", "auf": "audio/L16;rate=16000", "vcn": "xiaoyan", "tte": "utf8"}
        self.Data = {"status": 2, "text": str(base64.b64encode(self.Text.encode('utf-8')), "utf-8")}

    # 按讯飞要求使用 HMAC-SHA256 生成带鉴权参数的 WebSocket 地址。
    def create_url(self):
        url = 'wss://tts-api.xfyun.cn/v2/tts'
        now = datetime.datetime.now(); date = now.strftime('%a, %d %b %Y %H:%M:%S GMT')
        signature_origin = "host: tts-api.xfyun.cn\ndate: " + date + "\nGET /v2/tts HTTP/1.1"
        signature_sha = hmac.new(self.APISecret.encode('utf-8'), signature_origin.encode('utf-8'), hashlib.sha256).digest()
        signature_sha = base64.b64encode(signature_sha).decode(encoding='utf-8')
        authorization_origin = 'api_key="{}", algorithm="hmac-sha256", headers="host date request-line", signature="{}"'.format(self.APIKey, signature_sha)
        authorization = base64.b64encode(authorization_origin.encode('utf-8')).decode(encoding='utf-8')
        return url + '?' + urlencode({"authorization": authorization, "date": date, "host": "tts-api.xfyun.cn"})

# WebSocket 错误回调：当前程序不需要额外处理错误，所以保持为空。
def dummy_error(ws, error): pass
# WebSocket 关闭回调：当前程序不需要额外处理关闭事件，所以保持为空。
def dummy_close(ws, close_status_code, close_msg): pass

# 调用讯飞 TTS，把答案转换成板端可以播放的 8 kHz、8-bit、单声道 PCM。
def generate_tts_pcm_xf(text_content, output_pcm_path):
    if os.path.exists("xf_temp_16k.pcm"): os.remove("xf_temp_16k.pcm")
    if os.path.exists(output_pcm_path): os.remove(output_pcm_path)
    
    # 接收讯飞分片返回的音频，并在收到最后一片后统一转码。
    def on_tts_message(ws, message):
        msg = json.loads(message)
        if msg["code"] == 0:
            audio = base64.b64decode(msg["data"]["audio"])
            with open("xf_temp_16k.pcm", "ab") as f: f.write(audio)
        if msg["data"]["status"] == 2:
            os.system("ffmpeg -y -f s16le -ar 16000 -ac 1 -i xf_temp_16k.pcm -f u8 -acodec pcm_u8 -ar 8000 -ac 1 {} >/dev/null 2>&1".format(output_pcm_path))
            ws.close()

    tts_param = TTS_Param(APPID, API_KEY, API_SECRET, text_content)
    ws = websocket.WebSocketApp(tts_param.create_url(), on_message=on_tts_message, on_error=dummy_error, on_close=dummy_close)
    ws.on_open = lambda w: _thread.start_new_thread(lambda: w.send(json.dumps({"common": tts_param.CommonArgs, "business": tts_param.BusinessArgs, "data": tts_param.Data})), ())
    ws.run_forever(sslopt={"cert_reqs": ssl.CERT_NONE})

# 生成答案语音，并把“问题、答案文本 + 音频数据”一次性发送回开发板。
def send_text_and_voice_to_board(board_ip, question, answer):
    try:
        generate_tts_pcm_xf(answer, "reply.pcm")
        if not os.path.exists("reply.pcm") or os.path.getsize("reply.pcm") < 100:
            return

        back_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        back_socket.settimeout(4)  
        back_socket.connect((board_ip, 9999))
        
        with open("reply.pcm", "rb") as f:
            pcm_bytes = f.read()
            
        text_payload = "❓ 问: {}\n🤖 答: {}\n".format(question, answer)
        final_packet = text_payload.encode('utf-8') + b"[AUDIO_START]" + pcm_bytes
        
        back_socket.sendall(final_packet)
        back_socket.close()
        print("⚡ [状态] 音视频同步回传板子成功！")
    except Exception as e:
        print("❌ [状态] 回传板子失败:", e)

# 保存讯飞语音识别请求所需的认证信息和识别参数。
class Ws_Param(object):
    # 初始化讯飞 IAT 请求的公共参数和业务参数。
    def __init__(self, APPID, APIKey, APISecret):
        self.APPID = APPID; self.APIKey = APIKey; self.APISecret = APISecret
        self.CommonArgs = {"app_id": self.APPID}
        self.BusinessArgs = {"domain": "iat", "language": "zh_cn", "accent": "mandarin", "vinfo": 1, "vad_eos": 10000}

    # 按讯飞要求生成带鉴权参数的语音识别 WebSocket 地址。
    def create_url(self):
        url = 'wss://iat-api.xfyun.cn/v2/iat'
        now = datetime.datetime.now(); date = now.strftime('%a, %d %b %Y %H:%M:%S GMT')
        signature_origin = "host: iat-api.xfyun.cn\ndate: " + date + "\nGET /v2/iat HTTP/1.1"
        signature_sha = hmac.new(self.APISecret.encode('utf-8'), signature_origin.encode('utf-8'), hashlib.sha256).digest()
        signature_sha = base64.b64encode(signature_sha).decode(encoding='utf-8')
        authorization_origin = 'api_key="{}", algorithm="hmac-sha256", headers="host date request-line", signature="{}"'.format(self.APIKey, signature_sha)
        authorization = base64.b64encode(authorization_origin.encode('utf-8')).decode(encoding='utf-8')
        return url + '?' + urlencode({"authorization": authorization, "date": date, "host": "iat-api.xfyun.cn"})

# 处理讯飞识别结果：拼接文字、调用大模型，并触发答案回传。
def on_message(ws, message):
    try:
        code = json.loads(message)["code"]
        if code == 0:
            data = json.loads(message)["data"]["result"]["ws"]
            result = "".join([w["w"] for i in data for w in i["cw"]])
            if result.strip():
                # 过滤无意义的标点符号误触
                if result.strip() in ["。", "？", "，", "."]:
                    return
                
                global current_board_ip
                print("\n" + "─"*50)
                print("❓ 用户提问: \033[1;32m{}\033[0m".format(result))  # 绿色高亮提问
                
                ai_response = get_ai_answer(result)
                print("🤖 AI 答复 : \033[1;36m{}\033[0m".format(ai_response)) # 青色高亮回答
                print("─"*50)
                
                send_text_and_voice_to_board(current_board_ip, result, ai_response)
    except Exception as e:
        pass

# 语音识别 WebSocket 错误回调；当前只忽略错误，不中断主服务。
def on_error(ws, error): pass
# 语音识别 WebSocket 关闭回调；当前不需要额外清理动作。
def on_close(ws, a, b): pass
# 语音识别 WebSocket 建立后，按讯飞协议分片上传 WAV 中的 PCM 数据。
def on_open(ws):
    # 在独立线程中发送音频，避免阻塞 WebSocket 的事件循环。
    def run(*args):
        frameSize = 8000; status = STATUS_FIRST_FRAME
        with open("rec_clean_16k.wav", "rb") as fp:
            fp.seek(44)
            while True:
                buf = fp.read(frameSize)
                if not buf: status = STATUS_LAST_FRAME
                if status == STATUS_FIRST_FRAME:
                    ws.send(json.dumps({"common": wsParam.CommonArgs, "business": wsParam.BusinessArgs, "data": {"status": 0, "format": "audio/L16;rate=16000", "audio": str(base64.b64encode(buf), 'utf-8'), "encoding": "raw"}}))
                    status = STATUS_CONTINUE_FRAME
                elif status == STATUS_CONTINUE_FRAME:
                    ws.send(json.dumps({"data": {"status": 1, "format": "audio/L16;rate=16000", "audio": str(base64.b64encode(buf), 'utf-8'), "encoding": "raw"}}))
                elif status == STATUS_LAST_FRAME:
                    ws.send(json.dumps({"data": {"status": 2, "format": "audio/L16;rate=16000", "audio": str(base64.b64encode(buf), 'utf-8'), "encoding": "raw"}}))
                    time.sleep(1); break
                time.sleep(0.04)
        ws.close()
    _thread.start_new_thread(run, ())

# 将板端的 8 kHz、8-bit 无符号 PCM 放大为 16 kHz、16-bit 有符号 WAV。
def convert_8k_u8_to_16k_s16_wav(input_path, output_path):
    if not os.path.exists(input_path): return False
    with open(input_path, "rb") as f: raw_data = f.read()
    if len(raw_data) < 100: return False 
    upgraded_pcm = bytearray()
    for i in range(len(raw_data)):
        u8_val = raw_data[i]; s16_val = int((u8_val - 128) * 256); s16_val = max(-32768, min(32767, s16_val))
        packed_bytes = struct.pack('<h', s16_val); upgraded_pcm.extend(packed_bytes); upgraded_pcm.extend(packed_bytes)
    pcm_len = len(upgraded_pcm); header = bytearray(44)
    header[0:4] = b'RIFF'; header[4:8] = struct.pack('<I', pcm_len + 36); header[8:12] = b'WAVE'; header[12:16] = b'fmt '; header[16:20] = struct.pack('<I', 16); header[20:22] = struct.pack('<H', 1); header[22:24] = struct.pack('<H', 1); header[24:28] = struct.pack('<I', 16000); header[28:32] = struct.pack('<I', 32000); header[32:34] = struct.pack('<H', 2); header[34:36] = struct.pack('<H', 16); header[36:40] = b'data'; header[40:44] = struct.pack('<I', pcm_len)
    with open(output_path, "wb") as f: f.write(header); f.write(upgraded_pcm)
    return True

# 启动 8888 TCP 服务：接收板端录音，并串联音频转换、识别、问答和回传流程。
def start_socket_server():
    server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    server_socket.bind(('0.0.0.0', 8888))
    server_socket.listen(5)
    print("\n========================================================")
    print("🚀  [GEC6818 语音交互全双工系统] 满血上线！正在监听 8888 端口...")
    print("========================================================")
    global current_board_ip
    while True:
        client_socket, client_address = server_socket.accept()
        current_board_ip = client_address[0]
        with open("rec.pcm", "wb") as f:
            while True:
                data = client_socket.recv(1024)
                if not data: break
                f.write(data)
        client_socket.close()
        filesize = os.path.getsize("rec.pcm") if os.path.exists("rec.pcm") else 0
        if filesize < 100: continue
        
        print("\n📥 [状态] 收到板子语音流 ({} 字节)...".format(filesize))
        if convert_8k_u8_to_16k_s16_wav("rec.pcm", "rec_clean_16k.wav"):
            global wsParam; wsParam = Ws_Param(APPID, API_KEY, API_SECRET)
            ws = websocket.WebSocketApp(wsParam.create_url(), on_message=on_message, on_error=on_error, on_close=on_close)
            ws.on_open = on_open; ws.run_forever(sslopt={"cert_reqs": ssl.CERT_NONE})

if __name__ == "__main__":
    start_socket_server()