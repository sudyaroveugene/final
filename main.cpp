#include <iostream>
#include <sys/stat.h>
#include <signal.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <unistd.h>

#include "queryparser.h"
#include "prepresponse.h"

int set_nonblock( int fd )
{
    int flags;
#if defined (O_NONBLOCK)
    if( -1==(flags=fcntl( fd, F_GETFL, 0)))
        flags = 0;
#else
    flags = 1;
    return ioctl( fd, FIOBIO, &flags );
#endif
}

static int res;
FILE *log_file;

struct sockaddr_in client_name;//--структура sockaddr_in клиентской машины (параметры ее нам неизвестны. Мы не знаем какая машина к нам будет подключаться)
int client_name_size = sizeof(client_name);//--размер структуры (тоже пока неизвестен)
int client_socket_fd;          //--идентификатор клиентского сокета

int server();

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

// открываем файл лога
    log_file = fopen( "final.log", "wb" /*"a" */);
    if( log_file==nullptr )
    {
        perror( "Error open final.log");
        return -1;
    }
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
    fprintf( log_file, "default_dir=%s port=%s ip_addr=%s\n", default_dir.data(), port.data(), ip_addr.data() );

     int pid = fork();
     if( pid<0 )
     {
         perror( "fork");
         exit( 1 );
     }
     else if( pid>0 )  // parent
     {
         return  0;
     }
     else       // child
     {
        if( setsid()<0 )
        {
            perror( "setsid");
            exit( 1 );
        }
    /* Закрываем стандартные файловые дескрипторы */
         close(STDIN_FILENO);
         close(STDOUT_FILENO);
         close(STDERR_FILENO);
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
                fd_in = open( "query.in", O_RDWR
                #if defined(__linux__)
                                                    | O_NONBLOCK
                #else
                                            | O_BINARY
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
                fd_out = open( "query.out", O_RDWR | O_CREAT | O_TRUNC
                #if defined(__linux__)
                                                    | O_NONBLOCK
                #else
                                            | O_BINARY
                #endif
                                                            , 0666 );  // O_NONBLOCK не работает в Windows
                if( fd_out<0 )
                {
                    perror( "Error open query.out\n");
                    return -1;
                }
            }

            request( fd_in, fd_out );
            sleep( 5 );
            fprintf( log_file, "Waiting request\n" );
            fflush( log_file );
        } while(1);// res!=-2 );//req.kepp_alive );  //  если соединение не закрыто, повторяем

        close( fd_in );
        close( fd_out );
        fprintf( log_file, "Server shutdown\n\n" );
        return 0;
    }
    return 0;
}
