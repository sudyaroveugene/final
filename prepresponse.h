#ifndef PREPRESPONSE_H
#define PREPRESPONSE_H

#include "query.h"
#include <list>

using namespace std;

extern struct query_string req;

int prepare_response( std::list<std::string> &response, std::list<std::string> &response_data );
int send_response( int fd_out, std::list<std::string> &response, std::list<std::string> &response_data );

#endif // PREPRESPONSE_H
