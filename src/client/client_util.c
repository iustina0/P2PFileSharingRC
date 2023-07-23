#include "client_util.h"

struct FileDetails fisiere_detinute[20]; 

void get_files_dir(char *dir, int y) {
    DIR * direct;
    struct dirent *file;
    int i=0; 

    if((direct=opendir("."))!=NULL)
    {
        while ((file = readdir (direct)) != NULL && i<20) {
           // if(file->d_type!=4){
                strcpy(fisiere_detinute[i].nume, file->d_name);
                fisiere_detinute[i].tip= file->d_type;
            //}
            i++;
        }   
        closedir (direct);
    } else {
        perror ("\033[1;31m<client> Nu putem citi fisierele din directoriu: \n\033[0m");
        exit(1);
    }

}

void search_for_file(char *request, int server)
{
    get_files_dir(".", 0);
    int hits=0;
    char raspuns[BUFF_SIZE];
    bzero(raspuns, BUFF_SIZE);
    char lista[1000];
    char * file_name=strtok(request, ": "); //luam doar <term> din comanda search for : <term>
    file_name=strtok(NULL, ": ");
    file_name=strtok(NULL, ": ");

    for(int i=0 ; i<20; i++)
    {
        if(strstr(fisiere_detinute[i].nume, file_name)){
            hits++;
            strcat(lista, fisiere_detinute[i].nume);
            strcat(lista, " ");
        }
    }
    if(hits!=0)
    {
        sprintf(raspuns, "%d ", hits);
        strcat(raspuns, lista);
    }
    else{
        strcat(raspuns, "0 ");
    }
    if (write(server, raspuns, BUFF_SIZE) <= 0)
    {
        perror("\033[1;31m<client>Eroare la write() spre server.\n\033[0m");
        return;
    }
}

#include <zlib.h>

void generate_encryption_key(unsigned char *key) {
    if (RAND_bytes(key, ENCRYPTION_KEY_SIZE) != 1) {
        perror("Error generating encryption key");
        exit(EXIT_FAILURE);
    }
}

void aes_encrypt(const unsigned char *plaintext, int plaintext_len, const unsigned char *key, unsigned char *ciphertext) {
    AES_KEY aes_ctx;
    AES_set_encrypt_key(key, 256, &aes_ctx);
    AES_encrypt(plaintext, ciphertext, &aes_ctx);
}

void aes_decrypt(const unsigned char *ciphertext, int ciphertext_len, const unsigned char *key, unsigned char *plaintext) {
    AES_KEY aes_ctx;
    AES_set_decrypt_key(key, 256, &aes_ctx);
    AES_decrypt(ciphertext, plaintext, &aes_ctx);
}

int send_encrypted_data(int fd, const unsigned char *data, int data_len, const unsigned char *key) {
    unsigned char *encrypted_data = (unsigned char *)malloc(data_len);
    aes_encrypt(data, data_len, key, encrypted_data);

    int bytes_sent = send(fd, encrypted_data, data_len, 0);
    free(encrypted_data);

    return bytes_sent;
}

int recv_encrypted_data(int fd, unsigned char *buffer, int buffer_size, const unsigned char *key) {
    int bytes_received = recv(fd, buffer, buffer_size, 0);
    if (bytes_received <= 0) {
        return bytes_received;
    }

    unsigned char *decrypted_buffer = (unsigned char *)malloc(buffer_size);
    aes_decrypt(buffer, bytes_received, key, decrypted_buffer);
    memcpy(buffer, decrypted_buffer, bytes_received);
    free(decrypted_buffer);

    return bytes_received;
}

void recive_process(char *file_name, int server) {
    sleep(1);
    int sender_fd;
    struct sockaddr_in sender;
    char msg[BUFF_SIZE], ip_sender[BUFF_SIZE], buffer[BUFF_SIZE];
    int by;
    bzero(ip_sender, BUFF_SIZE);
    bzero(buffer, BUFF_SIZE);
    bzero(msg, BUFF_SIZE);
    by = read(server, ip_sender, BUFF_SIZE);
    if (by <= 0) {
        perror("\033[1;31m<client>Eroare la read() cu serverul.\n\033[0m\n");
        return;
    }

    if ((sender_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("\033[1;31m<client>Eroare la socket().\n\033[0m");
        return;
    }
    sender.sin_family = AF_INET;
    sender.sin_addr.s_addr = inet_addr(ip_sender);
    sender.sin_port = htons(PORT_d);

    if (connect(sender_fd, (struct sockaddr *)&sender, sizeof(struct sockaddr)) == -1) {
        perror("\033[1;31m<client>Eroare la connect().\n\033[0m");
        return;
    }
    int i;
    for (i = 0; i < 20; i++) {
        if (!strcmp(fisiere_detinute[i].nume, file_name)) {
            i = 23;
        }
    }
    if (i >= 23) {
    bzero(msg, BUFF_SIZE);
    strcat(msg, "file not found");
    if (write(sender_fd, msg, BUFF_SIZE) <= 0) {
        perror("\033[1;31m<client>Eroare la write().\n\033[0m");
        return;
    }
    return;
    }
    else {
        bzero(msg, BUFF_SIZE);
        strcat(msg, "ready for reciving");
        if (write(sender_fd, msg, BUFF_SIZE) <= 0) {
            perror("\033[1;31m<client>Eroare la write().\n\033[0m");
            return;
        }
        bzero(msg, BUFF_SIZE);
        int n = read(sender_fd, msg, BUFF_SIZE);
        if (n < 0 || strcmp(msg, "ready to send")) {
            printf("\033[1;31m\n<client>Actiune intrerupta de sender: %s\n\033[0m", msg);
            return;
        }
    }

    FILE *fisier;
    fisier = fopen(file_name, "wb");

    unsigned char buffer[BUFF_SIZE];
    while (1) {
        int bytes_received = recv_encrypted_data(sender_fd, buffer, sizeof(buffer), encryption_key);
        if (bytes_received <= 0)
            break;
        int bytes_to_write = (bytes_received == AES_BLOCK_SIZE) ? bytes_received : bytes_received - (bytes_received % AES_BLOCK_SIZE);
        fwrite(buffer, 1, bytes_to_write, fisier);
    }
    printf("\033[1;32m\nAm primit fisierul %s.\n\033[0m", file_name);
    fclose(fisier);

    uLongf decompressed_size = (uLongf)BUFF_SIZE;
    char *decompressed_buffer = (char *)malloc(BUFF_SIZE);
    if (decompressed_buffer == NULL) {
        perror("<client> Eroare la alocarea memoriei pentru decompresie.\n");
        return;
    }

    if (uncompress((Bytef *)decompressed_buffer, &decompressed_size, (const Bytef *)buffer, BUFF_SIZE) != Z_OK) {
        perror("<client> Eroare la decompresie.\n");
        free(decompressed_buffer);
        return;
    }

    fisier = fopen(file_name, "wb");
    if (fisier == NULL) {
        perror("<client> Eroare la deschiderea fisierului.\n");
        free(decompressed_buffer);
        return;
    }

    fwrite(decompressed_buffer, 1, decompressed_size, fisier);
    printf("\033[1;32m\nAm primit fisierul %s.\n\033[0m", file_name);
    fclose(fisier);
    free(decompressed_buffer);
}

void look_for_download(int server, char *request){
    char raspuns[BUFF_SIZE], utilizator[BUFF_SIZE], rasp[BUFF_SIZE];
    bzero(utilizator, BUFF_SIZE);
    bzero(rasp, BUFF_SIZE);
    char *part;
    if (send(server, request, BUFF_SIZE, 0) <= 0)   
    {
        perror("\033[1;31m<client>Eroare la send().\n\033[0m");
        return;
    }

    int by;
    bzero(raspuns, BUFF_SIZE);
    by=read(server, raspuns, BUFF_SIZE);
    if (by <= 0)
    {
        perror("\033[1;31m<client>Serverul este deconectat.\n\033[0m\n");
        exit(1);
        return;
    }

    if(raspuns[strlen(raspuns)-1]=='0')
    {
        printf("\033[0;34m Nu am gasit fisiere compatibile cu termenul de cautare \n \033[0m ");
        return;
    }
    if(raspuns[strlen(raspuns)-1]=='1') 
    {
        raspuns[strlen(raspuns)-1]=='\0';
        printf("\033[0;34m \nAm gasit fisiere compatibile cu termenul de cautare intr-un singur peer \n %s \033[0m", raspuns);

        
        printf("\033[0;32m \n>Doriti sa continuati (y/n):  \033[0m");

        fgets(utilizator, BUFF_SIZE, stdin);
        if(!strcmp(utilizator, "n\n")){
            bzero(raspuns, BUFF_SIZE);
            strcat(raspuns, "n");
            if (write(server, raspuns, BUFF_SIZE) < 0)
            {
                perror("<client> Eroare la write() .\n");
                fflush(stdout);
            } 
            return;
        }

        printf("\033[0;32m \n>Introduceti numele fisierului pe care doriti sa il descarcati din lista de mai sus:  \033[0m");
        fgets(utilizator, BUFF_SIZE, stdin);

        utilizator[strlen(utilizator)-1] = '\0';

        bzero(rasp, BUFF_SIZE);
        part=strtok(raspuns, " ");
        part=strtok(NULL, " ");
        part=strtok(NULL, " "); 
        strcat(rasp, part);
        strcat(rasp, " ");
        strcat(rasp, utilizator);

        if (write(server, rasp, BUFF_SIZE) < 0)
        {
            perror("<client> Eroare la write() .\n");
            fflush(stdout);
        } 
        
        recive_process(utilizator, server);
        return;

    }
    int c[10]={0}, i=0;
    char * p;
    printf("\033[0;34m \nAm gasit fisiere compatibile cu termenul de cautare in %c peers \n\033[0m", raspuns[strlen(raspuns)-1]);
    raspuns[strlen(raspuns)-1]=='\0';
    printf("\033[0;34m %s \n\033[0m", raspuns);

    printf("\033[0;32m \n>Doriti sa continuati (y/n):  \033[0m");

    fgets(utilizator, BUFF_SIZE, stdin);
    if(!strcmp(utilizator, "n\n")){
        bzero(raspuns, BUFF_SIZE);
        strcat(raspuns, "n");
        if (write(server, raspuns, BUFF_SIZE) < 0)
        {
            perror("<clinet> Eroare la write() .\n");
            fflush(stdout);
        } 
        return;
    }

    bzero(rasp, BUFF_SIZE);
    printf("\033[0;32m \n>Introduceti numele peerului din care vreti sa preluati fisierul: \033[0m");
    fgets(utilizator, BUFF_SIZE, stdin);
    utilizator[strlen(utilizator)-1] = '\0';
    strcat(rasp, utilizator);
    strcat(rasp, " ");

    printf("\033[0;32m \n>Introduceti numele fisierului pe care doriti sa il descarcati din lista de mai sus:  \033[0m");
    fgets(utilizator, BUFF_SIZE, stdin);
    utilizator[strlen(utilizator)-1] = '\0';
    strcat(rasp, utilizator);

    if (write(server, rasp, BUFF_SIZE) < 0)
    {
        perror("<client> Eroare la write() .\n");
        fflush(stdout);
    } 
    recive_process(utilizator, server);
}

void transfer_procces(char *file_name) 
{
    int reciver_fd, rec;
    socklen_t reciver;
    char buffer[BUFF_SIZE];
    struct sockaddr_in sender_addr, reciver_addr;
    int n;

    if ((rec = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("<client>Eroare la socket().\n");
        return ;
    }

    bzero((char *) &sender_addr, sizeof(sender_addr));
    sender_addr.sin_family = AF_INET;
    sender_addr.sin_addr.s_addr = INADDR_ANY;
    sender_addr.sin_port = htons(PORT_d);

    if (bind(rec, (struct sockaddr *)&sender_addr, sizeof(struct sockaddr)) == -1)
    {
        perror("<client> Eroare la bind().\n");
        return ;
    }

    if (listen(rec, 5) == -1)
    {
        perror("<client> Eroare la listen().\n");
        return ;
    }

    reciver = sizeof(reciver_addr);


    reciver_fd = accept(rec, (struct sockaddr *)&reciver_addr, &reciver);

    if (reciver_fd < 0)
    {
        perror("<client> Eroare la accept().\n");
        return;
    }
    char msg[BUFF_SIZE];

    bzero(msg, BUFF_SIZE);
    n = read(reciver_fd, msg, BUFF_SIZE);
    if (n < 0 || strcmp(msg, "ready for reciving")){
        return;
    } 
    int i;
    for(i=0; i<20;i++)
    {
        if(!strcmp(fisiere_detinute[i].nume, file_name))
        {
            i=23;
        }
    }
    if(i==23)
    {    
        bzero(msg, BUFF_SIZE);
        strcat(msg, "file not found");
        if (write(reciver_fd, msg, BUFF_SIZE) <= 0)
        {
            perror("\033[1;31m<client>Eroare la write().\n\033[0m");
            return;
    }
        return;
    }

    bzero(msg, BUFF_SIZE);
    strcat(msg, "ready to send");
    if (write(reciver_fd, msg, BUFF_SIZE) <= 0)
    {
        perror("\033[1;31m<client>Eroare la write().\n\033[0m");
        return;
    }


    FILE *fisier;
    fisier = fopen(file_name, "rb");

    unsigned char buffer[BUFF_SIZE];
    while (1) {
        int bytes_read = fread(buffer, 1, sizeof(buffer), fisier);
        if (bytes_read == 0)
            break;
        int padded_bytes = bytes_read;
        if (bytes_read < AES_BLOCK_SIZE) {
            padded_bytes = AES_BLOCK_SIZE;
            memset(buffer + bytes_read, 0, padded_bytes - bytes_read);
        }
        send_encrypted_data(reciver_fd, buffer, padded_bytes, encryption_key);
    }
    printf("\033[0;32m\nAm trimis un fisier : %s catre %d \033[0m", file_name, reciver_fd);
    fclose(fisier);
    close(reciver_fd);
    close(rec);
    return ; 

}
