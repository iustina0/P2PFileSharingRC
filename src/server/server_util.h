#ifndef SERVER_UTIL_H
#define SERVER_UTIL_H

#include <netinet/in.h>

extern int errno;
#define MAX_CLIENT 10
#define BUFF_SIZE 1024

int check_if_busy(int fd);
int check_if_busy_pid(pid_t pid);
void mark_busy(int fd, pid_t pid);
void unmark_busy(int fd);
char *conv_addr(struct sockaddr_in address);
void split_command_process(char *request_command, int desc_nr, int request_client, int status[]);
void search_for_file(char *req_comm, int desc_nr, int req_client, int status[]);
void make_transfer_conn(char *comm, int reciver, int status[]);
int sayHello(int fd);

#endif 
