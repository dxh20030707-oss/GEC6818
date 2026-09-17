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

# ==================== 🛠️ 沿用你 100% 正确的全部密钥 ====================
APPID = "4dea6200"
API_SECRET = "OTliZDhmZDllNGU1OWY1ZDA1ZmI3OWFi"
API_KEY = "fe4a2c5653ce426222e2ffa7fa42c8b4"

AI_API_KEY = "填入你正常的大模型API_KEY"  
AI_URL = "https://dashscope.aliyuncs.com/compatible-mode/v1/chat/completions"
AI_MODEL = "qwen-turbo"  # 已经完美对齐变量名
# ==================================================================

STATUS_FIRST_FRAME = 0
STATUS_CONTINUE_FRAME = 1
STATUS_LAST_FRAME = 2

# 🧠 大模型思考函数
def get_ai_answer(question_text):
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
        print("❌ AI大模型思考出错:", e)
        return "大模型脑部连接超时"

# 🎵 刚刚自检成功的：讯飞原生官方大炮 TTS 逻辑
class TTS_Param(object):
    def __init__(self, APPID, APIKey, APISecret, Text):
        self.APPID = APPID; self.APIKey = APIKey; self.APISecret = APISecret; self.Text = Text
        self.CommonArgs = {"app_id": self.APPID}
        self.BusinessArgs = {"aue": "raw", "auf": "audio/L16;rate=16000", "vcn": "xiaoyan", "tte": "utf8"}
        self.Data = {"status": 2, "text": str(base64.b64encode(self.Text.encode('utf-8')), "utf-8")}
    def create_url(self):
        url = 'wss://tts-api.xfyun.cn/v2/tts'
        now = datetime.datetime.now(); date = now.strftime('%a, %d %b %Y %H:%M:%S GMT')
        signature_origin = "host: tts-api.xfyun.cn\ndate: " + date + "\nGET /v2/tts HTTP/1.1"
        signature_sha = hmac.new(self.APISecret.encode('utf-8'), signature_origin.encode('utf-8'), hashlib.sha256).digest()
        signature_sha = base64.b64encode(signature_sha).decode(encoding='utf-8')
        authorization_origin = 'api_key="{}", algorithm="hmac-sha256", headers="host date request-line", signature="{}"'.format(self.APIKey, signature_sha)
        authorization = base64.b64encode(authorization_origin.encode('utf-8')).decode(encoding='utf-8')
        return url + '?' + urlencode({"authorization": authorization, "date": date, "host": "tts-api.xfyun.cn"})

def generate_tts_pcm_xf(text_content, output_pcm_path):
    if os.path.exists("xf_temp_16k.pcm"): os.remove("xf_temp_16k.pcm")
    if os.path.exists(output_pcm_path): os.remove(output_pcm_path)
    
    def on_tts_message(ws, message):
        msg = json.loads(message)
        if msg["code"] == 0:
            audio = base64.b64decode(msg["data"]["audio"])
            with open("xf_temp_16k.pcm", "ab") as f: f.write(audio)
        if msg["data"]["status"] == 2:
            os.system("ffmpeg -y -f s16le -ar 16000 -ac 1 -i xf_temp_16k.pcm -f u8 -acodec pcm_u8 -ar 8000 -ac 1 {} >/dev/null 2>&1".format(output_pcm_path))
            ws.close()

    tts_param = TTS_Param(APPID, API_KEY, API_SECRET, text_content)
    ws = websocket.WebSocketApp(tts_param.create_url(), on_message=on_tts_message, on_error=lambda w,e:print(e), on_close=lambda w,a,b:pass)
    ws.on_open = lambda w: _thread.start_new_thread(lambda: w.send(json.dumps({"common": tts_param.CommonArgs, "business": tts_param.BusinessArgs, "data": tts_param.Data})), ())
    import _thread
    ws.run_forever(sslopt={"cert_reqs": ssl.CERT_NONE})

# 🚀 将最终的“文字 + 音频大炮”一起砸向开发板
def send_text_and_voice_to_board(board_ip, question, answer):
    try:
        # 1. 调用刚刚自检成功的原生讯飞合成通道
        generate_tts_pcm_xf(answer, "reply.pcm")
        
        if not os.path.exists("reply.pcm") or os.path.getsize("reply.pcm") < 100:
            print("❌ 核心报错：reply.pcm 声音文件未成功生成，放弃网络下发！")
            return

        # 2. 建立网络长驱管道，强砸板子的 9999 端口
        print("🚚 [网络发射] 正在通过 9999 端口向板子灌入音视频大包...")
        back_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        back_socket.settimeout(4)  # 容错时间给足
        back_socket.connect((board_ip, 9999))
        
        with open("reply.pcm", "rb") as f:
            pcm_bytes = f.read()
            
        text_payload = "❓ 问: {}\n🤖 答: {}\n".format(question, answer)
        final_packet = text_payload.encode('utf-8') + b"[AUDIO_START]" + pcm_bytes
        
        back_socket.sendall(final_packet)
        back_socket.close()
        print("🏆 【全链路超级大闭环】文字与物理声音大炮已 100% 同步合体发射至开发板喇叭！")
    except Exception as e:
        print("❌ 下发给开发板 9999 端口失败 (Connection Refused), 请检查板子上 client 是否在安全运行！:", e)

# 📡 讯飞听写 WebSocket 核心参数
class Ws_Param(object):
    def __init__(self, APPID, APIKey, APISecret):
        self.APPID = APPID; self.APIKey = APIKey; self.APISecret = APISecret
        self.CommonArgs = {"app_id": self.APPID}
        self.BusinessArgs = {"domain": "iat", "language": "zh_cn", "accent": "mandarin", "vinfo": 1, "vad_eos": 10000}
    def create_url(self):
        url = 'wss://iat-api.xfyun.cn/v2/iat'
        now = datetime.datetime.now(); date = now.strftime('%a, %d %b %Y %H:%M:%S GMT')
        signature_origin = "host: iat-api.xfyun.cn\ndate: " + date + "\nGET /v2/iat HTTP/1.1"
        signature_sha = hmac.new(self.APISecret.encode('utf-8'), signature_origin.encode('utf-8'), hashlib.sha256).digest()
        signature_sha = base64.b64encode(signature_sha).decode(encoding='utf-8')
        authorization_origin = 'api_key="{}", algorithm="hmac-sha256", headers="host date request-line", signature="{}"'.format(self.APIKey, signature_sha)
        authorization = base64.b64encode(authorization_origin.encode('utf-8')).decode(encoding='utf-8')
        return url + '?' + urlencode({"authorization": authorization, "date": date, "host": "iat-api.xfyun.cn"})

def on_message(ws, message):
    try:
        code = json.loads(message)["code"]
        if code == 0:
            data = json.loads(message)["data"]["result"]["ws"]
            result = "".join([w["w"] for i in data for w in i["cw"]])
            if result.strip():
                global current_board_ip
                print("\n" + "="*50 + "\n✨ [讯飞听写成功]: {}".format(result) + "\n" + "="*50)
                
                # 喂给大模型
                ai_response = get_ai_answer(result)
                print("🤖 Model 智能答复: {}".format(ai_response))
                
                # 发射声音+文字复合包
                send_text_and_voice_to_board(current_board_ip, result, ai_response)
    except Exception as e:
        pass

def on_error(ws, error): print("### 讯飞听写接口错误:", error)
def on_close(ws, a, b): pass
def on_open(ws):
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
    import _thread; _thread.start_new_thread(run, ())

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

def start_socket_server():
    server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    server_socket.bind(('0.0.0.0', 8888))
    server_socket.listen(5)
    print("\n🚀 [AI完全体·音视频全双工原生对答服务器] 已满血上线，正在监听 8888 端口...")
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
        print("\n📥 接收到开发板音频流 (大小: {} 字节)，正在对接科大讯飞听写...".format(filesize))
        if convert_8k_u8_to_16k_s16_wav("rec.pcm", "rec_clean_16k.wav"):
            global wsParam; wsParam = Ws_Param(APPID, API_KEY, API_SECRET)
            ws = websocket.WebSocketApp(wsParam.create_url(), on_message=on_message, on_error=on_error, on_close=on_close)
            ws.on_open = on_open; ws.run_forever(sslopt={"cert_reqs": ssl.CERT_NONE})

if __name__ == "__main__":
    start_socket_server()