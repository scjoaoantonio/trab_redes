#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <openssl/sha.h>
#include <sys/time.h> 

#define PORT 8080
#define BUFFER_SIZE 1024
#define FILE_NAME "arquivo_recebido.bin"
#define SERVER_HASH_SIZE SHA256_DIGEST_LENGTH
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
    unsigned char received_hash[SERVER_HASH_SIZE];
    unsigned char computed_hash[SERVER_HASH_SIZE];
    SHA256_CTX sha256_context;
    struct timeval start, end;

    // Criar o socket TCP
    if ((client_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Erro ao criar socket");
        exit(EXIT_FAILURE);
    }

    // Configurar endereço do servidor
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    if (connect(client_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Erro ao conectar ao servidor");
        close(client_socket);
        exit(EXIT_FAILURE);
    } 


    send(client_socket, "request", strlen("request"), 0);
    printf("Solicitação enviada ao servidor.\n");

    file = fopen(FILE_NAME, "wb");
    if (!file) {
        perror("Erro ao abrir arquivo");
        close(client_socket);
        exit(EXIT_FAILURE);
    }

    // Inicializar o cálculo do hash
    SHA256_Init(&sha256_context);

    gettimeofday(&start, NULL);


    while (1) {
        int n = recv(client_socket, &packet, sizeof(Packet), 0);
        if (n < 0) {
            perror("Erro ao receber dados");
            fclose(file);
            close(client_socket);
            exit(EXIT_FAILURE);
        }

        // Verificar fim da transmissão (pacote de finalização)
        if (packet.packet_id == -1) {
            printf("Fim da transferência.\n");
            break;  
        }

        fwrite(packet.data, 1, packet.data_size, file);
        SHA256_Update(&sha256_context, packet.data, packet.data_size);
        //printf("Pacote %d recebido.\n", packet.packet_id);
    }

    // Receber o hash do arquivo do servidor
    recv(client_socket, received_hash, SERVER_HASH_SIZE, 0);

    // Finalizar o cálculo do hash no cliente
    SHA256_Final(computed_hash, &sha256_context);

    // Comparar os hashes
    if (memcmp(received_hash, computed_hash, SERVER_HASH_SIZE) == 0) {
        printf("Integridade verificada: o arquivo é válido.\n");
    } else {
        printf("Erro: a integridade do arquivo foi comprometida.\n");
    }

    gettimeofday(&end, NULL);

    double elapsed = (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1e6;
    printf("Tempo total: %.2f segundos\n", elapsed);

    printf("Taxa de download: %.2f MB/s\n", LENGTH_FILE/ elapsed);

    fclose(file);
    close(client_socket);
    return 0;
}
