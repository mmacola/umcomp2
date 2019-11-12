#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>

#define MAX_NAME_LENGTH 255
#define MAX_MESSAGE_LENGTH 1024
#define PORT 5000

typedef struct
{
    char name[MAX_NAME_LENGTH];
    char message[MAX_MESSAGE_LENGTH];
} client_t;

pthread_cond_t message = PTHREAD_COND_INITIALIZER;
pthread_mutex_t messageLock = PTHREAD_MUTEX_INITIALIZER;
client_t lastMessage;
int messageSize;

void *testsend(void *client_socket)
{
    int sock = *(int*)client_socket;

    pthread_mutex_lock(&messageLock);

    while (1)
    {
        pthread_cond_wait(&message, &messageLock);
        write(sock, &lastMessage, sizeof(lastMessage));
    }

    pthread_mutex_unlock(&messageLock);
}

void *connection_handler(void *client_socket)
{
    //Obtener el descriptor de socket
    int sock = *(int*)client_socket;
    int read_size;
    pthread_t sender;
    client_t client;
    read_size = recv(sock, &client, sizeof(client), 0);

    if(read_size != -1)
        printf("%s connected!\n", client.name);

    //TODO: Verifique si client.name ya está en uso

    pthread_create(&sender, NULL, testsend, (void*)&sock);

    //Recibe un mensaje del cliente

    while((read_size = recv(sock, &client, sizeof(client), 0)) > 0)
    {
        pthread_mutex_lock(&messageLock);
        messageSize=read_size;
        strncpy(lastMessage.message, client.message, MAX_MESSAGE_LENGTH);
        strncpy(lastMessage.name, client.name, MAX_NAME_LENGTH);
        pthread_cond_broadcast(&message);
        pthread_mutex_unlock(&messageLock);
    }

    if(read_size == 0)
    {
        printf("%s disconnected\n", client.name);
        fflush(stdout);
    }
    else if(read_size == -1)
        perror("recv failed");

    return 0;
}

void sig_handler(int sig)
{
    char c;

    signal(sig, SIG_IGN);
    printf("¿Presionaste Ctrl-C?\n¿Realmente quieres salir? [y / n]");

    c = getchar();
    if(c == 'y' || c == 'Y')
        exit(0);
    else
        signal(SIGINT, sig_handler);

    getchar(); // Obtener new line character
}

int main(int argc, char *argv[])
{
    int socket_desc, client_sock, c;
    struct sockaddr_in server, client;
    signal(SIGINT, sig_handler);

    //Mensaje de Inicialización
    memset(lastMessage.message, 0, MAX_MESSAGE_LENGTH);

    //Crear Socket
    socket_desc = socket(AF_INET, SOCK_STREAM, 0);

    if(socket_desc == -1)
    {
        perror("No se pudo crear el socket");
        exit(1);
    }

    printf("Socket creadon");

    //Preparar la estructura sockaddr_in
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(PORT);

    //Bind(enlazar)
    if(bind(socket_desc,(struct sockaddr *)&server, sizeof(server)) < 0)
    {
        //Imprimir mensaje de error
        perror("Bind fallo. Error!");
        return 1;
    }

    printf("Bind hecho!\n");

    //Listen
    listen(socket_desc, 3);

    //Aceptar coneccion entrante
    printf("Esperando conecciones entrantes ...\n");
    c = sizeof(struct sockaddr_in);

    while((client_sock = accept(socket_desc, (struct sockaddr*)&client, (socklen_t*)&c)))
    {
        printf("Coneccion aceptada\n");

        pthread_t sniffer_thread;

        if(pthread_create(&sniffer_thread, NULL, connection_handler, (void*) &client_sock) < 0)
        {
            perror("No se pudo crear hilo!");
            return 1;
        }

        // Ahora únete al hilo, para que no terminemos antes del hilo
        printf("Manejador asignado\n");
    }

    if(client_sock < 0)
    {
        perror("accept falló");
        return 1;
    }

    return 0;
}
