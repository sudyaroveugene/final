#include <iostream>
#include <regex>
#include <cctype>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#if defined(__linux__)
#include <sys/socket.h>
#endif
#include "queryparser.h"

extern FILE* log_file;

void poprn( string& str )
{
    if( str.back() == '\n' )
        str.pop_back();
    if( str.back() == '\r' )
        str.pop_back();
}

// Разбор строки запроса
// Строка запроса выглядит так: <Метод> <URI> <HTTP/Версия протокола>
// например: GET  /web-programming/index.html  HTTP/1.1
// мы будем обрабатывать только GET и POST
// возвращаем 0, если успешно, <0 - ошибка. Результат разбора - в структуре res
int parse_query_start_string( std::string query_start_str, struct query_string &req )
{
//  Валидатор для URI ( https://tools.ietf.org/html/rfc3986#appendix-B )
// URI = scheme ":" hier-part [ "?" query ] [ "#" fragment ]
// ^(([^:/?#]+):)?(//([^/?#]*))?([^?#]*)(\?([^#]*))?(#(.*))?
    std::regex URI_regex("^(([^:/?#]+):)?(//([^/?#]*))?([^?#]*)(\\?([^#]*))?(#(.*))?");
// список методов(команд) сервера. Все команды кроме GET и POST обрабатываются как ошибочные
    std::string methods[] = { "GET", "POST" };//, "HEAD", "PUT", "PATCH", "DELETE", "TRACE", "LINK", "OPTIONS", "UNLINK" };
    std::string lexem;  // текущая лексема для разбора
    size_t k;   // номер анализируемого символа в строке запроса

    if( query_start_str.empty() )   // пустой запрос
    {
        fprintf( log_file, "Bad query: empty\n");
        req.ret_code = 501;
        return 1;
    }
    for( k=0; query_start_str[k]<0 || isspace( query_start_str[k] ); k++ );    // считаем пустые символы в начале строки запроса
    if( k ) query_start_str.erase(0, k);    // удаляем их

    for( k=0; !isspace( query_start_str[k] ); k++ );    // k - индекс первого разделителя
    if( k>=query_start_str.length() )   // сплошная строка символов вместо команды
    {
        fprintf( log_file, "Bad query: bad command\n" );
        req.ret_code = 501;
        return 1;
    }

    lexem = query_start_str.substr(0, k);       // получили первую лексему
    int l=1;
    for( size_t i=0; i<(sizeof(methods)/sizeof(methods[0])); i++ )
    {
        l = lexem.compare(methods[i]);  // ищем ее в списке методов (команд)
        if( l == 0 )
        {
            req.method_name = methods[i];        // заносим найденую команду в результат
            req.method_num = static_cast<unsigned>(i);    // номер команды
            if( i>1 ) l=1;  // все команды кроме GET и POST не обрабатываем
            break;
        }
    }
    if( l )
    {
        fprintf( log_file, "Bad query: Not Implemented \"%s\"\n", lexem.data() );
        req.ret_code = 501;
        return 1;
    }

    while( isspace( query_start_str[k] ) ) k++;    // пропускаем пустые символы после команды
    if( k<query_start_str.length() ) query_start_str.erase(0, k);    // удаляем их вместе с разобранной командой
    for( k=0; !isspace( query_start_str[k] ); k++ );    // считаем символы до разделителя
    if( k>=query_start_str.length() )   // сплошная строка символов вместо URI
    {
        fprintf( log_file, "Bad query: missing protocol\n" );
        req.ret_code = 501;
        return 1;
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
        fprintf( log_file, "Bad query: bad URI\n" );
        req.ret_code = 501;
        return 1;
    }
    string quer = uri_group[7].str();
    string frag = uri_group[9].str();
    if( uri_group[7].length() || uri_group[9].length() )
    {
        fprintf( log_file, "Bad query: Not Implemented \"%s\" with query or fragment expressions in URI\n", req.method_name.data() );
        req.ret_code = 501;
        return 1;
    }
    req.uri = uri_group[5].str();

    while( isspace( query_start_str[k] ) ) k++;    // пропускаем пустые символы после команды
    if( k<query_start_str.length() ) query_start_str.erase(0, k);    // удаляем их вместе с URI. В запросе остался только протокол
// проверяем протокол
    if( query_start_str.find( "HTTP/1.") == std::string::npos )     // протокол не HTTP/1
    {
        poprn( query_start_str );
        fprintf( log_file, "Bad query: wrong protocol \"%s\"\n", query_start_str.data() );
        req.ret_code = 501;
        return 1;
    }
//    if( !query_start_str[7] || query_start_str[7] != '0' ) // версия протокола не 1.0
//    {
//        fprintf( log_file, "Bad query: HTTP Version Not Supported 1.%c\n", query_start_str[7] );
//        req.ret_code = 505;
//        return 1;
//    }

    return 0;
}

// Разбор заголовков запроса
// Заголовок выглядит так последовательность строк вида: <параметр> <:> <значение>
// например: "Server: Apache/2.2.3 (CentOS)"
// получаем заголовок в виде ссылки на вектор строк
// так как заголовки мы не используем, то просто проверяем на верность структуру заголовка
// возвращаем 0, если успешно, -1 - ошибка.
int parse_query_header( std::list<std::string>* header, struct query_string& req )
{
    int res;
    size_t find_pos;

    if( !header || header->size()==0 )
    {
        fprintf( log_file, "Bad header: NULL header" );
        req.ret_code = 501;
        return 1;
    }
// разбираем стартовую строку
    res = parse_query_start_string( header->front(), req );
    if( res>0 )
        return res;  // ошибка в стартовой строке
//    else    // печать для отладки
//        fprintf( log_file, "command %d=%s URI=%s\n", req.method_num, req.method_name.data(), req.uri.data() );

    req.kepp_alive = false; // по умолчанию - не сохранять соединение
// проверяем остальные строки заголовка
    auto i=header->begin();
    i++;
    for( ; i!=header->end(); i++ )
    {
        res = 0;
        string tmp = i->data();
// убираем из строки лишние пробелы: несколько пробелов подряд=один пробел
        for( auto cur_ch=i->begin(); cur_ch!=i->end(); cur_ch++ )
        {
            if( isspace(*cur_ch) )
            {
                auto next_ch=cur_ch+1;
                while( next_ch!=i->end() && isspace(*next_ch) && !(*cur_ch=='\r' && *next_ch=='\n') )
                    i->erase(next_ch);
            }
// конвертируем текущую строку в нижний регистр
            *cur_ch = static_cast<char>( tolower( *cur_ch ));
// ищем в строке двоеточие
            if( *cur_ch==':' && cur_ch!=i->begin() )    // двоеточие не может быть в начале строки
            {
                res = 1;
                auto prev_ch=cur_ch-1;
                if( isspace(*prev_ch) ) // если перед двоеточием пробел, то убираем его
                    i->erase(prev_ch);
            }
        }
        if( !res )   // в строке отсутствует двоеточие
        {
            poprn( *i );  // убираем пeревод строки для нормальной печати ошибки
            fprintf( log_file, "Bad header: incorrect parameter \"%s\"  will be ignored\n", i->data() );
            header->erase(i--);     // удаляем ошибочную строку из запроса
            i++;
            continue;
        }
//  присутствует параметр Content-Length - размер тела сообщения
        if( (i->find("content-length:"))==0 ) // строка начинается с
        {
            char *ch = const_cast<char*>(i->data()+15); // указатель на след. символ
            req.content_length = ::stoul( ch );     // преобразуем найденую строку в число
            fprintf( log_file, "Content-length: %u\n", req.content_length);   // печать для отладки
            continue;
        }

//  присутствует параметр Content-Encoding - есть тело сообщения, способ его кодировки
//        if( (i->find("content-encoding")==0) )
//        {
//  распознавать способ конвертирования сообщения не будем
//        }
//  присутствует параметр Content-Type
        if( (i->find("content-type:")==0) )
        {
// варианты: "application", "audio", "image", "text", "video", и др.
            std::string types[] { "image", "text" };    // будем обрабатывать только текст и картинки
            bool found = false;
            for( auto content_type: types )
            {
                find_pos = i->find( content_type.data(), 13, content_type.length());
                if( find_pos!=std::string::npos )
                {
                    found = true;
                    req.content_type = i->substr( find_pos );
                }
            }
            if( !found )
            {
                req.ret_code = 501;
                poprn( *i );  // убираем пeревод строки
                fprintf( log_file, "Bad header: Not Implemented \"%s\"\n", i->data() );
                return 1;
            }
            continue;
        }
//  присутствует параметр Connection : (Keep-Alive?) - сохранять соединение
        if( (i->find("connection")!=std::string::npos ) && (i->find("keep-alive")!=std::string::npos) )
        {
            req.kepp_alive = true;
            fprintf( log_file, "Connection: Keep-Alive header found\n" );   // печать для отладки
            continue;
        }
        if( i->find("keep-alive:")==0 )   // ищем заголовок keep-alive
        {
            find_pos=i->find("timeout=", 11);     // ищем timeout после заголовка
            if( find_pos!=std::string::npos )
            {
                char *ch = const_cast<char*>(i->data()+find_pos+8); // указатель на след. символ после '='
                req.keep_alive_timeout = ::stoul( ch );     // преобразуем найденую строку в число
                fprintf( log_file, "Keep-Alive timeout: %u sec\n", req.keep_alive_timeout );   // печать для отладки
            }
            else    // таймаут в старом формате - просто число после двоеточия
            {
                res = 1;
                for( auto ch = i->data()+11; *ch!='\0'; ch++ ) // проверяем, что после двоеточия есть число
                    if( !isdigit(*ch) && !isspace(*ch) )
                    {
                        res = 0;
                        break;
                    }
                if( res )   // есть
                    req.keep_alive_timeout = ::stoul( (i->data()+11) );     // преобразуем найденую строку в число
            }
        }
    }
    return 0;
}

// длина строки запроса более 2000 - ошибка. Возврат сервера -414 (Request-URI Too Long)
// возвращает 2, если запроса нет; 1 если произошла ошибка; 0 - все в порядке
// 3 получен сигнал завершения "shutdown server"
int parse_query( int fd_in, std::list<std::string> &query, data_type &query_data, struct query_string &req )
{
    char str[QUERY_MAX_LEN+1];
    size_t data_length = 0, header_size = 0;
    int res = 0;
    bool newstring = true;
    bool empty_string = false;

    query.clear();
    query_data.clear();
    req.clear();
    while( (data_length=ReadLine( fd_in, str, QUERY_MAX_LEN)) )
    {
        fprintf( log_file, "%s", str );
        header_size += data_length;
        if( header_size > MAX_HEADER_SIZE )
        {
            fprintf( log_file, "Bad header: Entity Too Large\n" );
            req.ret_code = 413;
            return 1;
        }
        if( newstring ) // если предыдущая строка заканчивается переводом строки
            query.push_back( str ); // то создаем новую строку
        else
            query.back().append( str ); // иначе добавляем в последнюю
        newstring = str[data_length-1] == '\n'; // прочитанная строка кончается переводом строки? true
        if( query.back().length() > QUERY_MAX_LEN ) // длина строки очень большая, разбираться не будем
        {
            fprintf( log_file, "Bad header: Request-URI too large\n \"%s\"\n", query.back().data() );
            req.ret_code = 414;
            return 1;
        }
        if( (query.back().compare("\n") )==0 || (query.back().compare("\r\n") )==0 )    // строка только из '\n'
        {                
            query.pop_back();   // удаляем ее
            if( query.size() == 0 )
            {
//                cout<<"empty string in begin"<<endl;
                continue;   // пустая строка вначале - пропускаем ее
            }
            else    // не вначале
            {
//                if( empty_string )  // вторая подряд пустая строка -
//                cout<<"empty string in query - break input"<<endl;
                break;  // заголовок есть - уходим из цикла, дальше должны быть данные
            }
        }
        else
            empty_string = false;
    }
//  разбираем заголовки
    if( query.size() == 0 )
    {
//        cout<<"Not header in input: return 2"<<endl;
        return 2;  // заголовок отсутствует
    }

    if( query.front().find( "shutdown server" )!=std::string::npos )
    {
        return 3;  // получена команда завершения работы
    }
    res = parse_query_header( &query, req );
    if( res>0 ) return 1;  // неверный заголовок

// если в запросе есть данные - читаем их
    if( req.content_length )
    {
        std::vector<uint8_t> str_vec(DATA_STRING_LEN);
        size_t len_to_read = req.content_length>DATA_STRING_LEN? DATA_STRING_LEN: req.content_length;   // сколько будем читать
        header_size = 0;    // тут будем накапливать кол-во прочитанных данных
// сначала читаем из буфера уже принятые данные: в ReadLine параметр flush=1
        do
        {
            if( header_size+len_to_read > req.content_length )  // если хотим прочитать больше
                len_to_read = req.content_length - header_size;
            if( !len_to_read )  // все данные прочли
                break;
            if( (data_length=ReadLine( fd_in, str, static_cast<ssize_t>(len_to_read+1), 1 )) )
            {
                header_size += data_length;
                str_vec.resize( data_length );
                memcpy( str_vec.data(), str, data_length );
                query_data.push_back( str_vec ); // создаем новую строку
            }
        } while( data_length ); // пока буфер не пуст

        do  // дальше читаем из сокета
        {
            if( header_size+len_to_read > req.content_length )  // если хотим прочитать больше
                len_to_read = req.content_length - header_size;
            if( !len_to_read )  // все данные прочли
                break;
//            if( (res=read( fd_in, str, static_cast<unsigned>(len_to_read) ))>0 )
            if( (res=recv( fd_in, str, static_cast<unsigned>(len_to_read), MSG_NOSIGNAL ))>0 )
            {
                size_t ures = static_cast<size_t>(res);
                header_size += ures;
                str_vec.resize( ures );
                memcpy( str_vec.data(), str, ures );
                query_data.push_back( str_vec ); // создаем новую строку
            }
            if( res==-1 && errno==EAGAIN ) // нет доступных данных и сокет нв режиме O_NONBLOCK
                res = 1;        // пусть читает заново
        } while( res>0 ); // пока буфер не пуст

        if( query_data.size()==0 )
        {
            fprintf( log_file, "Bad query: Missing query data %u bytes", req.content_length );
            return 1;
        }
    }
    for( auto i: query )
        fprintf( log_file,"%s", i.data() );

    return 0;
}

// вспомогательная функция поиска символа в буфере ограниченной длины
char* buf_strchr( char* buffer, char ch, size_t bufferlen )
{
    for( size_t un=0; un<bufferlen; un++)
        if( buffer[un] == ch )
            return buffer+un;
    return nullptr;
}

// We read-ahead, so we store in static buffer
// what we already read, but not yet returned by ReadLine.
// если flush<>0, чтения файла не производится, возвращаются уже прочитанные данные
size_t ReadLine(int fd, char* line, ssize_t len, int flush)
{
     static char *buffer = static_cast<char*>(malloc(1025*sizeof(char)));
     static size_t bufferlen=0;
     char tmpbuf[1025];
// Do the real reading from fd until buffer has '\n'.
     char *pos;
     ssize_t n;
     size_t un; //, i;  // unsigned значение n, шоб 100 раз не преобразовывать

     if( !line || !len )
         return 0;
     if( flush )
         pos=buffer+bufferlen-1;
     else
     {
         while( (pos=buf_strchr(buffer, '\n', bufferlen)) ==nullptr )
         {
//             n = read(fd, tmpbuf, 1024);
             n = recv(fd, tmpbuf, 1024, MSG_NOSIGNAL );
             if (n==0 || n==-1)
             {  // handle errors
                 bufferlen=0;
                 buffer = static_cast<char*>( realloc(buffer, sizeof(char)) );
                 buffer[0] = '\0';
                 line = buffer;
                 return 0;
             }
             un = static_cast<size_t>(n);
             tmpbuf[un] = '\0';
             buffer = static_cast<char*>( realloc(buffer, (bufferlen+un)*sizeof(char)) );
             memcpy( buffer+bufferlen, tmpbuf, un );
             bufferlen += un;
         }
     }
// Split the buffer around '\n' found and return first part.
     if( (pos-buffer+1)>=len ) // полученная срока больше буфера для чтения
     {  // возвращаем то, что влезет
         un = static_cast<size_t>(len-1);
         memcpy(line, buffer, un);
         line[un] = '\0';
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
     return un;
}
