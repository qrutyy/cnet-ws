# в локальной сети был сервер, который раз в 2 сек открывался на случайном порту и бродкастом отправлял свой порт.
# после того, как на его порт приходило сообщение, в ответ он кидал скрытое сообщение.
# нужно было получить скрытое сообщение.

import re
import socket
from scapy.all import IP, UDP, AsyncSniffer

SERVER_IP = None
SERVER_PORT = None
FOUND = False


def is_broadcast(ip):
    return ip == "255.255.255.255" or ip.endswith(".255")


def packet_handler(pkt):
    global SERVER_IP, SERVER_PORT, FOUND
    if IP in pkt and UDP in pkt:
        dst_ip = pkt[IP].dst
        src_ip = pkt[IP].src
        if is_broadcast(dst_ip):
            try:
                payload = bytes(pkt[UDP].payload)
                msg = payload.decode("utf-8", errors="ignore").strip()
                print(f"[+] Broadcast от {src_ip}: {msg}")

                port_match = re.search(r"\b(\d{4,5})\b", msg)
                if port_match:
                    port = int(port_match.group(1))
                    if 1024 <= port <= 65535:
                        SERVER_IP = src_ip
                        SERVER_PORT = port
                        FOUND = True
                        print(f"[*] Найден сервер: {SERVER_IP}:{SERVER_PORT}")
            except Exception as e:
                pass


def listen_broadcast_with_scapy():
    print("[*] Сниффинг всех UDP-пакетов в поисках broadcast...")
    sniffer = AsyncSniffer(filter="udp", prn=packet_handler, store=False)
    sniffer.start()

    try:
        while not FOUND:
            pass
    except KeyboardInterrupt:
        pass
    finally:
        sniffer.stop()

    return SERVER_IP, SERVER_PORT


def send_and_get_flag(server_ip, server_port):
    try:
        sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        sock.settimeout(2)
        sock.sendto(b"ping", (server_ip, server_port))
        data, _ = sock.recvfrom(1024)
        flag = data.decode().strip()
        print(f"Флаг: {flag}")
        return flag
    except Exception as e:
        print(f"Не удалось получить флаг: {e}")
        return None


if __name__ == "main":
    ip, port = listen_broadcast_with_scapy()
    if ip and port:
        send_and_get_flag(ip, port)

