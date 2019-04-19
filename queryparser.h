#ifndef QUERYPARSER_H
#define QUERYPARSER_H

#include <list>
#include "query.h"

using namespace std;

#define QUERY_MAX_LEN 2048      // максимальная длина URI
#define MAX_HEADER_SIZE 8192    // максимальный размер для заголовка, включая стартовую строку

int parse_query_start_string( std::string query_start_str, struct query_string &req );
int parse_query_header( std::list<std::string>* header, struct query_string &req );
int parse_query( int fd_in,  std::list<std::string> &query, data_type &query_data, struct query_string &req );
size_t ReadLine(int fd, char* line, ssize_t len, int flush=0);

#endif // QUERYPARSER_H
