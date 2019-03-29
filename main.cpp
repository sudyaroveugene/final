#include <iostream>
#include <vector>
//#include <stdio.h>

#include "queryparser.h"

using namespace std;

int main()
{
    std::vector<std::string> test_query = {
        "  GET ajaja:/web-programming/index.html HTTP/1.1",
        "GET http://localhost:80/foo.html?&q=1:2:3 HTTP/2.1",
        "GET https://localhost:80/foo.html?&q=1 HTTP/1.0",
        "GET localhost/foo HTTP/1",
        "GET https://localhost/foo HTTPS/10",
        "GET localhost:8080 HTTP/1.0",
        "GET localhost?&foo=1 HTTPS/2.1",
        "GET localhost?&foo=1:2:3 HTTPS/1.0"
    };
    struct query_string req;

    for( auto i=test_query.begin(); i!=test_query.end(); i++ )
        if( parse_query_start_string( *i, req )!=-1 )
            cout << "command "<<req.ncommand<<"="<< req.method
                 << " URI="<<req.uri<<" protocol="<<req.prot_version<< endl;

    return 0;
}
