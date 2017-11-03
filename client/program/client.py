import socket
import re
import os
import random
import time
import ctypes
import getpass
import threading
import traceback
import math

# used for multi-thread receiving
class DataBlock(object):

  def __init__(self, idx):
    self.idx = idx
    self.data = bytes()
    
class Client(object):
  """ftp client"""
  def __init__(self):
    self.hip = None # server ip
    self.hport = None
    self.sock = None
    self.buf_size = 8192
    self.logged = False
    self.lip = None # local ip
    self.mode = 'pasv'
    self.append = False
    self.encrypt = False
    self.rsalib = ctypes.CDLL('./librsa.so')
    self.rsalib.decodeBytesChar.restype = ctypes.c_char_p
    self.rsalib.encodeBytesChar.restype = ctypes.c_char_p
    self.pub_exp = None
    self.pub_mod = None
    self.blocksize = 84 # magic number related to server
    self.bts = 82 # magic number related to server
    self.uname = ''
    self.pwd = ''
    self.thread_num = 1
    self.running = True

  def zero_padding(self, data):
    current = len(data)
    target = int(math.ceil(current / self.blocksize) * self.blocksize)
    pad = "\0" * (target - current)
    data += bytes(pad, encoding='ascii')
    return data

  def decode(self, msg):
    decoded_length = math.ceil((len(msg) / self.blocksize)) * self.bts
    # print('decoded length should be %d bytes' % decoded_length)
    buf = (ctypes.c_byte * decoded_length)()
    self.rsalib.decodeBytesChar(
      msg,
      ctypes.c_int(len(msg)),
      ctypes.c_int(self.bts),
      buf,
      bytes(self.pub_exp, encoding='ascii'),
      bytes(self.pub_mod, encoding='ascii')
    )
    ret = bytearray(buf)
    return ret

  def encode(self, msg):
    encoded_length = math.ceil((len(msg) / self.bts)) * self.blocksize
    buf = (ctypes.c_byte * encoded_length)()
    self.rsalib.encodeBytesChar(
      msg,
      ctypes.c_int(len(msg)),
      ctypes.c_int(self.bts),
      buf,
      bytes(self.pub_exp, encoding='ascii'),
      bytes(self.pub_mod, encoding='ascii')
    )
    ret = bytearray(buf)
    return ret;

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

  def extract_rl(self, arg):
    remote = arg[0]
    local = None
    if len(arg) == 2:
      local = arg[1]
    else:
      local = arg[0]
    print('remote: %s local: %s' % (remote, local))
    return remote, local

  def recv_block_decode(self, sock):
    read = 0
    data = sock.recv(self.buf_size)
    if not data:
      return None
    read += len(data)
    while read % self.blocksize != 0:
      new_read = sock.recv(self.buf_size)
      if not new_read:
        return None
      data += new_read
      read += len(data)
    # print('start decoding %d bytes...' % read)
    return self.decode(data)

  def send(self, msg, cmdsk=None):
    if not cmdsk:
      cmdsk = self.sock
    msg = msg.strip() + '\r\n'
    if self.encrypt:
      msg = self.encode(bytes(msg, encoding='ascii'))
    else:
      msg = bytes(msg, encoding='ascii')
    cmdsk.sendall(msg)

  def recv(self, cmdsk=None):
    if not cmdsk:
      cmdsk = self.sock
    res = None
    if self.encrypt:
      res = self.recv_block_decode(cmdsk)
    else:
      res = cmdsk.recv(self.buf_size)
    res = res.decode('ascii').strip()
    code = int(res[0]) # only first number of the code is concerned
    return code, res

  def xchg(self, msg, cmdsk=None):
    if not cmdsk:
      cmdsk = self.sock
    self.send(msg, cmdsk)
    code, res = self.recv(cmdsk)
    return code, res

  def pasv(self, cmdsk=None):
    if not cmdsk:
      cmdsk = self.cmdsk
    code, res = self.xchg('PASV', cmdsk)
    print(res)
    ip = None
    port = None
    if code == 2:
      ip, port = self.extract_addr(res)
    return ip, port

  def port(self, cmdsk=None):
    if not cmdsk:
      cmdsk = self.cmdsk
    lport = random.randint(20000, 65535)
    p1 = lport // 256
    p2 = lport % 256
    ip = self.lip.replace('.', ',')
    code, res = self.xchg('PORT %s,%d,%d' % (ip, p1, p2), cmdsk)
    print(res)
    if code == 2:
      lstn_sock = socket.socket()
      lstn_sock.bind(('', lport))
      lstn_sock.listen(10)
      return lstn_sock
    else:
      return None

  def data_connect(self, msg, verbose=True, cmdsk=None):
    data_sock = None
    if not cmdsk:
      cmdsk = self.sock
    if self.mode == 'pasv':
      ip, port = self.pasv(cmdsk)
      self.send(msg, cmdsk)
      if ip and port:
        data_sock = socket.socket()
        try:
          data_sock.connect((ip, port))
        except Exception as e:
          print(str(e))
          data_sock.close()
          return None
        code, res = self.recv(cmdsk)
        if verbose:
          print(res)
        if code != 1:
          data_sock.close()
          return None
      else:
        print('Error in Client.data_connect: no ip or port')
    elif self.mode == 'port':
      lstn_sock = self.port(cmdsk)
      self.send(msg, cmdsk)
      if lstn_sock:
        data_sock, _ = lstn_sock.accept()
        lstn_sock.close()
        code, res = self.recv(cmdsk)
        if verbose:
          print(res)
      else:
        print('Error in Client.data_connect: no lstn_sock')
    else:
      print('Error in Client.data_connect: illegal mode')
    return data_sock

  def login(self):
    cmdsk = socket.socket()
    print('connecting %s %d' % (self.hip, self.hport))
    cmdsk.connect((self.hip, self.hport))
    code, res = self.recv(cmdsk)
    if code == 2: # success connect
      code, res = self.xchg('USER ' + self.uname, cmdsk)
      if code == 3: # ask for password
        code, res = self.xchg('PASS ' + self.pwd, cmdsk)
        if code == 2: # login success
          code, res = self.xchg('TYPE I', cmdsk)
        else:
          print('login failed')
      else:
        print('login failed')
    else:
      print('connection failed due to server')
    return cmdsk

  def recv_thread(self, path, offset, blocksize, dblock):
    idx = dblock.idx
    print('Thread %d connecting...' % idx)
    cmdsk = self.login()
    if not cmdsk:
      print('Thread %d login failed...')
      return
    code, res = self.xchg('REST %d' % offset, cmdsk)
    if code != 2 and code != 3:
      print('Thread %d set REST failed...' % idx)
      print('server response: %s' % res)
      dblock.data = None
      return
    print('Thread %d requesting...' % idx)
    datask = self.data_connect('RETR ' + path, verbose=False, cmdsk=cmdsk)
    if not datask:
      print('Thread %d RETR failed...' % idx)
      dblock.data = None
      return
    remained = blocksize
    dblock.data = bytes()
    print('Thread %d reading...' % idx)
    while len(dblock.data) < blocksize:
      dblock.data += datask.recv(remained)
    print('Thread %d exiting...' % idx)
    datask.close()
    cmdsk.close()

  def command_open(self, arg):
    self.hip = arg[0]
    self.hport = 21
    if len(arg) > 1:
      self.hport = int(arg[1])
    if self.logged:
      print('Error: you are connected, please close first.')
      return
    self.sock = socket.socket()
    self.sock.connect((self.hip, self.hport))
    code, res = self.recv()
    print(res)

    # self.send('SYST')
    # res = self.recv()
    # print('Server system: %s' % res)

    if code == 2: # success connect
      self.uname = input('username: ')
      code, res = self.xchg('USER ' + self.uname)
      if code == 3: # ask for password
        self.pwd = getpass.getpass('password: ')
        code, res = self.xchg('PASS ' + self.pwd)
        if code == 2: # login success
          print('login successfully as %s' % self.uname)
          self.logged = True
          code, res = self.xchg('TYPE I')
          if code == 2: # use binay
            print('using binary.')
          else:
            print('server refused using binary.')
        else:
          print(res)
          print('login failed')
      else:
        print(res)
        print('login failed')
    else:
      print('connection fail due to server')

  def command_recv(self, arg):
    remote, local = self.extract_rl(arg)
    code, res = self.xchg('SIZE ' + remote)
    file_size = None
    if code == 2:
      file_size = int(res.split()[1])
    elapse = time.time()
    data_sock = self.data_connect('RETR ' + remote)
    if data_sock:
      f = None
      if self.append:
        f = open(local, 'ab')
        self.append = False
        print('resuming transfer...')
      else:
        f = open(local, 'wb')
      
      if self.encrypt:
        written = 0
        data = self.recv_block_decode(data_sock)
        while data:
          new_recv = len(data)
          to_write = new_recv if new_recv + written <= file_size else file_size - written
          f.write(data[:to_write])
          written += to_write
          # print('written %d bytes...' % to_write)
          data = self.recv_block_decode(data_sock)
      else:
        file_size = 0
        data = data_sock.recv(self.buf_size)
        while data:
          f.write(data)
          file_size += len(data)
          data = data_sock.recv(self.buf_size)

      f.close()
      data_sock.close()
      code, res = self.recv()
      print(res)
      elapse = time.time() - elapse
      print('%dkb in %f seconds, %fkb/s in avg' % (file_size, elapse, file_size/elapse/1e3))
    else:
      print('Error in Client.command_recv: no data_sock')

  def command_multirecv(self, arg):
    if self.encrypt:
      print('multirecv is not supported in encrypt mode')
      return
    remote, local = self.extract_rl(arg)
    elapse = time.time()
    if self.thread_num == 1:
      print('use "thread" command to specify thread number first')
      return
    code, res = self.command_size([remote])

    # got the things between ""
    _, direct = self.command_pwd(None)
    if '"' in direct:
      direct = direct.split('"')[1]
    # direct = "." + direct
    remote = os.path.join(direct, remote)
    if code != 2:
      print('cannot start multi-thread receiving')
      print('server response: %s' % res)
      return
    size = int(res.split()[1])
    interval = size // self.thread_num
    threads = []
    blocks = []
    for i in range(self.thread_num):
      blocks.append(DataBlock(i))
      offset = i * interval
      blocksize = interval if (offset + interval <= size) else (size - offset)
      threads.append(\
        threading.Thread(target=self.recv_thread,
                         args=(remote, offset, blocksize, blocks[i])))
    for t in threads:
      t.start()
    for t in threads:
      t.join()
    blocks = sorted(blocks, key=lambda x: x.idx)
    total = bytes()
    for b in blocks:
      if b.data:
        total += b.data
      else:
        print('Error: data block %d is broken' % b.idx)
        return
    with open(local, 'wb') as f:
      f.write(total)
    elapse = time.time() - elapse
    print('%dkb in %f seconds, %fkb/s in avg' % (size, elapse, size/elapse/1e3))

  def command_send(self, arg):
    remote, local = self.extract_rl(arg)
    elapse = time.time()
    data_sock = self.data_connect('STOR ' + remote)
    if data_sock:
      with open(local, 'rb') as f:
        data = f.read()
        if self.encrypt:
          data = self.encode(data)
        data_sock.sendall(data)
      data_sock.close()
      code, res = self.recv()
      print(res)
      total = os.path.getsize(local)
      elapse = time.time() - elapse
      print('%dkb in %f seconds, %fkb/s in avg' % (total, elapse, total/elapse/1e3))
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
    try:
      code, res = self.xchg('QUIT')
      print(res)
    finally:
      # close anyway
      self.sock.close()
      self.__init__()

  def command_bye(self, arg):
    if self.logged:
      self.command_close('')
    # print('good luck')
    self.running = False

  def command_nlist(self, arg):
    arg = ''.join(arg)
    if len(arg) == 0:
      arg = './'
    else:
      arg = arg[0]
    data_sock = self.data_connect('NLST ' + arg)
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

  def command_mkdir(self, arg):
    arg = ''.join(arg)
    code, res = self.xchg('MKD ' + arg)
    print(res)

  def command_rm(self, arg):
    arg = ''.join(arg)
    code, res = self.xchg('RMD ' + arg)
    print(res)

  def command_cd(self, arg):
    arg = ''.join(arg)
    code, res = self.xchg('CWD ' + arg)
    print(res)

  def command_resume(self, arg):
    try:
      offset = os.path.getsize(''.join(arg))
      code, res = self.xchg('REST %d' % offset)
      if code == 2 or code == 3:
        self.append = True
      else:
        print('server rejected resume')
      self.command_recv(arg)
    except Exception as e:
      print(str(e))

  def command_pasv(self, arg):
    self.mode = 'pasv'
    print('switch to pasv mode')

  def command_port(self, arg):
    self.mode = 'port'
    print('switch to part mode')
    print('ip address %s' % self.lip)

  def command_mult(self, arg):
    code, res = self.xchg('MULT')
    print(res)

  def command_crypt(self, arg):
    if self.encrypt:
      self.send('ENCR')
      self.encrypt = False
      code, res = self.recv()
      print(res)
    else:
      code, res = self.xchg('ENCR')
      # print(res)
      if code == 2:
        self.encrypt = True
        self.pub_exp, self.pub_mod, self.bts = res.split()[1].split(',')
        self.bts = int(self.bts)
        code, res = self.recv()
        print(res)
      else:
        print(res)

  def command_thread(self, arg):
    if len(arg) == 0:
      arg = 4 if self.thread_num == 1 else 1
    else:
      arg = int(arg[0]) if int(arg[0]) > 0 else 0
    if self.thread_num != 1:
      self.thread_num = arg
      print('switched to %d thread(s)' % arg)
      return
    else:
      self.thread_num = arg
    code, res = self.xchg('REST 128')
    if code == 2 or code == 3:
      print('switched to multi-thread mode using %d threads' % self.thread_num)
    else:
      print('the server doesn\'t support multi-thread, back to single')
      self.thread_num = 1

  def command_size(self, arg):
    code, res = self.xchg('SIZE ' + arg[0])
    print(res)
    return code, res

  def command_ext(self, arg):
    code, res = self.xchg(' '.join(arg))
    print(res)

  def command_pwd(self, arg):
    code, res = self.xchg('PWD')
    print(res)
    return code, res

  def command_ip(self, arg):
    self.lip = arg[0]
    print('set local ip to %s' % self.lip)

  def run(self):
    print('ftp client start')
    while self.running:
      cmd = input('ftp > ').split()
      arg = cmd[1:]
      cmd = cmd[0]
      try:
        getattr(self, "command_%s" % cmd)(arg)
      except Exception as e:
        # traceback.print_exc()
        if type(e) == AttributeError:
          print('invalid command')
        else:
          print(str(e))

if __name__ == '__main__':
  client = Client()
  client.run()



