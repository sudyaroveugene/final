#include <iostream>
#include <regex>
#include <cctype>
//#include <unistd.h>
#include <io.h>
#include <stdlib.h>
#include "queryparser.h"

//using namespace std;

#define METHOD_GET 0
#define QUERY_MAX_LEN 2048
static struct query_string req;

// Разбор строки запроса
// Строка запроса выглядит так: <Метод> <URI> <HTTP/Версия протокола>
// например: GET  /web-programming/index.html  HTTP/1.1
// мы будем обрабатывать только GET и POST
// возвращаем 0, если успешно, <0 - ошибка. Результат разбора - в структуре res
int parse_query_start_string( std::string query_start_str )
{
//  Валидатор для URI ( https://tools.ietf.org/html/rfc3986#appendix-B )
// URI = scheme ":" hier-part [ "?" query ] [ "#" fragment ]
// ^(([^:/?#]+):)?(//([^/?#]*))?([^?#]*)(\?([^#]*))?(#(.*))?
    std::regex URI_regex("^(([^:/?#]+):)?(//([^/?#]*))?([^?#]*)(\\?([^#]*))?(#(.*))?");
// список методов(команд) сервера. Все команды кроме GET и POST обрабатываются как ошибочные
    std::string methods[] = { "GET", "POST", "HEAD", "PUT", "PATCH", "DELETE", "TRACE", "LINK", "OPTIONS", "UNLINK" };
    std::string lexem;  // текущая лексема для разбора
    size_t k;   // номер анализируемого символа в строке запроса

    if( query_start_str.empty() )   // пустой запрос
    {
        cerr << "Bad query: empty query" << endl;
        return -1;
    }
    for( k=0; isspace( query_start_str[k] ); k++ );    // считаем пустые символы в начале строки запроса
    if( k ) query_start_str.erase(0, k);    // удаляем их

    for( k=0; !isspace( query_start_str[k] ); k++ );    // k - индекс первого разделителя
    if( k>=query_start_str.length() )   // сплошная строка символов вместо команды
    {
        cerr<< "Bad query: bad command"<< endl;
        return -1;
    }

    lexem = query_start_str.substr(0, k);       // получили первую лексему
    int l=1;
    for( size_t i=0; i<(sizeof(methods)/sizeof(methods[0])); i++ )
    {
        l = lexem.compare(methods[i]);  // ищем ее в списке методов (команд)
        if( l == 0 )
        {
            req.method = methods[i];        // заносим найденую команду в результат
            req.ncommand = static_cast<unsigned>(i);    // номер команды
            if( i>1 ) l=1;  // все команды кроме GET и POST не обрабатываем
            break;
        }
    }
    if( l )
    {
        cerr<<"Bad query: Not Implemented "<< lexem<<endl;
        return -501;
    }

    while( isspace( query_start_str[k] ) ) k++;    // пропускаем пустые символы после команды
    if( k<query_start_str.length() ) query_start_str.erase(0, k);    // удаляем их вместе с разобранной командой
    for( k=0; !isspace( query_start_str[k] ); k++ );    // считаем символы до разделителя
    if( k>=query_start_str.length() )   // сплошная строка символов вместо URI
    {
        cerr<< "Bad query: missing protocol"<< endl;
        return -1;
    }
    lexem = query_start_str.substr(0, k);       // получили URI
    std::cmatch uri_group;      // массив строк с группами URI
//	группа 2 — схема обращения к ресурсу (часто указывает на сетевой протокол), например http, ftp, file, ldap, mailto, urn
//	группа 4 — источник=адрес сервера,
//	группа 5 — путь, содержит путь к ресурсу
//	группа 7 — запрос,
//	группа 9 — фрагмент.
    l = regex_match( lexem.c_str(), uri_group, URI_regex );
    if( !l )
    {
        cerr<< "Bad query: bad URI"<< endl;
        return -1;
    }
    req.uri = lexem;

    while( isspace( query_start_str[k] ) ) k++;    // пропускаем пустые символы после команды
    if( k<query_start_str.length() ) query_start_str.erase(0, k);    // удаляем их вместе с URI. В запросе остался только протокол
// проверяем протокол
    if( query_start_str.find( "HTTP/1.") == std::string::npos )     // протокол не HTTP/1
    {
        cerr<< "Bad query: wrong protocol "<< query_start_str<<endl;
        return -1;
    }
    if( !query_start_str[7] || query_start_str[7] != '0' ) // версия протокола не 1.0
    {
        cerr<< "Bad query: wrong protocol version 1."<< query_start_str[7]<<endl;
        return -1;
    }
    req.prot_version = query_start_str;

//    cout<<"start line OK"<<endl;
    return 0;
}

// Разбор заголовков запроса
// Заголовок выглядит так последовательность строк вида: <параметр> <:> <значение>
// например: "Server: Apache/2.2.3 (CentOS)"
// получаем заголовок в виде ссылки на вектор строк
// так как заголовки мы не используем, то просто проверяем на верность структуру заголовка
// возвращаем 0, если успешно, -1 - ошибка.
int parse_query_header( std::list<std::string>* header )
{
    int res;
    size_t find_pos;

    if( !header )
    {
        cerr<< "Bad header: NULL header"<<endl;
        return -1;
    }
// разбираем стартовую строку
    res = parse_query_start_string( header->front() );
    if( res<0 )
        return res;  // ошибка в стартовой строке
    else    // печать для отладки
        cout << "command "<<req.ncommand<<"="<< req.method
             << " URI="<<req.uri<<" protocol="<<req.prot_version<< endl;

    req.kepp_alive = false; // по умолчанию - не сохранять соединение
// проверяем остальные строки заголовка
    auto i=header->begin();
    i++;
    while( i!=header->end() )
    {
        res = 0;
        string tmp = i->data();
// убираем из строки лишние пробелы: несколько пробелов подряд=один пробел
        for( auto cur_ch=i->begin(); cur_ch!=i->end(); cur_ch++ )
        {
            if( isspace(*cur_ch) )
            {
                auto next_ch=cur_ch+1;
                while( next_ch!=i->end() && isspace(*next_ch) )
                    i->erase(next_ch);
            }
// конвертируем текущую строку в нижний регистр
            *cur_ch = static_cast<char>( tolower( *cur_ch ));
            if( *cur_ch == ':' )
                res = 1;
        }
        if( !res )   // в строке отсутствует двоеточие
        {
            i->pop_back();  // убираем пeревод строки для нормальной печати ошибки
            cerr<< "Bad header: incorrect parameter \""<< i->data()<<"\" will be ignored"<<endl;
            header->erase(i--);     // удаляем ошибочную строку из запроса
            i++;
            continue;
        }
//  присутствует параметр Content-Length - есть тело сообщения
        find_pos = i->find("content-length");
        if( find_pos==0 ) // строка начинается с
        {
            if( req.ncommand == METHOD_GET )    // если команда не GET, то это ошибочная команда
            {
                i->pop_back();  // убираем превод строки
                cerr<< "Bad header: incorrect parameter \""<< i->data()<<"\" with GET command ignored"<<endl;
                header->erase(i--);     // удаляем ошибочную строку из запроса
                i++;
                continue;
            }
            find_pos=i->find(":", find_pos+14);     // ищем ':' после заголовка
            if( find_pos!=std::string::npos )
            {
                char *ch = const_cast<char*>(i->data()+find_pos+1); // указатель на след. символ
                req.content_length = ::stoul( ch );     // преобразуем найденую строку в число
                cout << "Content-length: "<< req.content_length<<endl;   // печать для отладки
            }
            else
            {
                i->pop_back();  // убираем превод строки
                cerr<< "Bad header: incorrect parameter Content-Lengt without value ignored"<<endl;
                header->erase(i--);     // удаляем ошибочную строку из запроса
                i++;
                continue;
            }
        }
//  присутствует параметр Transfer-Encoding - есть тело сообщения
        if( (i->find("transfer-encoding")==0) && req.ncommand==METHOD_GET )
        {
                // если команда не GET, то это ошибочная команда
            i->pop_back();  // убираем превод строки
            cerr<< "Bad header: incorrect parameter \""<< i->data()<<"\" with GET command ignored"<<endl;
            header->erase(i--);     // удаляем ошибочную строку из запроса
            i++;
            continue;
        }
//  присутствует параметр Connection : (Keep-Alive?) - сохранять соединение
        if( (i->find("connection")!=std::string::npos ) && (i->find("keep-alive")!=std::string::npos) )
        {
            req.kepp_alive = true;
            cout << "Connection: Keep-Alive header found"<< endl;   // печать для отладки
        }
        find_pos=i->find("keep-alive");
        if( find_pos==0 )   // ищем заголовок keep-alive
        {
            find_pos=i->find(":", find_pos+10);     // ишем ':' после заголовка
            if( find_pos!=std::string::npos )
            {
                char *ch = const_cast<char*>(i->data()+find_pos+1); // указатель на след. символ
                req.keep_alive_timeout = ::stoul( ch );     // преобразуем найденую строку в число
                cout << "Keep-Alive timeout: "<< req.keep_alive_timeout<< " ms"<<endl;   // печать для отладки
            }
        }
        i++;
    }
    return 0;
}

// длина строки запроса более 2000 - ошибка. Возврат сервера -414 (Request-URI Too Long)
int parse_query( int fd_in,  std::list<std::string> &query, std::list<std::string> &query_data )
{
    char str[QUERY_MAX_LEN+1];
    size_t data_length = 0;
    int res = 0;

    query.clear();
    query_data.clear();
    req.clear();
    printf( "reading from file\n" );
    while( ReadLine( fd_in, str, QUERY_MAX_LEN) )
            query.push_back( str );
// проверяем строки на наличие \n - если нет, то была слишком длинная строка и надо вернуть 414
// если строка состоит только из \n, то дальше заголовок закончился, дальше идут данные
    for( auto i=query.begin(); i!=query.end(); i++ )
    {
        if( i->back()!='\n' )
        {
            cerr<< "Bad header: Request-URI too large\n"<<i->data()<< endl;
            return -414;
        }
        if( i->length()==1 )    // строка только из '\n'
        {
            query_data.splice( query_data.begin(), query, i, query.end() ); // перемещаем все последующие элементы в список data
            query_data.pop_front();     // убираем строку только из '\n'
            break;  // уходим из цикла, т.к. i теперь неопределено
        }
    }
    res = parse_query_header( &query );
    if( req.content_length )    // проверяем длину данных на соответствие
    {
        for( auto i: query_data )
            data_length += i.length();
        if( data_length>req.content_length )
        {
            size_t dif = data_length - req.content_length;
            auto i=query_data.back();   // идем с конца
            while( dif>0 )
            {
                if( i.length()>dif )   // режем строку на разницу в размерах
                {
                    query_data.back().resize(i.length()-dif);
                    break;
                }
                else    // длины одной строки не хватает
                {
                    dif -= i.length();
                    query_data.pop_back();  // удаляем строку
                    i = query_data.back();  // и переходим к предыдущей
                }
            }
        }
    }
// отладка
    for( auto i: query_data )
        cout<<i.data();
    cout<<endl;
//
    return res;
}

// We read-ahead, so we store in static buffer
// what we already read, but not yet returned by ReadLine.
bool ReadLine(int fd, char* line, ssize_t len)
{
     static char *buffer = static_cast<char*>(malloc(1025*sizeof(char)));
     static size_t bufferlen=0;
     char tmpbuf[1025];
// Do the real reading from fd until buffer has '\n'.
     char *pos;
     ssize_t n;
     size_t i, un;

     while( (pos=strchr(buffer, '\n'))==nullptr )
     {
         n = read (fd, tmpbuf, 1024);
         if (n==0 || n==-1)
         {  // handle errors
             bufferlen=0;
             buffer = static_cast<char*>( realloc(buffer, sizeof(char)) );
             buffer[0] = '\0';
             line = buffer;
             return false;
         }
         un = static_cast<size_t>(n);
         tmpbuf[un] = '\0';
         buffer = static_cast<char*>( realloc(buffer, (bufferlen+un)*sizeof(char)) );
         for( i=0; i<un; i++ )
             buffer[bufferlen+i] = tmpbuf[i];
         bufferlen += un;
     }
// Split the buffer around '\n' found and return first part.
     if( (pos-buffer+1)>=len ) // полученная срока больше буфера для чтения
     {  // возвращаем то, что влезет
         un = static_cast<size_t>(len-1);
         memcpy(line, buffer, un);
         line[un+1] = '\0';
         memmove( buffer, buffer+un, bufferlen-un );  // остаток оставляем в буфере
         bufferlen -= un;
         buffer = static_cast<char*>( realloc(buffer, (bufferlen)*sizeof(char)) );
     }
     else    // строка входит в буфер
     {
         un = static_cast<size_t>(pos-buffer+1);
         memcpy(line, buffer, un);
         line[un] = '\0';
         if( bufferlen>un )
         {
             bufferlen -= un;
             memmove( buffer, pos+1, bufferlen );  // остаток оставляем в буфере
             buffer = static_cast<char*>( realloc(buffer, (bufferlen)*sizeof(char)) );
         }
         else
         {
             bufferlen = 0;
             buffer = static_cast<char*>( realloc(buffer, sizeof(char)) );
             buffer[0] = '\0';
         }
     }
     return true;
}
