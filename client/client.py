import socket
import re
import logging

class Client(object):
  """ftp client"""
  def __init__(self):
    self.hip = None
    self.hport = None
    self.sock = socket.socket()
    self.buf_size = 8192
    self.logged = False
    self.mode = 'pasv'

  def extract_addr(self, string):
    ip = None
    port = None

    result = re.findall(r"\b(?:(?:25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\,){5}(?:25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\b", string)
    if len(result) != 1:
      print('client failed to find valid address in %s', string)
      return ip, port

    addr = result[0].replace(',', '.')

    # split ip addr and port number
    idx = -1
    for i in range(4):
      idx = addr.find('.', idx + 1)

    ip = addr[:idx]
    port = addr[idx + 1:]

    idx = port.find('.')
    p1 = int(port[:idx])
    p2 = int(port[idx + 1:])
    port = int(p1 * 256 + p2)

    return ip, port

  def send(self, msg):
    self.sock.sendall(bytes(msg + '\r\n', encoding='ascii'))

  def recv(self):
    res = self.sock.recv(self.buf_size).decode('ascii').strip()
    code = int(res.split()[0])
    return code, res

  def xchg(self, msg):
    """exchange message: send server the msg and return response"""
    # try:
    self.send(msg)
    code, res = self.recv()
    # except Exception as e:
    #   print('Error in Client.xchg' + str(e))
    return code, res

  def pasv(self):
    code, res = self.xchg('PASV')
    print(res)
    ip = None
    port = None
    if code == 227:
      ip, port = self.extract_addr(res)

    return ip, port

  def data_connect(self, msg):
    ip = None
    port = None
    if self.mode == 'pasv':
      ip, port = self.pasv()
    elif self.mode == 'port':
      pass
    else:
      print('Error in Client.data_connect: illegal mode')

    # code, res = self.xchg(msg)
    # code = 150
    self.send(msg)
    data_sock = None
    if ip and port:
      data_sock = socket.socket()
      data_sock.connect((ip, port))
      code, res = self.recv()
      print(res)
      if (code != 150):
        data_sock.close()
        data_sock = None;
    else:
      print('Error in Client.data_connect: no ip or port')
    
    return data_sock

  def command_open(self, arg):
    self.hip = arg[0]
    self.hport = 21
    if len(arg) > 1:
      self.hport = int(arg[1])
    if self.logged:
      print('Error: you are connected, please close first.')
      return
    self.sock.connect((self.hip, self.hport))
    code, res = self.recv()
    print(res)

    # self.send('SYST')
    # res = self.recv()
    # print('Server system: %s' % res)

    if code == 220: # success connect
      uname = input('username: ')
      code, res = self.xchg('USER ' + uname)
      if code == 331: # ask for password
        pwd = input('password: ')
        code, res = self.xchg('PASS ' + pwd)
        if code // 100 == 2: # login success
          print('login successful as %s' % uname)
          self.logged = True
          code, res = self.xchg('TYPE I')
          if code == 200: # use binay
            print('using binary.')
          else:
            # but we'll still use binary =)
            print('server refused using binary.')
        else:
          print('login failed')
      else:
        print('login failed')
    else:
      print('connection fail due to server')

  def command_recv(self, arg):
    arg = ''.join(arg)
    data_sock = self.data_connect('RETR ' + arg)
    if data_sock:
      with open(arg, 'wb') as f:
        data = data_sock.recv(self.buf_size)
        while data:
          f.write(data)
          data = data_sock.recv(self.buf_size)
      data_sock.close()
      code, res = self.recv()
      print(res)
    else:
      print('Error in Client.command_recv: no data_sock')

  def command_send(self, arg):
    arg = ''.join(arg)
    data_sock = self.data_connect('STOR ' + arg)
    if data_sock:
      with open(arg, 'rb') as f:
        data_sock.sendall(f.read())
        # data = f.read(self.buf_size)
        # while data:
        #   data_sock.sendall(data)
        #   data = f.read(self.buf_size)
      data_sock.close()
      code, res = self.recv()
      print(res)
    else:
      print('Error in Client.command_send: no data_sock')

  def command_ls(self, arg):
    arg = ''.join(arg)
    if len(arg) == 0:
      arg = './'
    else:
      arg = arg[0]
    data_sock = self.data_connect('LIST ' + arg)
    if data_sock:
      data = ""
      packet = data_sock.recv(self.buf_size)
      while packet:
        data += packet.decode('ascii').strip()
        packet = data_sock.recv(self.buf_size)
      print(data)
      code, res = self.recv()
      print(res)
      data_sock.close()
    else:
      print('Error in Client.command_ls: no data_sock')

  def command_help(self, arg):
    print('supported commands:')
    for attr in dir(self):
      if 'command_' in attr:
        print(attr[len('command_'): ])

  def command_close(self, arg):
    code, res = self.xchg('QUIT')
    print(res)
    self.logged= False

  def command_bye(self, arg):
    print('bye')
    return True

  def run(self):
    flag = None
    while not flag:
      cmd = input('ftp > ').split()
      arg = cmd[1:]
      cmd = cmd[0]
      try:
        flag = getattr(self, "command_%s" % cmd)(arg)
      except:
        print('invalid command')


    
if __name__ == '__main__':
  client = Client()
  client.run()



