# TinyFtp
A minimal ftp server in C, a minimal ftp client in Python, and a UDP toy.
This project is an assignment of course Computer Network.

Some experimental features I like:
1. Server and client supports RSA encryption. However, the encryption and decryption is server-client-specific. which means only
**this server** and **this client** can communicate with each other.
2. Client supports brute-and-force multi-thread transfer: log in multiple clients together and each thread receive one part of the file.
