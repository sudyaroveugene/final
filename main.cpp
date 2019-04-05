#include <iostream>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "queryparser.h"
#include "prepanswer.h"

struct query_string req;

//using namespace std;

int main()
{
    std::list<std::string> query, query_data;       // запрос и данные запроса
    std::list<std::string> answer, answer_data;     //  ответ, данные ответа
    int fd_in=-1, fd_out=-1;

    do
    {
        query.clear(); query_data.clear();
        answer.clear(); answer_data.clear();
        if( !req.kepp_alive )
        {
            close( fd_in );
            close( fd_out );
            fd_in=-1;
            fd_out=-1;
        }
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
        parse_query( fd_in, query, query_data );    // получаем запрос
        prepare_answer( answer, answer_data );      // подготавливаем ответ
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
        send_answer( fd_out, answer, answer_data );     // отсылаем ответ
    } while( req.kepp_alive );  //  если соединение не закрыто, повторяем

    close( fd_in );
    close( fd_out );

    return 0;
}
