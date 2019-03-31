#include <iostream>
#include <sys/stat.h>
#include <fcntl.h>

#include "queryparser.h"

//using namespace std;

int main()
{
    std::list<std::string> query, query_data;
    int fd_in = open( "query.in", O_RDWR /*| O_NONBLOCK*/, 0666 );  // O_NONBLOCK не работает в Windows
    if( fd_in<0 )
    {
        printf( "Error open query.in\n");
        return 1;
    }

    parse_query( fd_in, query, query_data );
    close( fd_in );

    return 0;
}
