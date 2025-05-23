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

void limpa_stdin() {
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
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
    printf("Digite seu UID (remetente): ");
    scanf_s("%hu", &meu_uid);

    // Envia OI
    memset(&msg, 0, sizeof(msg));
    msg.type = htons(0); // OI
    msg.orig_uid = htons(meu_uid);
    msg.dest_uid = htons(1); // EXIBIDOR

    send(sock, (char*)&msg, sizeof(msg), 0);

    // Aguarda resposta OI
    int bytes = recv(sock, (char*)&msg, sizeof(msg), 0);
    if (bytes <= 0 || ntohs(msg.type) != 0) {
        printf("Conexão rejeitada pelo servidor.\n");
        closesocket(sock);
        WSACleanup();
        return 1;
    }

    printf("Conexão aceita.\n");
    //limpa_stdin();

    while (1) {
        char texto[141];
        uint16_t dest_uid;

        printf("\nDestino UID (ou -1 para sair): ");
        scanf_s("%hd", &dest_uid);
        limpa_stdin();
        if ((int)dest_uid == -1) break;

        printf("Mensagem (até 140 caracteres): ");
        fgets(texto, sizeof(texto), stdin);
        texto[strcspn(texto, "\n")] = 0; // remove \n

        memset(&msg, 0, sizeof(msg));
        msg.type = htons(2); // MSG
        msg.orig_uid = htons(meu_uid);
        msg.dest_uid = htons(dest_uid);
        msg.text_len = htons((uint16_t)strlen(texto));
        strncpy_s(msg.text, texto, sizeof(msg.text) - 1);

        send(sock, (char*)&msg, sizeof(msg), 0);
    }

    closesocket(sock);
    WSACleanup();
    return 0;
}
