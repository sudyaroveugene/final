#ifndef QUERYPARSER_H
#define QUERYPARSER_H

#include <list>
#include "query.h"

using namespace std;

#define QUERY_MAX_LEN 2048      // максимальная длина URI
#define MAX_HEADER_SIZE 8192    // максимальный размер для заголовка, включая стартовую строку
#define DATA_STRING_LEN 76      // будем хранить данные в строках такого размера

// в формате base64
                                // для кодированной в base64 строки максимальная длина 76 символов минус <CR><LF>
                                // 54/3*4 = 72 + CRLF = 74 <76

extern struct query_string req;

int parse_query_start_string( std::string query_start_str );
int parse_query_header( std::list<std::string>* header );
int parse_query( int fd_in,  std::list<std::string> &query, std::list<std::string> &query_data );
size_t ReadLine(int fd, char* line, ssize_t len, int flush=0);

#endif // QUERYPARSER_H
