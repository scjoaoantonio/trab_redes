#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/time.h>

#define PORT 8080
#define BUFFER_SIZE 1024
#define FILE_NAME "arquivo_recebido.bin"
#define LENGTH_FILE 10.5

typedef struct {
    int packet_id;             
    size_t data_size;          
    char data[BUFFER_SIZE];  
} Packet;

int main() {
    int client_socket;
    struct sockaddr_in server_addr;
    socklen_t addr_len = sizeof(server_addr);
    Packet packet;
    FILE *file;
    struct timeval start, end;

    // Criar o socket UDP
    if ((client_socket = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("Erro ao criar socket");
        exit(EXIT_FAILURE);
    }

    // Configurar endereço do servidor
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    sendto(client_socket, "request", strlen("request"), 0, (const struct sockaddr *)&server_addr, addr_len);
    printf("Solicitação enviada ao servidor.\n");

    file = fopen(FILE_NAME, "wb");
    if (!file) {
        perror("Erro ao abrir arquivo");
        close(client_socket);
        exit(EXIT_FAILURE);
    }

    gettimeofday(&start, NULL);

    int packet_id = 0, expected_id = 0, pacotes_recebidos = 0, pacotes_perdidos = 0;

    while (1) {
        int n = recvfrom(client_socket, &packet, sizeof(Packet), 0, (struct sockaddr *)&server_addr, &addr_len);
        if (n < 0) {
            perror("Erro ao receber dados");
            fclose(file);
            close(client_socket);
            exit(EXIT_FAILURE);
        }

        // Verificar fim da transmissão
        if (packet.packet_id == -1) {
            printf("Fim da transferência.\n");
            break;  
        }

        // Contar pacotes perdidos
        if (packet.packet_id != expected_id) {
            pacotes_perdidos += (packet.packet_id - expected_id - 1); 
            expected_id = packet.packet_id + 1; 
        }

        fwrite(packet.data, 1, packet.data_size, file);
        pacotes_recebidos++;

        expected_id = packet.packet_id + 1; 
    }

    gettimeofday(&end, NULL);
    double elapsed = (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1e6;
    printf("Tempo total: %.2f segundos\n", elapsed);

    printf("Pacotes recebidos: %d\n", pacotes_recebidos);
    printf("Pacotes perdidos: %d\n", pacotes_perdidos);
    printf("Taxa de download: %.2f MB/s\n", LENGTH_FILE / elapsed);

    fclose(file);
    close(client_socket);
    return 0;
}
