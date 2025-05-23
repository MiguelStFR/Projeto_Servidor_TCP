#define _WINSOCK_DEPRECATED_NO_WARNINGS // Permite uso de fun��es antigas como inet_addr
#include <winsock2.h>                   // Biblioteca principal para sockets no Windows
#include <windows.h>                    // Necess�ria para fun��es do Windows
#include <ws2tcpip.h>                   // Extens�es TCP/IP (n�o usadas diretamente aqui)
#include <stdio.h>                      // Entrada e sa�da padr�o
#include <stdint.h>                     // Tipos inteiros com tamanho definido
#include <string.h>                     // Manipula��o de strings

#pragma comment(lib, "ws2_32.lib")      // Linka biblioteca de sockets do Windows

#define SERVER_IP "127.0.0.1"           // IP do servidor (localhost)
#define SERVER_PORT 12345               // Porta de comunica��o

// Estrutura da mensagem bin�ria
typedef struct {
    uint16_t type;                      // Tipo da mensagem (OI, TCHAU, MSG, SISTEMA)
    uint16_t orig_uid;                 // UID do cliente que envia
    uint16_t dest_uid;                 // UID do cliente que recebe
    uint16_t text_len;                 // Comprimento do texto
    char text[141];                    // Texto da mensagem
} msg_t;

// Fun��o auxiliar para imprimir mensagens recebidas
void print_msg(msg_t* msg) {
    // Converte campos da ordem de rede para host
    msg->type = ntohs(msg->type);
    msg->orig_uid = ntohs(msg->orig_uid);
    msg->dest_uid = ntohs(msg->dest_uid);
    msg->text_len = ntohs(msg->text_len);

    // Exibe o conte�do da mensagem
    if (msg->type == 3) { // MSG normal
        printf_s("Mensagem de %hu: %s\n", msg->orig_uid, msg->text);
    }
    else if (msg->type == 2) { // Mensagem do sistema
        printf_s("Mensagem de %hu: %s\n", msg->orig_uid, msg->text);
    }
}

// Fun��o principal do cliente de exibi��o
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

    // Cria��o do socket TCP
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == INVALID_SOCKET) {
        printf("Erro no socket: %d\n", WSAGetLastError());
        return 1;
    }

    // Configura o endere�o do servidor
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
    msg.dest_uid = htons(2); // UID do servidor (ou valor simb�lico)

    send(sock, (char*)&msg, sizeof(msg), 0);

    // Aguarda resposta do servidor
    int bytes = recv(sock, (char*)&msg, sizeof(msg), 0);
    if (bytes <= 0 || ntohs(msg.type) != 0) {
        printf("Conex�o rejeitada pelo servidor.\n");
        closesocket(sock);
        WSACleanup();
        return 1;
    }

    printf("Conex�o aceita. Aguardando mensagens...\n");

    // Loop principal de recep��o de mensagens
    while (1) {
        bytes = recv(sock, (char*)&msg, sizeof(msg), 0);
        if (bytes <= 0) {
            printf("Conex�o encerrada pelo servidor.\n");
            break;
        }
        print_msg(&msg); // Imprime mensagens do tipo MSG ou SISTEMA
    }

    // Finaliza o socket e limpa a Winsock
    closesocket(sock);
    WSACleanup();
    return 0;
}
