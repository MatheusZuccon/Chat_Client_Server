// server.c
#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define PORTA 20032
#define TAMMAX 250
#define MAX_CLIENTS 100

typedef struct {
    int sock;
    char name[TAMMAX];
} client_t;

client_t *clients[MAX_CLIENTS];
pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;

void add_client(client_t *cl) {
    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < MAX_CLIENTS; ++i) {
        if (!clients[i]) {
            clients[i] = cl;
            break;
        }
    }
    pthread_mutex_unlock(&clients_mutex);
}

void remove_client(int sock) {
    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < MAX_CLIENTS; ++i) {
        if (clients[i]) {
            if (clients[i]->sock == sock) {
                free(clients[i]);
                clients[i] = NULL;
                break;
            }
        }
    }
    pthread_mutex_unlock(&clients_mutex);
}

void broadcast_message(const char *msg, int except_sock) {
    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < MAX_CLIENTS; ++i) {
        if (clients[i]) {
            if (clients[i]->sock != except_sock) {
                ssize_t sent = send(clients[i]->sock, msg, strlen(msg), 0);
                (void)sent; // ignorar retorno aqui (poderia logar)
            }
        }
    }
    pthread_mutex_unlock(&clients_mutex);
}

void *handle_client(void *arg) {
    client_t *cli = (client_t *)arg;
    char buffer[TAMMAX];
    ssize_t n;

    // Primeiro: receber nome (espera que cliente envie nome ao conectar)
    n = recv(cli->sock, buffer, TAMMAX - 1, 0);
    if (n <= 0) {
        close(cli->sock);
        remove_client(cli->sock);
        return NULL;
    }
    buffer[n] = '\0';
    strncpy(cli->name, buffer, TAMMAX - 1);
    cli->name[TAMMAX-1] = '\0';

    // Notificar entrada
    char join_msg[TAMMAX + 50];
    snprintf(join_msg, sizeof(join_msg), "*** %s entrou no chat ***\n", cli->name);
    broadcast_message(join_msg, cli->sock);
    printf("%s", join_msg);

    // Loop de recebimento e broadcast
    while ((n = recv(cli->sock, buffer, TAMMAX - 1, 0)) > 0) {
        buffer[n] = '\0';
        // se o cliente enviar "exit", encerra
        if (strcmp(buffer, "exit") == 0) break;

        // formatar mensagem com nome
        char out[TAMMAX + 50];
        snprintf(out, sizeof(out), "%s: %s\n", cli->name, buffer);
        printf("%s", out); // log no servidor
        broadcast_message(out, cli->sock);
    }

    // cliente saiu ou erro
    snprintf(join_msg, sizeof(join_msg), "*** %s saiu do chat ***\n", cli->name);
    broadcast_message(join_msg, cli->sock);
    printf("%s", join_msg);

    close(cli->sock);
    remove_client(cli->sock);
    return NULL;
}

int main() {
    int listenfd = 0, connfd = 0;
    struct sockaddr_in servaddr, cliaddr;
    socklen_t cli_len = sizeof(cliaddr);

    // limpar lista
    for (int i = 0; i < MAX_CLIENTS; ++i) clients[i] = NULL;

    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if (listenfd < 0) { perror("socket"); exit(1); }

    int opt = 1;
    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(PORTA);

    if (bind(listenfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        perror("bind"); close(listenfd); exit(1);
    }

    if (listen(listenfd, 10) < 0) {
        perror("listen"); close(listenfd); exit(1);
    }

    printf("Servidor escutando na porta %d...\n", PORTA);

    while (1) {
        connfd = accept(listenfd, (struct sockaddr *)&cliaddr, &cli_len);
        if (connfd < 0) {
            perror("accept");
            continue;
        }

        // alocar cliente
        client_t *cli = (client_t *)malloc(sizeof(client_t));
        if (!cli) { perror("malloc"); close(connfd); continue; }
        cli->sock = connfd;
        cli->name[0] = '\0';

        add_client(cli);

        pthread_t tid;
        if (pthread_create(&tid, NULL, &handle_client, (void *)cli) != 0) {
            perror("pthread_create");
            remove_client(cli->sock);
            close(connfd);
            free(cli);
        } else {
            pthread_detach(tid); // não precisamos fazer pthread_join
        }
    }

    close(listenfd);
    return 0;
}