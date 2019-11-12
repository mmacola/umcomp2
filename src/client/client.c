#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <pthread.h>

#define MAX_NAME_LENGTH 255
#define MAX_MESSAGE_LENGTH 1024
#define PORT 5000

typedef struct
{
    char name[MAX_NAME_LENGTH];
    char message[MAX_MESSAGE_LENGTH];
} client_t;

typedef struct
{
    int socket;
    client_t clientInfo;
} threadArg_t;

void *listenThread(void *client_info)
{
    threadArg_t arguments = *(threadArg_t*)client_info;
    int sock = arguments.socket;
    client_t server_reply;

    //Receive a reply from the server
    while(1)
    {
        if(recv(sock, &server_reply, sizeof(client_t), 0) < 0)
        {
            printf("Recv falló!\n");
            break;
        }

        if(strcmp(server_reply.name, arguments.clientInfo.name) != 0)
        {
            printf("[%s]: %s\n", server_reply.name, server_reply.message);
        }

        memset(server_reply.message, 0, MAX_MESSAGE_LENGTH);
    }

    return 0;
}

int main(int argc, char *argv[])
{
    int sock;
    struct sockaddr_in server;
    client_t cl;

    if(argc < 2)
    {
        perror("usage: ./client nickname\n");
        return 1;
    }

    strcpy(cl.name, argv[1]);

    //Create socket
    sock = socket(AF_INET, SOCK_STREAM, 0);

    if(sock == -1)
        printf("No se pudo crear el socket\n");

    printf("Socket creado\n");

    server.sin_addr.s_addr = inet_addr("127.0.0.1");
    server.sin_family = AF_INET;
    server.sin_port = htons(PORT);

    //Conectarse al server remoto
    if(connect(sock, (struct sockaddr *)&server, sizeof(server)) < 0)
    {
        perror("connect failed. Error\n");
        return 1;
    }
    else
    {
        client_t temp;
        strcpy(temp.name, argv[1]);
        send(sock, &temp, sizeof(temp), 0);
    }

    printf("Conectado\n");

    pthread_t listener;
    threadArg_t listenerArgument;
    listenerArgument.socket = sock;
    listenerArgument.clientInfo = cl;
    pthread_create(&listener, NULL, listenThread, (void*)&listenerArgument);

    //seguir comunicándose con el server
    while(1)
    {
        printf("[%s]: ", cl.name);
        scanf("%s", cl.message);

        if(strlen(cl.message) > MAX_MESSAGE_LENGTH)
        {
            printf("Max length execedeed!\n");
            continue;
        }

        //Enviar algunos datos
        if(send(sock, &cl, sizeof(cl), 0) < 0)
        {
            printf("Envío fallido!\n");
            return 1;
        }

        memset(cl.message, 0, MAX_MESSAGE_LENGTH);
    }

    close(sock);
    return 0;
}
