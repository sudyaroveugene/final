#ifndef QUERY_H
#define QUERY_H

#include <string>
#include <list>
#include <vector>

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
        method_name.clear(); method_num=100; uri.clear(); keep_alive_timeout=30; kepp_alive=true; content_length=0; ret_code=200;
        content_type="text/html; charset=utf-8\r\n";
//  kepp_alive=false для HTTP 1.0
    }
};

#define METHOD_GET 0
#define METHOD_POST 1
#define DATA_STRING_LEN 76      // будем хранить данные в строках такого размера
// в формате base64
                                // для кодированной в base64 строки максимальная длина 76 символов минус <CR><LF>
                                // 54/3*4 = 72 + CRLF = 74 <76

typedef std::list< std::vector<uint8_t>> data_type;

#endif // QUERY_H
