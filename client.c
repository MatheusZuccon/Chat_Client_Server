// client.c
#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define PORTA 20032
#define TAMMAX 250

int sockfd = -1;
volatile int running = 1;

void *recv_handler(void *arg) {
    char buffer[TAMMAX + 50];
    ssize_t n;
    while (running && (n = recv(sockfd, buffer, sizeof(buffer)-1, 0)) > 0) {
        buffer[n] = '\0';
        printf("%s", buffer);
        fflush(stdout);
    }
    running = 0;
    return NULL;
}

int main(int argc, char **argv) {
    struct sockaddr_in servaddr;
    char msg[TAMMAX];

    if (argc < 3) {
        printf("Uso: %s <host> <nome>\n", argv[0]);
        printf("Ex: %s 127.0.0.1 Matheus\n", argv[0]);
        exit(0);
    }

    const char *host = argv[1];
    const char *name = argv[2];

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) { perror("socket"); exit(1); }

    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(PORTA);
    if (inet_pton(AF_INET, host, &servaddr.sin_addr) <= 0) {
        perror("inet_pton");
        close(sockfd);
        exit(1);
    }

    if (connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        perror("connect");
        close(sockfd);
        exit(1);
    }

    // enviar nome como primeira mensagem
    if (send(sockfd, name, strlen(name), 0) < 0) {
        perror("send name");
        close(sockfd);
        exit(1);
    }

    printf("Conectado ao servidor %s como '%s'. Digite mensagens (ou 'exit' para sair)\n", host, name);

    pthread_t recv_thread;
    if (pthread_create(&recv_thread, NULL, recv_handler, NULL) != 0) {
        perror("pthread_create");
        close(sockfd);
        exit(1);
    }

    // loop de escrita
    while (running) {
        if (!fgets(msg, TAMMAX, stdin)) break;
        // remover newline
        size_t len = strlen(msg);
        if (len > 0 && msg[len-1] == '\n') msg[len-1] = '\0';

        if (strcmp(msg, "exit") == 0) {
            // envia exit para avisar ao servidor
            send(sockfd, "exit", 4, 0);
            break;
        }

        if (send(sockfd, msg, strlen(msg), 0) < 0) {
            perror("send");
            break;
        }
    }

    // encerra
    running = 0;
    shutdown(sockfd, SHUT_RDWR);
    close(sockfd);
    pthread_join(recv_thread, NULL);
    printf("Cliente encerrado.\n");
    return 0;
}
