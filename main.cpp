#include <iostream>
#include <fcntl.h>

#include "queryparser.h"

//using namespace std;

int main()
{
    int fd_in = open( "query.in", O_RDWR, 0666 );
    if( fd_in<0 )
    {
        printf( "Error open query.in\n");
        return 1;
    }

    parse_query( fd_in );
    close( fd_in );

    return 0;
}
