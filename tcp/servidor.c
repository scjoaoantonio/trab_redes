
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <openssl/sha.h>

#define PORT 8080
#define BUFFER_SIZE 1024
#define FILE_NAME "arquivo.bin"
#define CLIENT_HASH_SIZE SHA256_DIGEST_LENGTH

typedef struct {
    int packet_id;            
    size_t data_size;          
    char data[BUFFER_SIZE];    
} Packet;

int main() {
    int sockfd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_len;
    FILE *file;
    Packet packet;
    unsigned char file_hash[CLIENT_HASH_SIZE];
    SHA256_CTX sha256_context;

    // Criar o socket TCP
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Falha ao criar o socket");
        exit(EXIT_FAILURE);
    }

    // Configurar o endereço do servidor
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(sockfd, (const struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Falha no bind");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    if (listen(sockfd, 1) < 0) {
        perror("Falha no listen");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    printf("Servidor aguardando conexões...\n");

    addr_len = sizeof(client_addr);
    int new_sock = accept(sockfd, (struct sockaddr *)&client_addr, &addr_len);
    if (new_sock < 0) {
        perror("Falha ao aceitar conexão");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    printf("Cliente conectado.\n");

    recv(new_sock, &packet, sizeof(packet), 0);
    printf("Solicitação de arquivo recebida.\n");

    file = fopen(FILE_NAME, "rb");
    if (!file) {
        perror("Erro ao abrir o arquivo");
        close(new_sock);
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    // Iniciar o cálculo do hash
    SHA256_Init(&sha256_context);

    int packet_id = 0;
    size_t bytes_read;
    while ((bytes_read = fread(packet.data, 1, BUFFER_SIZE, file)) > 0) {
        packet.packet_id = packet_id++; 
        packet.data_size = bytes_read;  
        send(new_sock, &packet, sizeof(packet), 0);
        SHA256_Update(&sha256_context, packet.data, bytes_read);
        printf("Pacote %d enviado (%ld bytes).\n", packet.packet_id, bytes_read);
    }

    // Enviar pacote de finalização
    packet.packet_id = -1;
    packet.data_size = 0;
    send(new_sock, &packet, sizeof(packet), 0);

    // Finalizar o cálculo do hash no servidor
    SHA256_Final(file_hash, &sha256_context);

    // Enviar o hash do arquivo ao cliente
    send(new_sock, file_hash, CLIENT_HASH_SIZE, 0);

    printf("Hash do arquivo enviado.\n");

    fclose(file);
    close(new_sock);
    close(sockfd);

    return 0;
}
