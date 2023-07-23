#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <sys/resource.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <signal.h>
#include <netdb.h>
#include <pthread.h>
#include <assert.h>
#include <stdio_ext.h>
#include <sys/select.h>
#include <stdbool.h>
#include <ctype.h>
#include "server_util.h"



extern int sock_accept, client; 
extern fd_set read_fds, all_fds; 
extern int busy[20]; 

int check_if_busy(int fd) {
    if (busy[fd] == 0)
        return 0;
    if ((waitpid((pid_t)busy[fd], NULL, WNOHANG)) == 0)
        return 1;
    busy[fd] = 0;
    return 0;
}

int check_if_busy_pid(pid_t pid) {
    if ((waitpid(pid, NULL, WNOHANG)) == 0) {
        return 1;
    }
    for (int i = 0; i < MAX_CLIENT; i++) {
        if (busy[i] == (int)pid) {
            busy[i] = 0;
        }
    }
    return 0;
}

void mark_busy(int fd, pid_t pid) {
    if (check_if_busy(fd))
        return;
    busy[fd] = (int)pid;
    return;
}

void unmark_busy(int fd) {
    if (check_if_busy(fd))
        return;
    for (int i = 0; i < MAX_CLIENT; i++) {
        if (busy[i] == fd)
            busy[i] = 0;
    }
    return;
}

char *conv_addr(struct sockaddr_in address) {
    static char str[25];
    char port[7];

    strcpy(str, inet_ntoa(address.sin_addr));
    bzero(port, 7);
    sprintf(port, ":%d", ntohs(address.sin_port));
    strcat(str, port);
    return (str);
}

void split_command_process(char *request_command, int desc_nr, int request_client, int status[]) {
    pid_t child_pid = fork();
    busy[request_client] = child_pid;
    if (child_pid == 0) {
        search_for_file(request_command, desc_nr, request_client, status);
        exit(1);
    } else {
        return;
    }
}


void search_for_file(char *req_comm, int desc_nr, int req_client, int status[]) {
    int hit_count = 0, client_hit = 0;
    int fds;
    int found_n, bytes;
    char sent_comm[BUFF_SIZE], rasp_peer[BUFF_SIZE], msgres[150], raspuns[BUFF_SIZE];
    char *rasp_part;
    bzero(sent_comm, BUFF_SIZE);
    bzero(msgres, 150);
    bzero(raspuns, BUFF_SIZE);
    strcat(sent_comm, req_comm);

    char *file_name = strtok(req_comm, ":");
    file_name = strtok(NULL, ":");
    file_name = strtok(NULL, ":");
    file_name = strtok(file_name, " ");
    char *filter_type = strtok(NULL, ":"); 
    char *filter_value = strtok(NULL, " ");

    for (int i = 0; filter_type[i]; i++) {
        filter_type[i] = tolower(filter_type[i]);
    }

    for (fds = 3; fds <= desc_nr; fds++) {
        if (fds != req_client && fds != sock_accept && status[fds] == 0) {
            if (write(fds, sent_comm, strlen(sent_comm)) < 0) {
                printf("[server] Eroare la write() catre client %d.\n", fds);
                perror("");
                fflush(stdout);
            }
            bytes = read(fds, rasp_peer, BUFF_SIZE);

            if (bytes <= 0) {
                close(fds);
                FD_CLR(fds, &all_fds);
            }

            rasp_part = strtok(rasp_peer, " ");
            found_n = atoi(rasp_part);

            if (found_n > 0) {
                bool has_matched_file = false;
                for (int i = 0; i < found_n; i++) {
                    rasp_part = strtok(NULL, " ");
                    if (strstr(rasp_part, file_name)) {
                        if (filter_type && filter_value) {
                            if (strcmp(filter_type, "filetype") == 0) {
                                if (strstr(rasp_part, filter_value)) {
                                    has_matched_file = true;
                                    break;
                                }
                            }
                        } else {
                            has_matched_file = true;
                            break;
                        }
                    }
                }

                if (has_matched_file) {
                    sprintf(msgres, "In clientul %d avem %d file-uri compatibile : ", fds, found_n);
                    hit_count += found_n;
                    client_hit++;
                    rasp_part = strtok(NULL, " ");
                    while (rasp_part != NULL) {
                        strcat(msgres, rasp_part);
                        strcat(msgres, " ");
                        rasp_part = strtok(NULL, " ");
                    }
                    strcat(raspuns, msgres);
                    strcat(raspuns, "\n");
                }
            }
        }
    }

    if (hit_count == 0) {
        bzero(raspuns, BUFF_SIZE);
        strcat(raspuns, "Nu avem hituri pentru file-ul cerut 0");
        if (write(req_client, raspuns, BUFF_SIZE) < 0) {
            printf("[server] Eroare la write() catre client %d.\n", fds);
            perror("");
            fflush(stdout);
        }
        exit(1);
    } else {
        sprintf(msgres, "%d", client_hit);
        strcat(raspuns, msgres);
        if (write(req_client, raspuns, BUFF_SIZE) < 0) {
            printf("[server] Eroare la write() catre client %d.\n", fds);
            perror("");
            fflush(stdout);
        }
        bytes = read(req_client, rasp_peer, BUFF_SIZE);
        if (bytes <= 0) {
            close(fds);
            FD_CLR(fds, &all_fds);
        }
        if (rasp_peer[0] == 'n') {
            return;
        }
        make_transfer_conn(rasp_peer, req_client, status);
    }
}

void make_transfer_conn(char *comm, int reciver, int status[]) {
    char *p;
    char send[BUFF_SIZE];
    bzero(send, BUFF_SIZE);
    p = strtok(comm, " ");
    int sender = atoi(p);
    if (status[sender]) {
        strcat(send, "client ocupat");
        if (write(reciver, send, BUFF_SIZE) < 0) {
            printf("[server] Eroare la write() catre client %d.\n", sender);
            perror("");
            fflush(stdout);
        }
        return;
    }

    struct sockaddr_in addresa;
    socklen_t addrlen = sizeof(addresa);
    getpeername(sender, (struct sockaddr *)&addresa, &addrlen);
    char *ip_sender = inet_ntoa(addresa.sin_addr);

    p = strtok(NULL, " ");
    bzero(send, BUFF_SIZE);
    strcat(send, p);
    strcat(send, " connection request");
    if (write(sender, send, BUFF_SIZE) < 0) {
        printf("[server] Eroare la write() catre client %d.\n", sender);
        perror("");
        fflush(stdout);
    }

    bzero(send, BUFF_SIZE);
    strcat(send, ip_sender);
    if (write(reciver, send, BUFF_SIZE) < 0) {
        printf("[server] Eroare la write() catre client %d.\n", sender);
        perror("");
        fflush(stdout);
    }
}

int sayHello(int fd) {
    char buffer[BUFF_SIZE];
    int bytes;
    char msg[BUFF_SIZE];
    char msgrasp[BUFF_SIZE];

    bytes = read(fd, msg, sizeof(buffer));
    if (bytes <= 0) {
        perror("Eroare la read() de la client.\n");
        return 1;
    }

    bzero(msgrasp, BUFF_SIZE);
    strcat(msgrasp, "Hello ");
    strcat(msgrasp, msg);

    if (bytes && write(fd, msgrasp, bytes) < 0) {
        perror("[server] Eroare la write() catre client.\n");
        return 1;
    }
    return bytes;
}
