#include <iostream>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "queryparser.h"
#include "prepresponse.h"

static int res;

void* request( int fd_in, int fd_out )
{
    std::list<std::string> query, response;       // запрос и данные запроса/ответа
    data_type query_data;                //  ответ
    struct query_string req;

    query.clear(); query_data.clear();
    response.clear();
    res = parse_query( fd_in, query, query_data, req );    // получаем запрос
    if( res == -2 )    // на входе нет запросов
        return nullptr;  // не нуль
    prepare_response( response, query_data, req );      // подготавливаем ответ
    send_response( fd_out, response, query_data );     // отсылаем ответ

    return nullptr;
}

int main( int argc, char** argv )
{
    int fd_in=-1, fd_out=-1;
    std::string ip_addr, default_dir, port;
    int optnum;

// задаем параметры по умолчанию
    default_dir = getcwd( nullptr, 0 );
    port = "12345";
    ip_addr = "localhost";
// читаем параметры из командной строки
    while( (optnum=getopt( argc, argv, "p:h:d:"))>0 )
    {
        switch( optnum )
        {
            case 'p':
                port = optarg; break;
            case 'h':
                ip_addr = optarg; break;
            case 'd':
                default_dir = optarg; break;
        }
    }
//    printf( "default_dir=%s port=%s ip_addr=%s\n", default_dir.data(), port.data(), ip_addr.data() );

    do
    {
//        if( !req.kepp_alive )
//        {
//            close( fd_in );
//            close( fd_out );
//            fd_in=-1;
//            fd_out=-1;
//        }
        if( fd_in<0 )
        {
            fd_in = open( "query.in", O_RDWR | O_BINARY
            #if defined(__linux__)
                                                | O_NONBLOCK
            #endif
                                                        , 0666 );  // O_NONBLOCK не работает в Windows
            if( fd_in<0 )
            {
                perror( "Error open query.in");
                return -1;
            }
        }
        if( fd_out<0 )
        {
            fd_out = open( "query.out", O_RDWR | O_CREAT | O_TRUNC | O_BINARY
            #if defined(__linux__)
                                                | O_NONBLOCK
            #endif
                                                        , 0666 );  // O_NONBLOCK не работает в Windows
            if( fd_out<0 )
            {
                perror( "Error open query.out\n");
                return -1;
            }
        }

        request( fd_in, fd_out );
    } while( res!=-2 );//req.kepp_alive );  //  если соединение не закрыто, повторяем

    close( fd_in );
    close( fd_out );

    return 0;
}
