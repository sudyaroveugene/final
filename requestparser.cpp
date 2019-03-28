#include <iostream>
#include <regex>
#include "requestparser.h"

using namespace std;

//  Валидатор для URI ( https://tools.ietf.org/html/rfc3986#appendix-B )
// URI = scheme ":" hier-part [ "?" query ] [ "#" fragment ]
// ^(([^:/?#]+):)?(//([^/?#]*))?([^?#]*)(\?([^#]*))?(#(.*))?
static std::regex URI_regex("^(([^:/?#]+):)?(//([^/?#]*))?([^?#]*)(\\?([^#]*))?(#(.*))?");

// Разбор строки запроса
// Строка запроса выглядит так: <Метод> <URI> <HTTP/Версия протокола>
// например: GET  /web-programming/index.html  HTTP/1.1
// мы будем обрабатывать только GET и POST
// возвращаем 0, если успешно, -1 - ошибка. Результат разбора - в структуре res
int parse_request_start_string( std::string req, struct request_string &res )
{
    std::string methods[] = { "GET", "POST", "HEAD", "PUT", "PATCH", "DELETE", "TRACE", "LINK", "OPTIONS", "UNLINK" };
    std::string lexem;  // текущая лексема для разбора
    size_t k;   // номер анализируемого символа в строке запроса

    if( req.empty() )   // пустой запрос
    {
        cerr << "Bad request: empty request" << endl;
        return -1;
    }
    for( k=0; isspace( req[k] ); k++ );    // считаем пустые символы в начале строки запроса
    if( k ) req.erase(0, k);    // удаляем их

    for( k=0; !isspace( req[k] ); k++ );    // k - индекс первого разделителя
    if( k>=req.length() )   // сплошная строка символов вместо команды
    {
        cerr<< "Bad request: bad command"<< endl;
        return -1;
    }

    lexem = req.substr(0, k);       // получили первую лексему
    int l=1;
    for( size_t i=0; i<(sizeof(methods)/sizeof(methods[0])); i++ )
    {
        l = lexem.compare(methods[i]);  // ищем ее в списке методов (команд)
        if( l == 0 )
        {
            res.method = methods[i];        // заносим найденую команду в результат
            res.ncommand = static_cast<unsigned>(i);    // номер команды
            if( i>1 ) l=1;  // все команды кроме GET и POST не обрабатываем
            break;
        }
    }
    if( l )
    {
        cerr<<"Bad request: unknown command "<< lexem<<endl;
        return -1;
    }

    while( isspace( req[k] ) ) k++;    // пропускаем пустые символы после команды
    if( k<req.length() ) req.erase(0, k);    // удаляем их вместе с разобранной командой
    for( k=0; !isspace( req[k] ); k++ );    // k - индекс разделителя
    if( k>=req.length() )   // сплошная строка символов вместо URI
    {
        cerr<< "Bad request: protocol absent"<< endl;
        return -1;
    }
    lexem = req.substr(0, k);       // получили URI
    std::cmatch uri_group;      // массив строк с группами URI
//	группа 2 — схема обращения к ресурсу (часто указывает на сетевой протокол), например http, ftp, file, ldap, mailto, urn
//	группа 4 — источник=адрес сервера,
//	группа 5 — путь, содержит путь к ресурсу
//	группа 7 — запрос,
//	группа 9 — фрагмент.
    l = regex_match( lexem.c_str(), uri_group, URI_regex );
    if( !l )
    {
        cerr<< "Bad request: bad URI"<< endl;
        return -1;
    }
    res.uri = lexem;

    return 0;
}
