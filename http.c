#include <stdbool.h>
#include <stdio.h>
#include <ws2tcpip.h>
#include <winsock2.h>
#include <windows.h>
#include <string.h>
#include <stdlib.h>

#pragma comment(lib, "Ws2_32.lib")

#define PORT 8080
char raspuns_html[356];
static int citit = 0;
static int citit_tot = 0;
#define Size_buffer 4096

char const give_mime(const char *filePath) {
    const char *tmp = strchr(filePath, '.');
    if (strcmp(tmp, ".html") == 0) return "index/html";
    if (strcmp(tmp, "jpg") == 0 || strcmp(tmp, ".jpeg") == 0) return "image/jpeg";
}

void trimitere_pe_chunckuri(SOCKET S, const char *file_Path, const char *mimetype) {
    FILE *file = fopen(file_Path, "rb");
    if (file == 0) {
        perror("Eroare la fisier");
        return;
    }

    const char *header[256];
    sprintf(header,
        "HTTP/1.1 200 OK\r\n"
        "Transfer-Encoding:chunked\r\n"
        "Content-Type:text/html\r\n"
        "Connection: close\r\n"
        "\r\n", mimetype);
    send(S, header, strlen(header), 0);

    char buffer[Size_buffer];
    size_t byte_read;

    char sendbuffer[Size_buffer + 64];

    while ((byte_read = fread(buffer, 1, Size_buffer, file)) > 0) {
        int headerlen = sprintf(sendbuffer, "%X\r\n", (unsigned int)byte_read);
        memcpy(sendbuffer + headerlen, buffer, byte_read);
        memcpy(sendbuffer + headerlen + byte_read, "\r\n", 2);
        int total_len = headerlen + byte_read + 2;
        send(S, sendbuffer, total_len, 0);
    }
    send(S, "0\r\n\r\n", 5, 0);
    fclose(file);
}

void real_http_handler(SOCKET s) {
    char recvbuffer[Size_buffer];
    int recvlen = recv(s, recvbuffer, sizeof(recvbuffer) - 1, 0);
    if (recvlen < 0) {
        perror("failed to receive data");
        return;
        closesocket(s);
    }
    recvbuffer[recvlen] = '\0';

    char method[16], path[500], protocol[16];
    sscanf(recvbuffer, "%s %s %s ", method, path, protocol);
    char filepath[556];
    if (strcmp(path, "/") == 0) {
        strcpy(filepath, "C:\\Users\\user\\CLionProjects\\http_server\\post.html");
    } else {
        sprintf(filepath, "C:\\Users\\user\\CLionProjects\\http_server\\%s", path + 1);
    }
    const char *mimetype = give_mime(filepath);
    trimitere_pe_chunckuri(s, filepath, mimetype);
    closesocket(s);
}

void chat(SOCKET s) {
    char buff[5000];
    memset(buff, 0, sizeof(buff));
    int recvline = recv(s, buff, sizeof(buff) - 1, 0);
    printf("%s\n", buff);
    if (recvline <= 0) {
        closesocket(s);
        return;
    }
    buff[recvline] = '\0';

    char method[16];
    sscanf(buff, "%s", method);

    if (strcmp(method, "GET") == 0) {
        send(s, raspuns_html, strlen(raspuns_html), 0);
    } else if (strcmp(method, "OPTIONS") == 0) {
        const char *optionsResponse =
            "HTTP/1.1 200 OK\r\n"
            "Access-Control-Allow-Origin: *\r\n"
            "Access-Control-Allow-Methods: GET, POST, OPTIONS\r\n"
            "Access-Control-Allow-Headers: Content-Type\r\n"
            "Content-Length: 0\r\n"
            "\r\n";
        send(s, optionsResponse, strlen(optionsResponse), 0);
    } else if (strcmp(method, "POST") == 0) {
        const char *postResponse =
            "HTTP/1.1 200 OK\r\n"
            "Access-Control-Allow-Origin: *\r\n"
            "Content-Type: application/json\r\n"
            "Content-Length: 20\r\n"
            "\r\n"
            "{\"status\":\"ok\"}";
        send(s, postResponse, strlen(postResponse), 0);
    }

    closesocket(s);
}

int main() {
    WSADATA wsa;
    SOCKET s, clientSocket;
    struct sockaddr_in server, clientAddr;

    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        printf("Failed to initialise winsock library");
        return 1;
    }
    printf("Good to go\n");

    if ((s = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET) {
        printf("Unable to create the socket");
    }

    memset(&server, 0, sizeof(server));
    server.sin_addr.s_addr = htonl(INADDR_ANY);
    server.sin_family = AF_INET;
    server.sin_port = htons(PORT);

    if (bind(s, (struct sockaddr_in*)&server, sizeof(server)) != 0) {
        printf("Something went wrong with binding :(( ");
        closesocket(s);
        WSACleanup();
        return 1;
    }

    if (listen(s, 5) != 0) {
        printf("Serverul rejected connections");
    }

    while (true) {
        int clientAddrSize = sizeof(clientAddr);
        clientSocket = accept(s, (struct sockaddr*)&clientAddr, &clientAddrSize);
        if (clientSocket == INVALID_SOCKET) {
            printf("Accept failed. Error code: %d\n", WSAGetLastError());
            closesocket(s);
            WSACleanup();
            return 1;
        }

        real_http_handler(clientSocket);
    }

    return 0;
}
