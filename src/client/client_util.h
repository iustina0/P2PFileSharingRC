#ifndef CLIENT_UTIL_H
#define CLIENT_UTIL_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/resource.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <signal.h>
#include <netdb.h>
#include <pthread.h>
#include <assert.h>
#include <stdio_ext.h>
#include <stdbool.h>
#include <dirent.h>
#include <zlib.h>
#include <openssl/aes.h>
#include <openssl/rand.h>

#define BUFF_SIZE 1024
#define IP_server "192.168.138.42"
#define port 4444
#define PORT_d 2123
#define ENCRYPTION_KEY_SIZE 32
unsigned char encryption_key[ENCRYPTION_KEY_SIZE];

#define max(a, b) ((a) > (b) ? (a) : (b))

struct FileDetails {
    char nume[BUFF_SIZE];
    int tip;
};

extern struct FileDetails fisiere_detinute[20];

void get_files_dir(char *dir, int y);
void search_for_file(char *request, int server);
void recive_process(char *file_name, int server);
void look_for_download(int server, char *request);
void transfer_procces(char *file_name);
void generate_encryption_key(unsigned char *key);
void aes_encrypt(const unsigned char *plaintext, int plaintext_len, const unsigned char *key, unsigned char *ciphertext);
void aes_decrypt(const unsigned char *ciphertext, int ciphertext_len, const unsigned char *key, unsigned char *plaintext);
int send_encrypted_data(int fd, const unsigned char *data, int data_len, const unsigned char *key);
int recv_encrypted_data(int fd, unsigned char *buffer, int buffer_size, const unsigned char *key);

#endif 
