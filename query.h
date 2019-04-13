#ifndef QUERY_H
#define QUERY_H

#include <string>

// Строка запроса выглядит так: <Метод> <URI> <HTTP/Версия протокола>
//
struct query_string
{
    std::string method_name;
    unsigned method_num; // порядковый номер метода(команды) в списке команд
    std::string uri;
    std::string content_type;
    unsigned content_length;
    bool kepp_alive;    // сохранять или нет соединение
    unsigned keep_alive_timeout;    // время ожидания в секундах
    unsigned ret_code;      // код состояния для возврата, по умолчанию - 500=ошибка сервера

    void clear( void )
    {
        method_name.clear(); method_num=100; uri.clear(); keep_alive_timeout=0; kepp_alive=false; content_length=0; ret_code=200;
        content_type="text/html; charset=utf-8";
//  kepp_alive=false для HTTP 1.0
    }
};

#define METHOD_GET 0
#define METHOD_POST 1

#endif // QUERY_H
