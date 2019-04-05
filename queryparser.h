#ifndef QUERYPARSER_H
#define QUERYPARSER_H
#include <list>
#include <string>

using namespace std;

#define QUERY_MAX_LEN 2048      // максимальная длина URI
#define MAX_HEADER_SIZE 8192    // максимальный размер для заголовка, включая стартовую строку
#define DATA_STRING_LEN 1024    // будем хранить данные в строках такого размера. Данное число должно быть меньше QUERY_MAX_LEN-1

// Строка запроса выглядит так: <Метод> <URI> <HTTP/Версия протокола>
//
struct query_string
{
    std::string method;
    unsigned ncommand; // порядковый номер метода(команды) в списке команд
    std::string uri;
    std::string prot_version;
    bool kepp_alive;    // сохранять или нет соединение
    unsigned keep_alive_timeout;    // время ожидания в мс
    unsigned content_length;
    unsigned ret_code;      // код состояния для возврата, по умолчанию - 500=ошибка сервера

    void clear( void )
    {
        method.clear(); ncommand=100; uri.clear(); prot_version.clear(); keep_alive_timeout=0; kepp_alive=false; content_length=0; ret_code=500;
    }
};

extern struct query_string req;

int parse_query_start_string( std::string query_start_str );
int parse_query_header( std::list<std::string>* header );
int parse_query( int fd_in,  std::list<std::string> &query, std::list<std::string> &query_data );
size_t ReadLine(int fd, char* line, ssize_t len, int flush=0);

#endif // QUERYPARSER_H
