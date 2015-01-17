#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <string.h>
#include <strings.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include<iostream>
#include<typeinfo>
#include<vector>

using namespace std;

#define MAX_MSG_LEN 4096
#define SERWER_PORT 8888
#define SERWER_IP "79.173.14.107"
#define MAX_CONNECTION 10

struct User {
    string ip;
    bool playing;
};

struct Game {
    string ipFirstPlayer;
    string ipSecondPlayer;
    string lastMove;
};

vector <User> Users;
vector <Game> Games;

bool checkIfPlayerIsLoggedIn (string ip, vector<User> us) {
    for(int i = 0; i < us.size(); i++) {
        if(us[i].ip == ip) return true;
    }
    return false;
}

void logIn(User player, vector <User> &us) {
    us.push_back(player);
}

int main()
{
    struct sockaddr_in serwer;
    int gniazdo;
    char bufor[ MAX_MSG_LEN ];

    bzero( & serwer, sizeof( serwer ) );
    bzero( bufor, sizeof( bufor ) );

    serwer.sin_family = AF_INET;
    serwer.sin_port = htons( SERWER_PORT );
    if( inet_pton( AF_INET, SERWER_IP, & serwer.sin_addr ) <= 0 )
    {
        perror( "inet_pton() ERROR" );
        exit( - 1 );
    }

    if(( gniazdo = socket( AF_INET, SOCK_STREAM, 0 ) ) < 0 )
    {
        perror( "socket() ERROR" );
        exit( - 1 );
    }

    socklen_t len = sizeof( serwer );
    if( bind( gniazdo,( struct sockaddr * ) & serwer, len ) < 0 )
    {
        perror( "bind() ERROR" );
        exit( - 1 );
    }

    if( listen( gniazdo, MAX_CONNECTION ) < 0 )
    {
        perror( "listen() ERROR" );
        exit( - 1 );
    }

    while( 1 )
    {
        struct sockaddr_in from;
        int gniazdo_clienta = 0;
        bzero( & from, sizeof( from ) );
        printf( "Waiting for connection...\n" );
        if(( gniazdo_clienta = accept( gniazdo,( struct sockaddr * ) & from, & len ) ) < 0 )
        {
            perror( "accept() ERROR" );
            continue;
        }

        bzero( bufor, sizeof( bufor ) );
        if(( recv( gniazdo_clienta, bufor, sizeof( bufor ), 0 ) ) <= 0 )
        {
            perror( "recv() ERROR" );
            exit( - 1 );
        }
        printf( "|Wiadomosc od clienta|: %s \n", bufor );
        char bufor_ip[ 128 ];
        bzero( bufor_ip, sizeof( bufor_ip ) );

        string ip = string(inet_ntop( AF_INET, & from.sin_addr, bufor_ip, sizeof( bufor_ip )));
        //cout<<"test";
        if(!checkIfPlayerIsLoggedIn(ip, Users)) {
            cout<<"test";
            User us;
            us.ip = ip;
            us.playing = false;
            logIn(us, Users);
            cout << Users[0].ip;
        }
        printf( "|Client ip: %s port: %d|\n", inet_ntop( AF_INET, & from.sin_addr, bufor_ip, sizeof( bufor_ip ) ), ntohs( from.sin_port ) );
        printf( "  New connection from: %s:%d\n", inet_ntoa( from.sin_addr ), ntohs( from.sin_port ) );

        bzero( bufor, sizeof( bufor ) );
        strcpy( bufor, "Wyslane z serwera" );
        if(( send( gniazdo_clienta, bufor, strlen( bufor ), 0 ) ) <= 0 )
        {
            perror( "send() ERROR" );
            exit( - 1 );
        }

        shutdown( gniazdo_clienta, SHUT_RDWR );
    }

    shutdown( gniazdo, SHUT_RDWR );

    return 0;
}
