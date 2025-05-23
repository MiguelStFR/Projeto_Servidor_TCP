#define _WINSOCK_DEPRECATED_NO_WARNINGS // Permite uso de funções antigas como inet_addr
#include <winsock2.h>                   // Biblioteca principal para sockets no Windows
#include <windows.h>                    // Necessária para funções do Windows
#include <ws2tcpip.h>                   // Extensões TCP/IP (não usadas diretamente aqui)
#include <stdio.h>                      // Entrada e saída padrão
#include <stdint.h>                     // Tipos inteiros com tamanho definido
#include <string.h>                     // Manipulação de strings

#pragma comment(lib, "ws2_32.lib")      // Linka biblioteca de sockets do Windows

#define SERVER_IP "127.0.0.1"           // IP do servidor (localhost)
#define SERVER_PORT 12345               // Porta de comunicação

// Estrutura da mensagem binária
typedef struct {
    uint16_t type;                      // Tipo da mensagem (OI, TCHAU, MSG, SISTEMA)
    uint16_t orig_uid;                 // UID do cliente que envia
    uint16_t dest_uid;                 // UID do cliente que recebe
    uint16_t text_len;                 // Comprimento do texto
    char text[141];                    // Texto da mensagem
} msg_t;

// Função auxiliar para imprimir mensagens recebidas
void print_msg(msg_t* msg) {
    // Converte campos da ordem de rede para host
    msg->type = ntohs(msg->type);
    msg->orig_uid = ntohs(msg->orig_uid);
    msg->dest_uid = ntohs(msg->dest_uid);
    msg->text_len = ntohs(msg->text_len);

    // Exibe o conteúdo da mensagem
    if (msg->type == 3) { // MSG normal
        printf_s("Mensagem de %hu: %s\n", msg->orig_uid, msg->text);
    }
    else if (msg->type == 2) { // Mensagem do sistema
        printf_s("Mensagem de %hu: %s\n", msg->orig_uid, msg->text);
    }
}

// Função principal do cliente de exibição
int main() {
    WSADATA wsa;
    SOCKET sock;
    struct sockaddr_in server;
    msg_t msg;

    // Inicializa a biblioteca Winsock
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        printf("WSAStartup falhou: %d\n", WSAGetLastError());
        return 1;
    }

    // Criação do socket TCP
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == INVALID_SOCKET) {
        printf("Erro no socket: %d\n", WSAGetLastError());
        return 1;
    }

    // Configura o endereço do servidor
    server.sin_family = AF_INET;
    server.sin_port = htons(SERVER_PORT);
    server.sin_addr.s_addr = inet_addr(SERVER_IP);

    // Tenta conectar ao servidor
    if (connect(sock, (struct sockaddr*)&server, sizeof(server)) < 0) {
        printf("Erro ao conectar no servidor.\n");
        return 1;
    }

    printf("Conectado ao servidor.\n");

    // Entrada do UID do cliente exibidor
    uint16_t meu_uid;
    printf("Digite seu UID (exibidor): ");
    scanf_s("%hu", &meu_uid);

    // Envia mensagem de OI ao servidor
    memset(&msg, 0, sizeof(msg));
    msg.type = htons(0); // Tipo OI
    msg.orig_uid = htons(meu_uid);
    msg.dest_uid = htons(2); // UID do servidor (ou valor simbólico)

    send(sock, (char*)&msg, sizeof(msg), 0);

    // Aguarda resposta do servidor
    int bytes = recv(sock, (char*)&msg, sizeof(msg), 0);
    if (bytes <= 0 || ntohs(msg.type) != 0) {
        printf("Conexão rejeitada pelo servidor.\n");
        closesocket(sock);
        WSACleanup();
        return 1;
    }

    printf("Conexão aceita. Aguardando mensagens...\n");

    // Loop principal de recepção de mensagens
    while (1) {
        bytes = recv(sock, (char*)&msg, sizeof(msg), 0);
        if (bytes <= 0) {
            printf("Conexão encerrada pelo servidor.\n");
            break;
        }
        print_msg(&msg); // Imprime mensagens do tipo MSG ou SISTEMA
    }

    // Finaliza o socket e limpa a Winsock
    closesocket(sock);
    WSACleanup();
    return 0;
}
