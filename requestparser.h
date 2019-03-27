#ifndef REQUESTPARSER_H
#define REQUESTPARSER_H

// Строка запроса выглядит так: <Метод> <URI> <HTTP/Версия протокола>
struct request_string
{
    std::string method;
    std::string uri;
    std::string prot_version;
};

int parse_request_string( std::string req, struct request_string &res );

#endif // REQUESTPARSER_H
