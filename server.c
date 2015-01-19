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
#define MAX_MSG_LEN 4096
#define PORT 8888


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
    int inProgress;
    char lastMove[MAX_MSG_LEN];
    int lastPlayerDsc;
    int sent;
};
struct User users[CLIENTS];
struct Game games[CLIENTS/2];

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
struct User destroyUser(int dsc, struct User us[])
{
    int i;
    for(i = 0; i < CLIENTS; i++ )
    {

        if(us[i].dsc ==dsc)
        {
            us[i].dsc = 0;
            us[i].gameId = 0;
            us[i].playing = 0;
            break;
        }
    }
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
        if(us[i].dsc == dsc) return i;
    }
    return -1;
}

int canIPlayWithSomebody(int dsc, struct User us[])
{
    int playerIndex = getUserIndex(dsc, us);
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
int findFreeGameIdx(struct Game gms[])
{
    int i;
    for(i = 0; i < CLIENTS/2; i++)
    {
        if(gms[i].dsc1 == 0) return i;
    }
    return -1;
}
struct User findOpponent(int dsc, struct User us[])
{
    int playerIndex = getUserIndex(dsc, us);
    if( playerIndex > 0 && !us[playerIndex].playing)
    {
        int i;
        for(i = 0; i < CLIENTS; i++)
        {
            if(us[i].dsc && us[playerIndex].dsc != us[i].dsc && !us[i].playing) return us[i];
        }
    }
}

void createGame (int dsc1, int dsc2, struct User us[], struct Game games[])
{
    int player1Index = getUserIndex(dsc1, us);
    int player2Index = getUserIndex(dsc2, us);
    int gameId = findFreeGameIdx(games);
    if(gameId >= 0)
    {
        // printf("TWORZE GRĘ gracze: %d i %d", users[player1Index].dsc, users[player2Index]);
        games[gameId].dsc1 = dsc1;
        games[gameId].dsc2 = dsc2;
        games[gameId].lastPlayerDsc = dsc2;
        games[gameId].sent = 0;
        games[gameId].inProgress = 1;
        strcpy(games[gameId].lastMove,"@#$");
        us[player1Index].gameId = gameId;
        us[player2Index].gameId = gameId;
        us[player2Index].playing = 1;
        us[player1Index].playing = 1;

    }

}


char *protocol = "tcp";
ushort service_port = PORT;

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
    printf("rozpoczynam swe zycie");
    int rcvd;
    char buffer[MAX_MSG_LEN];
    arguments a = *((arguments*) arg);
    if(!checkIfPlayerIsLoggedIn(a.socket,users))
    {
        logIn(createUser(a.socket),users);
    }
    char c;
    int idx = getUserIndex(a.socket, users);
    struct User this = users[idx];
    if(canIPlayWithSomebody(a.socket, users))
    {
        struct User opp =  findOpponent(a.socket, users);
        createGame(a.socket, opp.dsc,users, games);
        // printf("opponent dsc %d ", opp.dsc);
        strcpy(buffer, "white");
        c='w';
        send(a.socket, buffer,  strlen( buffer ), 0);

    }
    else   //nie ma z kim grać
    {
        printf("zaczynam oczekiwanie na gracza");
        while(!this.playing)
        {
            this = users[idx];
        }
        strcpy(buffer, "black");
        c='b';
        send(a.socket, buffer,  strlen( buffer ), 0);
    }
    bzero(&buffer, sizeof buffer);
    // printf("count %d can i play %d",getUsersCount(users), canIPlayWithSomebody(a.socket, users));
    int end = 0;
    while((strcmp(buffer,"end") != 0) && (strcmp(buffer,"user_disconnected")!= 0))
    {
        //printf('')
        int gameId = users[idx].gameId;
        if(!games[gameId].inProgress)break;
        int opp;
        if(games[gameId].lastPlayerDsc != users[idx].dsc)
        {
            if(games[gameId].dsc1 == users[idx].dsc) opp = games[gameId].dsc2;
            else opp = games[gameId].dsc1;
            if(games[gameId].sent != users[idx].dsc )
            {
           // printf("ciagle tu jestem %c",c);
           bzero(&buffer, sizeof buffer);
          // printf("jestem watkiem %c bufor%s \n", c,buffer);
                while((rcvd = recv(a.socket, buffer,  MAX_MSG_LEN, 0) > 0 ))
                {
                    printf("jestem watkiem %c otrzymalem wiadomosc %s %d\n", c,buffer, strlen(buffer));
                    if(strcmp(buffer,"end") == 0 ||  strcmp(buffer,"user_disconnected")== 0) {
                        games[gameId].inProgress = 0;
                        //send(a.socket, buffer, strlen(buffer), 0);
                        //games[gameId].lastPlayerDsc = users[idx].dsc;
                        printf("jestem watkiem %c otrzymalem wiadomosc konca", c);
                    }
                    send(opp, buffer, strlen(buffer), 0);
                    printf("jestem watkiem %c wyslalem wiadomosc %s %d\n", c,buffer, strlen(buffer));

                    games[gameId].sent = users[idx].dsc;
                    games[gameId].lastPlayerDsc = users[idx].dsc;
                    bzero(&buffer, sizeof buffer);
                    if(!games[gameId].inProgress)break;
                }
            }
        }
        else
        {
            while(games[gameId].lastPlayerDsc == users[idx].dsc);
        }

    }

     printf("\n koncze. watek %c\n", c);
     destroyUser(a.socket, users);
    bzero(&buffer, sizeof buffer);
    close(a.socket);

    pthread_mutex_lock(&mutex);
    client_threads[a.index] = 0;
    pthread_mutex_unlock(&mutex);
    printf("%c client disconnected\n", c);
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
            printf("we are done\n");
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
