#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <winsock2.h>
#include <windows.h>
#include <ws2tcpip.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#pragma comment(lib, "ws2_32.lib")

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 12345

typedef struct {
    uint16_t type;
    uint16_t orig_uid;
    uint16_t dest_uid;
    uint16_t text_len;
    char text[141];
} msg_t;

void print_msg(msg_t* msg) {
    msg->type = ntohs(msg->type);
    msg->orig_uid = ntohs(msg->orig_uid);
    msg->dest_uid = ntohs(msg->dest_uid);
    msg->text_len = ntohs(msg->text_len);

    if (msg->type == 3) { // MSG
        printf_s("Mensagem de %hu: %s\n", msg->orig_uid, msg->text);
    }
    else if (msg->type == 2) { // MENSAGEM DO SISTEMA
        printf_s("Mensagem de %hu: %s\n", msg->orig_uid, msg->text);
    }
    //else {
    //    printf_s("[INFO] Tipo desconhecido: %hu\n", msg->type);
    //}
}

int main() {
    WSADATA wsa;
    SOCKET sock;
    struct sockaddr_in server;
    msg_t msg;

    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        printf("WSAStartup falhou: %d\n", WSAGetLastError());
        return 1;
    }

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == INVALID_SOCKET) {
        printf("Erro no socket: %d\n", WSAGetLastError());
        return 1;
    }

    server.sin_family = AF_INET;
    server.sin_port = htons(SERVER_PORT);
    server.sin_addr.s_addr = inet_addr(SERVER_IP);

    if (connect(sock, (struct sockaddr*)&server, sizeof(server)) < 0) {
        printf("Erro ao conectar no servidor.\n");
        return 1;
    }

    printf("Conectado ao servidor.\n");

    uint16_t meu_uid;
    printf("Digite seu UID (exibidor): ");
    scanf_s("%hu", &meu_uid);

    // Envia OI
    memset(&msg, 0, sizeof(msg));
    msg.type = htons(0); // OI
    msg.orig_uid = htons(meu_uid);
	msg.dest_uid = htons(2); // EXIBIDOR

    send(sock, (char*)&msg, sizeof(msg), 0);

    // Aguarda resposta OI
    int bytes = recv(sock, (char*)&msg, sizeof(msg), 0);
    if (bytes <= 0 || ntohs(msg.type) != 0) {
        printf("Conexão rejeitada pelo servidor.\n");
        closesocket(sock);
        WSACleanup();
        return 1;
    }

    printf("Conexão aceita. Aguardando mensagens...\n");

    // Loop de recepção
    while (1) {
        bytes = recv(sock, (char*)&msg, sizeof(msg), 0);
        if (bytes <= 0) {
            printf("Conexão encerrada pelo servidor.\n");
            break;
        }
        print_msg(&msg);
    }

    closesocket(sock);
    WSACleanup();
    return 0;
}
