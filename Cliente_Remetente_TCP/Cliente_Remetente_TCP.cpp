#define _WINSOCK_DEPRECATED_NO_WARNINGS  // Permite uso de fun��es antigas como inet_addr
#include <winsock2.h>                    // Biblioteca principal para sockets no Windows
#include <windows.h>                     // Inclui defini��es b�sicas do Windows
#include <ws2tcpip.h>                    // Fun��es adicionais para sockets (n�o muito usadas aqui)
#include <stdio.h>                       // Fun��es padr�o de entrada/sa�da
#include <stdint.h>                      // Tipos inteiros com tamanho fixo (como uint16_t)
#include <string.h>                      // Manipula��o de strings

#pragma comment(lib, "ws2_32.lib")       // Linka a biblioteca de sockets do Windows

#define SERVER_IP "127.0.0.1"            // IP do servidor (localhost)
#define SERVER_PORT 12345                // Porta do servidor

// Estrutura da mensagem usada para comunica��o
typedef struct {
    uint16_t type;       // Tipo da mensagem (OI, TCHAU, MSG, etc.)
    uint16_t orig_uid;   // UID do remetente
    uint16_t dest_uid;   // UID do destinat�rio
    uint16_t text_len;   // Tamanho do texto
    char text[141];      // Texto da mensagem (limitado a 140 caracteres + terminador)
} msg_t;

// Fun��o para limpar o buffer de entrada (stdin)
void limpa_stdin() {
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
}

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

    // Cria o socket TCP
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == INVALID_SOCKET) {
        printf("Erro no socket: %d\n", WSAGetLastError());
        return 1;
    }

    // Define os par�metros do servidor para conex�o
    server.sin_family = AF_INET;
    server.sin_port = htons(SERVER_PORT);            // Porta convertida para formato de rede
    server.sin_addr.s_addr = inet_addr(SERVER_IP);   // IP do servidor

    // Conecta ao servidor
    if (connect(sock, (struct sockaddr*)&server, sizeof(server)) < 0) {
        printf("Erro ao conectar no servidor.\n");
        return 1;
    }

    printf("Conectado ao servidor.\n");

    // Solicita UID ao usu�rio
    uint16_t meu_uid;
    printf("Digite seu UID (remetente): ");
    scanf_s("%hu", &meu_uid);

    // Prepara e envia mensagem do tipo OI
    memset(&msg, 0, sizeof(msg));
    msg.type = htons(0);         // Tipo OI
    msg.orig_uid = htons(meu_uid);
    msg.dest_uid = htons(1);     // Identifica como cliente do tipo ENVIO

    send(sock, (char*)&msg, sizeof(msg), 0);

    // Aguarda resposta do servidor
    int bytes = recv(sock, (char*)&msg, sizeof(msg), 0);
    if (bytes <= 0 || ntohs(msg.type) != 0) {
        printf("Conex�o rejeitada pelo servidor.\n");
        closesocket(sock);
        WSACleanup();
        return 1;
    }

    printf("Conex�o aceita.\n");
    // limpa_stdin();  // Pode ser ativado para evitar sobras de '\n' no buffer

    while (1) {
        char texto[141];
        uint16_t dest_uid;

        // Pede o UID do destinat�rio
        printf("\nDestino UID (ou -1 para sair): ");
        scanf_s("%hd", &dest_uid);
        limpa_stdin(); // Garante que n�o sobre lixo no buffer
        if ((int)dest_uid == -1) break;

        // L� a mensagem do usu�rio
        printf("Mensagem (at� 140 caracteres): ");
        fgets(texto, sizeof(texto), stdin);
        texto[strcspn(texto, "\n")] = 0; // Remove '\n' do final, se presente

        // Preenche estrutura da mensagem
        memset(&msg, 0, sizeof(msg));
        msg.type = htons(2);  // Tipo MSG
        msg.orig_uid = htons(meu_uid);
        msg.dest_uid = htons(dest_uid);
        msg.text_len = htons((uint16_t)strlen(texto));
        strncpy_s(msg.text, texto, sizeof(msg.text) - 1);  // Copia com seguran�a

        // Envia a mensagem ao servidor
        send(sock, (char*)&msg, sizeof(msg), 0);
    }

    // Encerra conex�o e finaliza o Winsock
    closesocket(sock);
    WSACleanup();
    return 0;
}
