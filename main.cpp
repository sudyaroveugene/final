#include <iostream>
//#include <stdio.h>

#include "requestparser.h"

using namespace std;

int main()
{
    struct request_string req;
    int k = parse_request_start_string( "  GET ajaja:/web-programming/index.html HTTP/1.1", req );
    parse_request_start_string("GET http://localhost:80/foo.html?&q=1:2:3", req );
    parse_request_start_string("GET https://localhost:80/foo.html?&q=1", req );
    parse_request_start_string("GET localhost/foo", req );
    parse_request_start_string("GET https://localhost/foo", req );
    parse_request_start_string("GET localhost:8080", req );
    parse_request_start_string("GET localhost?&foo=1", req );
    parse_request_start_string("GET localhost?&foo=1:2:3", req );
//    cout << "command "<<req.ncommand<<"="<< req.method << endl;
//    cout<<"URI="<<req.uri<<endl;
//    printf( "tut\n ");
    return k;
}
