
#include "client_util.h"


int main() {
    int sd;
    struct sockaddr_in server;
    char msg[BUFF_SIZE];

    fd_set read_fds, all_fds; 
    FD_ZERO(&all_fds);
    FD_ZERO(&read_fds);
    int max_fd = 0;

    get_files_dir(".", 0);
    
    unsigned char encryption_key[ENCRYPTION_KEY_SIZE];
    generate_encryption_key(encryption_key);

    if ((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("<client>Eroare la socket().\n");
        return errno;
    }

    server.sin_family = AF_INET;
    server.sin_addr.s_addr = inet_addr(IP_server);
    server.sin_port = htons(port);

    if (connect(sd, (struct sockaddr *)&server, sizeof(struct sockaddr)) == -1) {
        perror("<client>Eroare la connect().\n");
        return errno;
    }

    FD_SET(sd, &all_fds);
    max_fd = max(max_fd, sd);

    FD_SET(0, &all_fds);
    max_fd = max(max_fd, 0);

    while (1) {
        bcopy((char *)&all_fds, (char *)&read_fds, sizeof(read_fds));
        printf("\033[0;32m> \033[0m");
        fflush(stdout);

        if (select(max_fd + 1, &read_fds, NULL, NULL, NULL) < 0)
        {
            perror("<client> Eroare la select().\n");
            return errno;
        }

        if (FD_ISSET(sd, &read_fds)) {
            fflush(stdout);
            int by;
            bzero(msg, BUFF_SIZE);
            by = recv(sd, msg, BUFF_SIZE, 0);
            if (by <= 0) {
                perror("<client>Serverul e deconectat.\n");
                return errno;
            }
            if (!strncmp(msg, "search for", 9)) {
                search_for_file(msg, sd);
            }
            if (strstr(msg, "connection request")) {
                char *p = strtok(msg, " ");
                transfer_procces(p);
            }
        } else {
            bzero(msg, BUFF_SIZE);
            fflush(stdout);
            if (fgets(msg, BUFF_SIZE, stdin)) {
                msg[strlen(msg) - 1] = '\0';
                if (!strncmp(msg, "search for", 9)) {
                    look_for_download(sd, msg);
                } else if (!strcmp(msg, "?")) {
                    printf("\033[0;32m \n>Introduceti search for : <> pentru a cauta fisiere in retea.\nRamaneti in retea pentru a impartasii fisierele proprii cu alti clienti\n\033[0m");
                    fflush(stdout);
                } else {
                    printf("\033[0;32m \n>comanda nerecunoscuta. Incercati ? pentru ajutor \n\033[0m");
                    fflush(stdout);
                    continue;
                }
            }
        }
    }
    close(sd);
}
