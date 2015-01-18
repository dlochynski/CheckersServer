#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <pthread.h>

#define QSIZE 5
#define CLIENTS 1000


struct User
{
    int dsc;
    int playing;
    int gameId;
};

struct Game
{
    int dsc1;
    int dsc2;
    char lastMove[4096];
};
struct User users[CLIENTS];

int checkIfPlayerIsLoggedIn (int dsc, struct User us[])
{
    int i;
    for( i = 0; i < CLIENTS; i++)
    {
        if(us[i].dsc == dsc) return 1;
    }
    return 0;
}
struct User createUser(int dsc)
{
    struct User us;
    us.dsc = dsc;
    us.playing = 0;
    return us;
}
void logIn(struct User player, struct User us[])
{
    int i;
    for(i = 0; i < CLIENTS; i++ )
    {

        if(!us[i].dsc)
        {
            us[i] = player;
            printf("player dsc %d %d", player.dsc, us[i].dsc);
            break;
        }
    }
}

int getUsersCount(struct User us[])
{
    int count = 0, i;
    for(i = 0; i < CLIENTS; i++ )
    {
        if(us[i].dsc) count++;
    }
    return count;
}

int getUserIndex(int dsc, struct User us[])
{
    int i;
    for(i = 0; i < CLIENTS; i++ )
    {
       if(i==0) printf("\n%d %d\n", us[i].dsc, dsc);
        if(us[i].dsc == dsc) return i;
    }
    return -1;
}

int canIPlayWithSomebody(int dsc, struct User us[])
{
    int playerIndex = getUserIndex(dsc, us);
    printf("playerIndex %d", playerIndex);
    if(!us[playerIndex].playing)
    {
        int i;
        for( i = 0; i < CLIENTS; i++)
        {

            if(us[i].dsc && !us[i].playing && us[i].dsc != dsc ) return 1;
        }
    }
    return 0;
}


char *protocol = "tcp";
ushort service_port = 8888;

char* response = "Hello, this is diagnostic service\n";
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
int clientsCount = 0;
pthread_t client_threads[CLIENTS];

struct arguments
{
    int index;
    int socket;
} typedef arguments;

void* client_loop(void *arg)
{
    int rcvd;
    char buffer[1024];
    arguments a = *((arguments*) arg);
    printf("%d", a.socket);
    if(!checkIfPlayerIsLoggedIn(a.socket,users))
    {
    printf("yes");
        logIn(createUser(a.socket),users);
    }
    printf("count %d can i play %d",getUsersCount(users), canIPlayWithSomebody(a.socket, users));
    while(buffer[0]!='e' && buffer[1]!='n' && buffer[2]!='d')
    {
        rcvd = recv(a.socket, buffer, 1024, 0);
        send(a.socket, buffer, rcvd, 0);
    }
    close(a.socket);

    pthread_mutex_lock(&mutex);
    client_threads[a.index] = 0;
    pthread_mutex_unlock(&mutex);
    printf("client disconnected\n");
    pthread_exit(NULL);
}

int main(int argc,char **argv)
{

    struct sockaddr_in    server_addr, client_addr;

    int sck, rcv_sck, rcv_len;

    bzero(&server_addr, sizeof server_addr);
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(service_port);

    if ((sck = socket (PF_INET, SOCK_STREAM, IPPROTO_TCP)) <0)
    {
        perror("Nie można utwożyć gniazdka");
        exit(EXIT_FAILURE);
    }

    int opt = 1;
    setsockopt(sck, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof opt);

    if(bind(sck, (struct sockaddr*) &server_addr, sizeof server_addr) < 0)
    {
        printf("Cannot bind socket %d to %d port\n", sck, service_port);
        exit(EXIT_FAILURE);
    }

    if(listen(sck, QSIZE) < 0)
    {
        perror("Cannot listen");
        exit(EXIT_FAILURE);
    }

    while(1)
    {

        rcv_len = sizeof (struct sockaddr_in);
        if((rcv_sck = accept(sck, (struct sockaddr*) &client_addr, (socklen_t*) &rcv_len)) < 0)
        {
            perror("Error while connecting with client");
            exit(EXIT_FAILURE);
        }

        printf("client connected\n");

        int i=0;
        pthread_mutex_lock(&mutex);
        for(i=0; i<CLIENTS; i++)
        {
            if(client_threads[i] == 0)
                break;
        }
        pthread_mutex_unlock(&mutex);

        if(i == CLIENTS)
        {
            printf("kaszana\n");
            close(rcv_sck);
        }
        else
        {
            arguments a;
            a.socket = rcv_sck;
            a.index = i;
            pthread_create(&client_threads[i], NULL, client_loop, &a);
        }

    }

    close(sck);
    exit(EXIT_SUCCESS);

}
