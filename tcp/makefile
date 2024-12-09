all: cliente servidor

cliente: cliente.c
	gcc -o cliente cliente.c -lcrypto -lssl

servidor: servidor.c
	gcc -o servidor servidor.c -lcrypto -lssl

clean: 
	rm -f cliente
	rm -f servidor
