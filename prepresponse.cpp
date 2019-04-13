#include <unistd.h>

#include "prepresponse.h"

int prepare_response( std::list<std::string> &response, std::list<std::string> &response_data )
{
    response.clear();
    response_data.clear();

    response.push_front( "HTTP/1.0\n" );

    if( req.ret_code != 200 )   // ошибка запроса уже определена при разборе
    {
        response.front().pop_back();
        switch( req.ret_code )
        {
            case 413:
                response.front().append( " 413 Request Entity Too Large\n" );
                break;
            case 414:
                response.front().append( " 414 Request-URI Too Long\n" );
                break;
            case 505:
                response.front().append( " 505 HTTP Version Not Supported\n" );
                break;
            case 501:
            default:
                response.front().append( " 501 Not Implemented\n" );
        };
        return 0;
    }

    if( req.method_num == METHOD_GET )
    {
// проверяем полученный путь

    }
    else if( req.method_num == METHOD_POST )
    {

    }
    else
    {
        response.front().pop_back();
        response.front().append( " 501 Not Implemented\n" );
    }
    return 0;
}

int send_response( int fd_out, std::list<std::string> &response, std::list<std::string> &response_data )
{
    if( fd_out<0 )
        return -1;

    for( auto i: response )
        write( fd_out, i.data(), static_cast<unsigned>(i.length()) );

    if( !response_data.empty() )
    {
        write( fd_out, "\r\n", 2 );
        for( auto i: response_data )
            write( fd_out, i.data(), static_cast<unsigned>(i.length()) );
    }
    return 0;
}
