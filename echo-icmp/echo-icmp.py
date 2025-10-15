import socket
import struct
import sys

try:
    s = socket.socket(socket.AF_INET, socket.SOCK_RAW, socket.IPPROTO_ICMP)
except socket.error as msg:
    print("Socket could not be created. Error Code : " + str(msg))
    sys.exit(-1)

try:
    ICMP_ECHO = 8
    checksum = 0x7d95
    identifier = 31337
    seq_number = 1

    # Create ICMP header
    packet = struct.pack("!BBHHH", ICMP_ECHO, 0, checksum, identifier, seq_number)

    # Send ICMP Echo
    s.sendto(packet, ("77.88.8.8", 1)) # Port number is irrelevant for ICMP

    # Recv ICMP Echo-reply
    packet = s.recvfrom(0xFFFF)

    packet_data = packet[0]
    address = packet[1]

    print(f"Захвачен ICMP пакет длиной {len(packet_data)} байт от {address}")
    print(packet_data)

except KeyboardInterrupt:
    print("\nОстановка захвата пакетов")
    s.close()
