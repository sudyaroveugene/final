#include <iostream>
#include <stdio.h>

#include "requestparser.h"

using namespace std;

int main()
{
    struct request_string req;
    int k = parse_request_string( "   GET /web-programming/index.html HTTP/1.1", req );
    cout << req.method << endl;
//    printf( "tut\n ");
    return k;
}
