#ifndef REQUESTPARSER_H
#define REQUESTPARSER_H

using namespace std;

// Строка запроса выглядит так: <Метод> <URI> <HTTP/Версия протокола>
struct request_string
{
    std::string method;
    unsigned ncommand; // порядковый номер метода(команды) в списке команд
    std::string uri;
    std::string prot_version;
};

int parse_request_start_string( std::string req, struct request_string &res );

#endif // REQUESTPARSER_H
