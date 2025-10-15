import click
import select
import signal
import socket
import struct
import sys
import subprocess

from fcntl import ioctl
from pyroute2 import IPRoute


# VPN UDP port
vpn_port = 10101

# Device name for the tunnel interface.
device_name = "tun0"

# https://jvns.ca/blog/2022/09/06/send-network-packets-python-tun-tap/#how-to-connect-to-the-tun-interface-in-python
def open_tun_interface(device_name: str = "tun0"):

    name_bytes = device_name.encode()

    assert len(name_bytes) < 16, "The interface name must be less than 16 bytes"

    tuntap = open("/dev/net/tun", "r+b", buffering=0)

    LINUX_IFF_TUN = 0x0001
    LINUX_IFF_NO_PI = 0x1000
    flags = LINUX_IFF_TUN | LINUX_IFF_NO_PI
    ifs = struct.pack("16sH22s", name_bytes, flags, b"")

    LINUX_TUNSETIFF = 0x400454CA
    r = ioctl(tuntap, LINUX_TUNSETIFF, ifs)
    print("Open TUN interface for read/write")
    return tuntap


def create_and_configure_tun_interface(device_name: str, local_vpn_ip: str, remote_vpn_ip: str):

    ipr = IPRoute()

    # create tun interface with Netlink
    ipr.link("add",ifname=device_name, kind="tuntap", mode="tun")
    
    # lookup tun interface by name
    ifidx = ipr.link_lookup(ifname=device_name)[0]
    
    # add ip to tun interface
    ipr.addr("add", index=ifidx, address=local_vpn_ip)

    # Adjust MTU to 1500 - 20 (IP hdr len) - 8 (UDP hdr len)
    ipr.link("set", index=ifidx, mtu=1472)

    # bring it up
    ipr.link('set', index=ifidx, state='up')

    # add route
    ipr.route("add", dst=remote_vpn_ip, mask=32, gateway=local_vpn_ip)
    ipr.close()

    print ("Create TUN interface")


def delete_tun_interface(device_name: str):

    ipr = IPRoute()
    ipr.link("delete", ifname=device_name)
    ipr.close()


def signal_handler(sig, frame):

    print('Pressed Ctrl+C! Exiting...')
    delete_tun_interface(device_name)
    sys.exit(0)


@click.command()
@click.option("-s", "--server-host")
@click.argument("local-vpn-ip")
@click.argument("remote-vpn-ip")
def start_vpn(
        server_host,
        local_vpn_ip,
        remote_vpn_ip
        ):

    create_and_configure_tun_interface(device_name, local_vpn_ip, remote_vpn_ip)
    tun_fd = open_tun_interface(device_name)
    
    vpn_socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    
    # If server host defines -> we're client.
    # If server host is None -> we're server.
    if server_host:
        vpn_socket.connect((server_host, vpn_port))

        # Send init message
        vpn_socket.send("init_dummy_vpn".encode('utf-8'))
        print("Send init message")
        data = vpn_socket.recv(0xFFFF)

        if data == b"OK":
            print("VPN connection is established")
        else:
            print(f"Bad VPN init answer - {data}. Exiting...")
            delete_tun_interface(device_name)
            sys.exit(1)
    
    else:
        vpn_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        vpn_socket.bind(('0.0.0.0', vpn_port))

        while True:
            data, (ip, port) = vpn_socket.recvfrom(1024)
            print (f"Got {data}")

            if data == b"init_dummy_vpn":
                vpn_socket.connect((ip, port))
                vpn_socket.send("OK".encode('utf-8'))
                print(f"Connection is established with {ip}:{port}")
                break

    # Main loop
    print ("VPN is ready")
    while True:

        rd_sockets, _, _ = select.select([tun_fd, vpn_socket], [], [], 1.0)

        for sock in rd_sockets:

            # If data from tun interface -> send to VPN
            if sock is tun_fd:
                data = tun_fd.read(0xFFFF)
                vpn_socket.send(data)

            # If data from VPN socket -> send to TUN
            elif sock is vpn_socket:
                data = vpn_socket.recv(0xFFFF)

                if data:
                    tun_fd.write(data)


    delete_tun_interface(device_name)


if __name__ == "__main__":
    
    signal.signal(signal.SIGINT, signal_handler)
    start_vpn()

