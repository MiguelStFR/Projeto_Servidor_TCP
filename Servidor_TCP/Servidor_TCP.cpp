// servidor_windows.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <time.h>

// Linka a biblioteca necessária para uso do Winsock
#pragma comment(lib, "Ws2_32.lib")

// Definições de porta, limites e tipos de mensagens
#define PORTA 12345
#define MAX_CLIENTS  10
#define TIPO_OI 0
#define TIPO_TCHAU 1
#define TIPO_MSG 2
#define TIPO_SERVER 3
#define TYPE_ENVIO 1
#define TYPE_EXIBIDOR 2

// Estrutura da mensagem que será trocada entre cliente e servidor
typedef struct {
    unsigned short type;
    unsigned short orig_uid;
    unsigned short dest_uid;
    unsigned short text_len;
    char text[256];
} msg_t;

// Estrutura para armazenar informações de um cliente
typedef struct {
    int socket;
    int uid;
    int type;
} ClientInfo;

ClientInfo clients[MAX_CLIENTS]; // Vetor que armazena os clientes conectados

// Remove um cliente do vetor, fechando seu socket
void remove_client(int sockfd) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].socket == sockfd) {
            closesocket(clients[i].socket);
            clients[i].socket = -1;
            clients[i].uid = 0;
            clients[i].type = 0;
            break;
        }
    }
}

// Retorna ponteiro para estrutura do cliente a partir do socket
ClientInfo* get_client_by_socket(int sockfd) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].socket == sockfd) {
            return &clients[i];
        }
    }
    return NULL;
}

// Envia mensagem do tipo SERVER a todos os clientes do tipo EXIBIDOR
void enviar_mensagem_periodica() {
    msg_t msg;
    msg.type = htons(TIPO_SERVER);
    msg.orig_uid = htons(0);
    msg.dest_uid = htons(0);
    strcpy_s(msg.text, "Mensagem periódica do servidor");
    msg.text_len = htons(strlen(msg.text));

    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].socket != -1 && clients[i].type == TYPE_EXIBIDOR) {
            send(clients[i].socket, (char*)&msg, sizeof(msg_t), 0);
        }
    }
}

int main() {
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData); // Inicializa Winsock

    SOCKET listen_fd = socket(AF_INET, SOCK_STREAM, 0); // Cria socket de escuta

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY; // Aceita conexões de qualquer IP
    server_addr.sin_port = htons(PORTA);      // Define a porta

    bind(listen_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)); // Liga o socket à porta
    listen(listen_fd, SOMAXCONN); // Começa a escutar conexões

    // Inicializa a lista de clientes
    for (int i = 0; i < MAX_CLIENTS; i++) {
        clients[i].socket = -1;
    }

    fd_set read_fds;
    printf("Servidor iniciado na porta %d\n", PORTA);

    time_t last_sent = time(NULL); // Marca tempo da última mensagem periódica

    while (1) {
        FD_ZERO(&read_fds);
        FD_SET(listen_fd, &read_fds);
        int max_fd = (int)listen_fd;

        // Adiciona todos os sockets de clientes ao conjunto de leitura
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (clients[i].socket != -1) {
                FD_SET(clients[i].socket, &read_fds);
                if (clients[i].socket > max_fd) {
                    max_fd = clients[i].socket;
                }
            }
        }

        struct timeval timeout;
        timeout.tv_sec = 1;
        timeout.tv_usec = 0;

        // Espera por atividade em qualquer socket
        int activity = select(max_fd + 1, &read_fds, NULL, NULL, &timeout);
        if (activity == SOCKET_ERROR) {
            perror("select");
            break;
        }

        // Verifica se é hora de enviar mensagem periódica
        time_t now = time(NULL);
        if (difftime(now, last_sent) >= 60) {
            enviar_mensagem_periodica();
            last_sent = now;
        }

        // Aceita nova conexão, se houver
        if (FD_ISSET(listen_fd, &read_fds)) {
            SOCKET new_sock = accept(listen_fd, NULL, NULL);
            for (int i = 0; i < MAX_CLIENTS; i++) {
                if (clients[i].socket == -1) {
                    clients[i].socket = (int)new_sock;
                    break;
                }
            }
        }

        // Lê dados de cada cliente que enviou algo
        for (int i = 0; i < MAX_CLIENTS; i++) {
            int sock = clients[i].socket;
            if (sock != -1 && FD_ISSET(sock, &read_fds)) {
                msg_t msg;
                int bytes_read = recv(sock, (char*)&msg, sizeof(msg_t), 0);
                if (bytes_read <= 0) {
                    remove_client(sock);
                    continue;
                }

                // Converte campos da mensagem de rede para host
                msg.type = ntohs(msg.type);
                msg.orig_uid = ntohs(msg.orig_uid);
                msg.dest_uid = ntohs(msg.dest_uid);
                msg.text_len = ntohs(msg.text_len);

                // Processa tipo da mensagem
                switch (msg.type) {
                case TIPO_OI:
                    // Identifica o cliente e seu tipo (ENVIO ou EXIBIDOR)
                    clients[i].uid = msg.orig_uid;
                    clients[i].type = msg.dest_uid;
                    printf("Cliente %d conectado como %s\n", msg.orig_uid,
                        msg.dest_uid == TYPE_ENVIO ? "ENVIO" : "EXIBIDOR");

                    // Envia confirmação de volta
                    msg_t fwd_msg = msg;
                    fwd_msg.type = htons(0);
                    fwd_msg.orig_uid = htons(0);
                    fwd_msg.dest_uid = htons(msg.orig_uid);
                    fwd_msg.text_len = htons(msg.text_len);

                    for (int j = 0; j < MAX_CLIENTS; j++) {
                        if (clients[j].socket != -1 && clients[j].uid == msg.dest_uid) {
                            send(clients[j].socket, (char*)&fwd_msg, sizeof(msg_t), 0);
                            break;
                        }
                    }
                    break;

                case TIPO_MSG: {
                    // Valida remetente
                    ClientInfo* sender = get_client_by_socket(sock);
                    if (!sender || sender->uid != msg.orig_uid) {
                        printf("Mensagem inválida de UID %d\n", msg.orig_uid);
                        break;
                    }

                    msg_t fwd_msg = msg;
                    fwd_msg.type = htons(TIPO_MSG);
                    fwd_msg.orig_uid = htons(msg.orig_uid);
                    fwd_msg.dest_uid = htons(msg.dest_uid);
                    fwd_msg.text_len = htons(msg.text_len);

                    // Broadcast se destino for 0, senão envia para destino específico
                    if (msg.dest_uid == 0) {
                        for (int j = 0; j < MAX_CLIENTS; j++) {
                            if (clients[j].socket != -1 && clients[j].type == TYPE_EXIBIDOR) {
                                send(clients[j].socket, (char*)&fwd_msg, sizeof(msg_t), 0);
                            }
                        }
                    }
                    else {
                        for (int j = 0; j < MAX_CLIENTS; j++) {
                            if (clients[j].socket != -1 &&
                                clients[j].uid == msg.dest_uid &&
                                clients[j].type == TYPE_EXIBIDOR) {
                                send(clients[j].socket, (char*)&fwd_msg, sizeof(msg_t), 0);
                                break;
                            }
                        }
                    }
                    break;
                }

                case TIPO_TCHAU:
                    // Cliente saiu
                    printf("Cliente %d se despediu.\n", msg.orig_uid);
                    remove_client(sock);
                    break;

                default:
                    // Mensagem desconhecida
                    printf("Tipo de mensagem desconhecido: %d\n", msg.type);
                    break;
                }
            }
        }
    }

    // Encerra socket e limpa Winsock
    closesocket(listen_fd);
    WSACleanup();
    return 0;
}
