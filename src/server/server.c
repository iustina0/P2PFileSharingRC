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
#include "server_util.h"

#define IP_SERVER "192.168.138.42"
#define PORT_S 4444
#define MAX_CLIENT 10
#define BUFF_SIZE 1024

int sock_accept, client;
fd_set read_fds, all_fds;
int busy[MAX_CLIENT];

int main() {
    struct sockaddr_in server_struct;
    struct sockaddr_in client_struct;

    FD_ZERO(&all_fds);

    int optval = 1;
    int fd, fds;
    int max_fd;
    int len;

    if ((sock_accept = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("<server> Eroare la socket().\n");
        return errno;
    }

    setsockopt(sock_accept , SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));

    bzero(&server_struct, sizeof(server_struct));
    server_struct.sin_family = AF_INET;
    server_struct.sin_addr.s_addr = htonl(INADDR_ANY);
    server_struct.sin_port = htons(PORT_S);

    if (bind(sock_accept, (struct sockaddr *)&server_struct, sizeof(struct sockaddr)) == -1) {
        perror("\033[1;31m<server> Eroare la bind().\033[0m\n");
        return errno;
    }

    if (listen(sock_accept, 5) == -1) {
        perror("\033[1;31m<server> Eroare la listen().\033[0m\n");
        return errno;
    }

    FD_ZERO(&all_fds);
    FD_SET(sock_accept, &all_fds);

    max_fd = sock_accept;

    while (1) {
        bcopy((char *)&all_fds, (char *)&read_fds, sizeof(read_fds));

        for (int i = 0; i < 20; i++) {
            int temp = check_if_busy(i);
        }

        if (select(max_fd + 1, &read_fds, NULL, NULL, NULL) < 0) {
            perror("\033[1;31m<server> Eroare la select().\033[0m\n");
            return errno;
        }

        if (FD_ISSET(sock_accept, &read_fds)) {
            len = sizeof(client_struct);
            bzero(&client_struct, sizeof(client_struct));

            client = accept(sock_accept, (struct sockaddr *)&client_struct, &len);
            if (client < 0) {
                perror("\033[1;31m<server> Eroare la accept().\033[0m\n");
                continue;
            }

            FD_SET(client, &all_fds);
            if (max_fd < client)
                max_fd = client;
        }

        for (fd = 0; fd <= max_fd; fd++) {
            if (fd != sock_accept && FD_ISSET(fd, &read_fds)) {
                char buffer[BUFF_SIZE];
                int bytes;
                char msg[BUFF_SIZE];
                char msgrasp[BUFF_SIZE];
                bzero(msg, BUFF_SIZE);
                bzero(msgrasp, BUFF_SIZE);

                bytes = read(fd, msg, sizeof(buffer));
                if (bytes <= 0) {
                    printf("\033[1;31m<server> S-a deconectat clientul cu descriptorul %d\033[0m.\n", fd);
                    fflush(stdout);
                    if (max_fd == fd) {
                        max_fd--;
                    }
                    close(fd);
                    FD_CLR(fd, &all_fds);
                }

                if (!strncmp(msg, "search for", 9)) {
                    for (int i = 0; i < 20; i++) {
                        int temp = check_if_busy(i);
                    }
                    split_command_process(msg, max_fd, fd, busy);
                } else {
                    continue;
                }
            }
        }
    }

    return 0;
}
