#include <iostream>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "queryparser.h"
#include "prepresponse.h"

struct query_string req;

//using namespace std;

int main()
{
    std::list<std::string> query, query_data;       // запрос и данные запроса
    std::list<std::string> response, response_data;     //  ответ, данные ответа
    int fd_in=-1, fd_out=-1;
    int res;

    do
    {
        query.clear(); query_data.clear();
        response.clear(); response_data.clear();
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
            #endif
                                                        , 0666 );  // O_NONBLOCK не работает в Windows
            if( fd_in<0 )
            {
                perror( "Error open query.in");
                return -1;
            }
        }
        res = parse_query( fd_in, query, query_data );    // получаем запрос
        if( res == -2 )    // на входе нет запросов
            break;
        prepare_response( response, response_data );      // подготавливаем ответ
        if( fd_out<0 )
        {
            fd_out = open( "query.out", O_RDWR | O_CREAT | O_TRUNC
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
        send_response( fd_out, response, response_data );     // отсылаем ответ
    } while( 1 );//req.kepp_alive );  //  если соединение не закрыто, повторяем

    close( fd_in );
    close( fd_out );

    return 0;
}
