#include "utils.h"

#include <sys/socket.h>
#include <netinet/in.h>

#include <unistd.h>
#include <errno.h>

#include <ctype.h>
#include <string.h>
#include <memory.h>
#include <stdio.h>

void split_command_test()
{
  char message[] = "USER Calcifer";
  char command[16] = "";
  char command_truth[] = "USER";
  char content[128] = "";
  char content_truth[] = "Calcifer";
  split_command(message, command, content);

  // int i = 0;
  // for (i = 0; i < 16; ++i) {
  //   putchar(command[i]);
  // }
  // printf("\n-------------------------------\n");
  // for (i = 0; i < 16; ++i) {
  //   putchar(content[i]);
  // }

  if (strcmp(command, command_truth) != 0) {
    printf("Split command test: wrong command:%s.\n", command);
  } else if (strcmp(content, content_truth) != 0) {
    printf("Split command test: wrong content:%s\n", content);
  } else {
    printf("Split command test passed.\n");
  }
}

void parse_addr_test()
{
  char content[] = "166,111,81,14,215,10";
  char ip[64] = "";
  int port = parse_addr(content, ip);

  if (strcmp(ip, "166.111.81.14") != 0) {
    printf("Parse addr test: wrong ip addr:%s\n", ip);
  } else if (port != 55050) {
    printf("Parse addr test: wrong port:%d\n", port);
  } else {
    printf("Parse addr test passed.\n");
  }
}

void command_list_test()
{
  FILE *fp;
  fp = popen("ls -l", "r");
  char buffer[256];
  while (fgets(buffer, 256, fp)) {
    printf("%s", buffer);
  }
  pclose(fp); 
}

void crypt_test()
{
  printf("crypt_test...\n");
  int bytes;
  char* encoded;
  char* decoded;
  bignum* pub_exp;
  bignum* pub_mod;
  bignum* priv_exp;
  bignum* priv_mod;
  char* hello = (char*)malloc(32);
  strcpy(hello, "Hello, world!");
  gen_rsa_key(&pub_exp, &pub_mod, &priv_exp, &priv_mod, &bytes);
  // encodeString(hello, &encoded, priv_exp, priv_mod);
  // decodeString(encoded, &decoded, pub_exp, pub_mod);
  encodeString(hello, &encoded, pub_exp, pub_mod);
  decodeString(encoded, &decoded, priv_exp, priv_mod);
  
  printf("Decoded result:\n");
  printf("%s\n", decoded);

  free(hello);
}

void bignum_length_test()
{
  int bytes = 94;
  int len = 940;
  char str[940];
  int i;
  for (i = 0; i < len; ++i) {
    str[i] = (rand() % 26) + 'A';
  }
  str[len - 1] = '\0';

  bignum* pub_exp;
  bignum* pub_mod;
  bignum* priv_exp;
  bignum* priv_mod;
  printf("Generating RSA key...\n");
  gen_rsa_key(&pub_exp, &pub_mod, &priv_exp, &priv_mod, &bytes);
  printf("Finished RSA key genration.\n");

  bignum* encoded = encodeMessage(940, bytes, str, pub_exp, pub_mod);
  printf("finish encoding\n");

  int pck_num = 10;
  for (i = 0; i < pck_num; ++i) {
    printf("%d\n", (encoded + i)->length);
  }
}

void crypt_to_bytes_test()
{
  printf("crypt_test...\n");
  int bytes;
  char* encoded;
  char* decoded;
  bignum* pub_exp;
  bignum* pub_mod;
  bignum* priv_exp;
  bignum* priv_mod;

  char hello[256];
  int len = 256;
  int i;
  for (i = 0; i < len; ++i) {
    hello[i] = (char)(rand() % 26) + 'A';
  }
  hello[len - 1] = '0';
  printf("============================ origin ============================\n");
  printf("%s", hello);
  printf("================================================================\n");

  printf("genrating rsa key...\n");
  gen_rsa_key(&pub_exp, &pub_mod, &priv_exp, &priv_mod, &bytes);
  printf("encoding...\n");
  encoded = encodeBytes(hello, len, bytes, pub_exp, pub_mod);

  int pck_num = (len + BLOCK_SIZE - 1) / BLOCK_SIZE;
  int encoded_len = pck_num * BLOCK_LENGTH * (sizeof(word) / sizeof(char));
  printf("encoded length: %d\n", encoded_len);

  printf("decoding...\n");
  decoded = decodeBytes(encoded, encoded_len, bytes, priv_exp, priv_mod);
  printf("Decoded result:\n");
  printf("%s\n", decoded);

  free(encoded);
  free(decoded);
}

int main()
{
  printf("================================================================\n");
  crypt_to_bytes_test();
  // bignum_length_test();
  // crypt_test();
  // command_list_test();
  // split_command_test();
  // parse_addr_test();
  return 0;
}

