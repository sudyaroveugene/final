#ifndef QUERYPARSER_H
#define QUERYPARSER_H
#include <list>
#include <string>

using namespace std;

// Строка запроса выглядит так: <Метод> <URI> <HTTP/Версия протокола>
struct query_string
{
    std::string method;
    unsigned ncommand; // порядковый номер метода(команды) в списке команд
    std::string uri;
    std::string prot_version;
};

int parse_query_start_string( std::string req, struct query_string &res );
int parse_query_header( std::list<std::string>* header );
int parse_query( int fd );
bool ReadLine(int fd, char* line, ssize_t len);

#endif // QUERYPARSER_H