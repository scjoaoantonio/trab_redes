#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>

#define PORT 8080
#define BUFFER_SIZE 1024
#define FILE_NAME "arquivo.bin"

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

    // Criar o socket UDP
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("Falha ao criar o socket");
        exit(EXIT_FAILURE);
    }

    // Configurar o endereço do servidor
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    // Associar o socket ao endereço e porta
    if (bind(sockfd, (const struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Falha no bind");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    printf("Servidor UDP rodando na porta %d...\n", PORT);

    addr_len = sizeof(client_addr);

    recvfrom(sockfd, &packet, sizeof(packet), 0, (struct sockaddr *)&client_addr, &addr_len);
    printf("Solicitação de arquivo recebida do cliente.\n");

    file = fopen(FILE_NAME, "rb");
    if (!file) {
        perror("Erro ao abrir o arquivo");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    int packet_id = 0;
    size_t bytes_read;
    while ((bytes_read = fread(packet.data, 1, BUFFER_SIZE, file)) > 0) {
        packet.packet_id = packet_id++; 
        packet.data_size = bytes_read;  

        sendto(sockfd, &packet, sizeof(packet), 0, (struct sockaddr *)&client_addr, addr_len);
        printf("Pacote %d enviado (%ld bytes).\n", packet.packet_id, bytes_read);
    }

    // Enviar pacote de finalização
    packet.packet_id = -1; 
    packet.data_size = 0;   
    sendto(sockfd, &packet, sizeof(packet), 0, (struct sockaddr *)&client_addr, addr_len);
    printf("Transferência concluída.\n");

    fclose(file);
    close(sockfd);
    return 0;
}
