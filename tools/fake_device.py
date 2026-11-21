import math
import random
import socket
import struct
import sys
import threading
import time

HOST = "127.0.0.1"
PORT = 1502
SERVER_PORT = 1503

state = {"temp": 280.0, "current": 460.0, "voltage": 2200.0, "status": 1}


def make_modbus_response(req):
    """功能：把读保持寄存器请求翻译成一帧 Modbus TCP 应答。"""
    if len(req) < 12:
        return None
    tid, pid, length, unit, func = struct.unpack(">HHHBB", req[:8])
    if pid != 0 or length != 6 or func != 3:
        return None
    _start, count = struct.unpack(">HH", req[8:12])
    count = min(max(count, 1), 16)
    tick = time.monotonic()                       # tick：单调递增的秒数，做缓慢周期
    target = 310 + 40 * math.sin(tick / 9.0)     # target：27~35 °C 的正弦趋势（×10）
    state["temp"] += (target - state["temp"]) * 0.35 + random.uniform(-1.5, 1.5)
    if state["temp"] < 250: state["temp"] = 250
    if state["temp"] > 360: state["temp"] = 360
    state["current"] += random.uniform(-1.5, 1.5)
    if state["current"] < 380: state["current"] = 380
    if state["current"] > 620: state["current"] = 620
    state["voltage"] += random.uniform(-3.0, 3.0)
    if state["voltage"] < 2150: state["voltage"] = 2150
    if state["voltage"] > 2250: state["voltage"] = 2250
    if random.random() < 0.04:                    # status：小概率翻转，模拟设备状态变化
        state["status"] = 1 - state["status"]

    base = [int(state["temp"]), int(state["current"]),
            int(state["voltage"]), state["status"]]  # base：温度/电流/电压/状态
    regs = [base[i % 4] for i in range(count)]
    body = struct.pack(">BBB", unit, func, count * 2) + struct.pack(">" + "H" * count, *regs)
    return struct.pack(">HHH", tid, 0, len(body)) + body


def split_frames(cache):
    """功能：按 MBAP 长度字段把字节流切成整帧。"""
    frames = []  # frames：切出来的整帧
    while len(cache) >= 6:
        total = (cache[4] << 8) + cache[5] + 6  # total：整帧长度
        if len(cache) < total:
            break
        frames.append(cache[:total])
        cache = cache[total:]
    return frames, cache


def reply_for(frame):
    """功能：按请求内容决定回 Modbus 帧还是一行文本。"""
    modbus = make_modbus_response(frame)
    if modbus:
        return modbus
    return ("TEMP=%.1f;CUR=%.1f;VOLT=%.1f\n"
            % (random.uniform(20, 30), random.uniform(40, 60), random.uniform(215, 225))).encode()


def serve_tcp_client(conn, addr):
    """功能：处理一个上位机连接，收一帧回一帧。"""
    cache = b""  # cache：没凑齐一帧的残留字节
    print("TCP 上位机连入", addr, flush=True)
    try:
        while True:
            data = conn.recv(4096)
            if not data:
                break
            print("TCP 收到", data.hex(" "), flush=True)
            cache += data
            if len(cache) >= 4 and cache[2] == 0 and cache[3] == 0:
                # 前四字节像 MBAP 头：按长度字段切，半包留着等下一批
                frames, cache = split_frames(cache)
            else:
                # 普通文本：一次 recv 就当一帧
                frames, cache = [cache], b""
            for frame in frames:
                response = reply_for(frame)
                conn.sendall(response)
                print("TCP 回复", response.hex(" "), flush=True)
    except OSError:
        pass
    finally:
        conn.close()
        print("TCP 上位机断开", addr, flush=True)


def tcp_server_loop():
    """功能：监听 1502，等 TCP 客户端模式的上位机连进来。"""
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    sock.bind((HOST, PORT))
    sock.listen(5)
    print("TCP 假设备就绪 127.0.0.1:%d（上位机选 TCP 客户端）" % PORT, flush=True)
    while True:
        conn, addr = sock.accept()
        threading.Thread(target=serve_tcp_client, args=(conn, addr), daemon=True).start()


def udp_loop():
    """功能：监听 1502 的 UDP 数据报，收到就回一行文本。"""
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    sock.bind((HOST, PORT))
    print("UDP 假设备就绪 127.0.0.1:%d（上位机选 UDP，目标端口填 %d）"
          % (PORT, PORT), flush=True)
    while True:
        data, addr = sock.recvfrom(4096)
        print("UDP 收到", addr, data.hex(" "), flush=True)
        response = reply_for(data)
        sock.sendto(response, addr)
        print("UDP 回复", response.hex(" "), flush=True)


def tcp_reverse_loop():
    """功能：反复尝试连上位机的 TCP 服务端，连上后收什么回什么。"""
    while True:
        try:
            sock = socket.create_connection((HOST, SERVER_PORT), timeout=2)
            print("已连上上位机 TCP 服务端 127.0.0.1:%d" % SERVER_PORT, flush=True)
            while True:
                data = sock.recv(4096)
                if not data:
                    break
                print("服务端模式收到", data.hex(" "), flush=True)
                response = reply_for(data)
                sock.sendall(response)
                print("服务端模式回复", response.hex(" "), flush=True)
            sock.close()
        except OSError:
            pass
        time.sleep(0.2)


def main():
    """功能：同时起 TCP 假设备、UDP 假设备，并尝试反向连上位机服务端。"""
    print("假设备启动，Ctrl+C 退出", flush=True)
    threading.Thread(target=tcp_server_loop, daemon=True).start()
    threading.Thread(target=udp_loop, daemon=True).start()
    threading.Thread(target=tcp_reverse_loop, daemon=True).start()
    try:
        while True:
            time.sleep(1)
    except KeyboardInterrupt:
        print("假设备已退出", flush=True)


if __name__ == "__main__":
    sys.exit(main())
