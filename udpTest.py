import socket
HOST = ''
PORT = 2200
    # 创建socket实例对象
s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
s.bind((HOST, PORT))                    # 绑定本地端口号50007
while True:                                # 循环读取数据
    data, sender = s.recvfrom(1024)        # 接收数据
    print('Got Data From', sender)                # 显示发送者的信息
    s_data = data.decode("utf-8")                # 解码
    print("Recv [%s] from " % s_data, sender)    # 显示数据内容
    s.sendto(data.replace(b'\n',b'')+b'_received\n', sender)                # 将接收到的数据送回给发送者
    if s_data == "quit":                        # 如果是quit，那么退出
        print("Got quit message, Echo Server Quit")
        break
s.close()                                        # 关闭socket接口
