#ifndef QUERYPARSER_H
#define QUERYPARSER_H

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

#endif // QUERYPARSER_H
