#include <iostream>
#include "requestparser.h"

using namespace std;

// Разбор строки запроса
// Строка запроса выглядит так: <Метод> <URI> <HTTP/Версия протокола>
// например: GET  /web-programming/index.html  HTTP/1.1
// мы будем обрабатывать только GET и POST
// возвращаем 0, если успешно, -1 - ошибка. Результат разбора - в структуре res
int parse_request_string( std::string req, struct request_string &res )
{
    std::string methods[] = { "OPTIONS", "GET", "HEAD", "POST", "PUT", "PATCH", "DELETE", "TRACE", "LINK", "UNLINK" };
    std::string lexem;  // текущая лексема для разбора
    size_t k;

    if( req.empty() )   // пустой запрос
    {
        cerr << "Bad request: empty request" << endl;
        return -1;
    }
    for( k=0; isspace( req[k] ); k++ );    // считаем пустые символы в начале строки запроса
    if( k ) req.erase(0, k);    // удаляем их

    while( !isspace( req[k] ) ) k++;    // k - индекс первого разделителя
    if( k>=req.length() )   // сплошная строка символов вместо команды
    {
        cerr<< "Bad request: bad command"<< endl;
        return -1;
    }

    lexem = req.substr(0, k);
    int l=0;
    for( size_t i=0; i<(sizeof(methods)/sizeof(methods[0])); i++ )
    {
        l = lexem.compare(methods[i]);
        if( l == 0 )
        {
            res.method = methods[i];
            break;
        }
    }
    if( l )
    {
        cerr<<"Bad request: unknown command"<<endl;
        return -1;
    }

    return 0;
}
