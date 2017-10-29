import socket
import logging

class Client(object):
  """ftp client"""
  def __init__(self):
    self.hip = None
    self.hport = None
    self.sock = socket.socket()
    self.buf_size = 8192
    self.logged = False
    self.commands = [
      'open'
    ]

  def send(self, data):
    # self.sock.sendall(bytes(data, encoding='ascii'))
    self.sock.send(bytes(data + '\r\n', encoding='ascii'))

  def recv(self):
    res = self.sock.recv(self.buf_size).decode('ascii').strip()
    return res

  def xchg(self, msg):
    """exchange message: send server the msg and return response"""
    # try:
    self.send(msg)
    res = self.recv()
    # except Exception as e:
    #   print('Error in Client.xchg' + str(e))
    code = int(res.split()[0])
    return code, res


  def command_open(self, arg):
    self.hip = arg[0]
    self.hport = 21
    if len(arg) > 1:
      self.hport = int(arg[1])
    self.sock.connect((self.hip, self.hport))
    res = self.recv()
    print(res)

    # self.send('SYST')
    # res = self.recv()
    # print('Server system: %s' % res)

    code = int(res.split()[0])
    if code == 220: # success connect
      uname = input('username: ')
      code, res = self.xchg('USER ' + uname)
      if code == 331: # ask for password
        pwd = input('password: ')
        code, res = self.xchg('PASS ' + pwd)
        if code == 230: # login success
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

  def command_help(self, arg):
    print('Supported commands:')
    for cmd in self.commands:
      print(cmd)

  def run(self):
    while True:
      cmd = input('ftp > ').split()
      arg = cmd[1:]
      cmd = cmd[0]
      try:
        getattr(self, "command_%s" % cmd)(arg)
      except Exception as e:
        print(str(e))


    
if __name__ == '__main__':
  client = Client()
  client.run()



