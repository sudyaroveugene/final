#include <unistd.h>
#include <fcntl.h>
#include <sys/file.h>
#include <stdio.h>
#include <memory.h>
#if defined(__linux__)
#include <sys/socket.h>
#endif

#include "prepresponse.h"
extern FILE* log_file;

int prepare_response( std::list<std::string> &response, data_type &response_data, struct query_string &req )
{
    char buffer[DATA_STRING_LEN+1];
    std::vector<uint8_t> buf_vec(DATA_STRING_LEN);

    response.clear();
    response.push_front( "HTTP/1.0" );

 // ошибка запроса уже определена при разборе
    if( req.ret_code != 200 )
    {
        switch( req.ret_code )
        {
            case 413:
                response.front().append( " 413 Request Entity Too Large\r\n" );
                break;
            case 414:
                response.front().append( " 414 Request-URI Too Long\r\n" );
                break;
            case 505:
                response.front().append( " 505 HTTP Version Not Supported\r\n" );
                break;
            case 501:
            default:
                response.front().append( " 501 Not Implemented\r\n" );
        };
        return 0;
    }

    std::string cwd = getcwd( nullptr, 0 );     // читаем рабочий каталог
// выделяем расширение файла из content_type
    std::string ext = ".";
    size_t beg = req.content_type.find( '/' )+1;
    size_t end = req.content_type.find( ';' );
    if( end == string::npos )
        end = req.content_type.length();
     ext.append( req.content_type.substr( beg, end-beg ) );
// проверяем наличие расширения файла в пути
    beg = req.uri.rfind( '/' );
    end = req.uri.rfind( '\\' );
    size_t slash_pos = beg<end? beg: end;
    if( slash_pos!=string::npos && slash_pos<(req.uri.length()-1)          // путь содержит слэш и не оканчивается на слэш
         && req.uri.find( '.', slash_pos )==string::npos )    // и после слэша нет точки
        req.uri.append( ext );
    if( req.uri.front()!='/' && req.uri.front()!='\\' )
        cwd.append("/");
    cwd.append( req.uri );  // добавляем путь к текущему каталогу
// проверяем полученный путь
    ssize_t n = 0;
    mode_t mode = 0;     // режим чтения файла
//    if( req.content_type.find("image")!=std::string::npos )
#if defined(__linux__)
#else
    mode |= O_BINARY;   // для картинок - двоичный
#endif

// по запросу GET будем отдавать файл целиком. Частичные запросы и запросы с параметрами не обрабатываем
    if( req.method_num == METHOD_GET )
    {
        response_data.clear();
        mode |= O_RDONLY;     // режим файла - чтение
        int fd = open( cwd.data(), mode );  // пробуем открыть файл
        if( fd<0 )  //  файл не найден
        {
            req.ret_code = 404;
//            response.front().pop_back();
            response.front().append( " 404 Not Found\r\n" );
            return 0;
        }
#if defined(__linux__)
        flock( fd, LOCK_SH );   // блокируем файл на чтение
#endif
        while( (n = read( fd, buffer, DATA_STRING_LEN))>0 ) // читаем данные из запрошенного файла в список данных
        {
//            buffer[n]='\0';
            size_t un = static_cast<size_t>(n);
            buf_vec.resize( un );
            memcpy( buf_vec.data(), buffer, un );
            response_data.push_back( buf_vec );
            req.content_length += n;
        }
#if defined(__linux__)
        flock( fd, LOCK_UN );   // разблокируем файл
#endif
        close( fd );    // закрываем запрошеный файл
        if( req.content_length )    //  если есть данные для ответа, то формируем заголовки с типом данных и длиной
        {
//            response_data.back().push_back('\n');
            response.push_back( "Content-Type:"+req.content_type );
            response.push_back( "Content-Length:");
            sprintf( buffer, "%u\r\n", req.content_length );
            response.push_back( buffer );
        }
        response.front().append( " 200 OK\r\n");
    }

// POST - изменение заданного файла
    else if( req.method_num == METHOD_POST )
    {
        bool create_new = false;
        mode |= O_RDONLY;     // режим файла - чтение
        int fd = open( cwd.data(), mode, 0666 );  // пробуем открыть файл
        if( fd<0 )  //  файл не найден, будем создавать новый - будет другой код возврата
            create_new = true;
        mode |= (O_RDWR | O_CREAT | O_TRUNC) ;     // режим файла - чтение/запись
        fd = open( cwd.data(), mode, 0666 );  // пробуем открыть файл
        if( fd<0 )  //  файл не найден
        {
            cwd = "Error open " + cwd;
            fprintf( log_file, "%s: \"%s\"\n", cwd.data(), strerror(errno));
            req.ret_code = 404;
//            response.front().pop_back();
            response.front().append( " 404 Not Found\r\n" );
            return 0;
        }
        beg = 0;
#if defined(__linux__)
        flock( fd, LOCK_EX );   // блокируем файл на запись
#endif
        // пишем данные в файл из списка данных
        while( !response_data.empty() )
        {
//            n = response_data.front().size();
            n = write( fd, response_data.front().data(), static_cast<unsigned>(response_data.front().size()));
            response_data.pop_front();
            beg += static_cast<size_t>( n );
        }
#if defined(__linux__)
        flock( fd, LOCK_UN );   // разблокируем файл
#endif
        close( fd );    // закрываем  файл
        if( beg == req.content_length )    //  если все записали
        {
            response.push_back( "Content-Type:"+req.content_type );
            response.push_back( "Content-Length: ");
            sprintf( buffer, "%u\r\n", req.content_length );
            response.back().append( buffer );
        }
        else
        {
            cwd = "Error writing " + cwd;
            fprintf( log_file, "%s: \"%s\"\n", cwd.data(), strerror(errno));
            req.ret_code = 500;
//            response.front().pop_back();
            response.front().append( " 500 Internal Server Error\r\n" );
        }
        if( create_new )
        {
            response.push_back( "Location: "+cwd+"\r\n" );
            response.front().append( " 201 Created\r\n");
        }
        else
            response.front().append( " 200 OK\r\n");
    }
    else
    {
        response.front().append( " 501 Not Implemented\r\n" );
    }
    return 0;
}

int send_response( int fd_out, std::list<std::string> &response, data_type &response_data )
{
    if( fd_out<0 )
        return -1;

    for( auto i: response )
    {
        fprintf( log_file,"%s", i.data() );
        send( fd_out, i.data(), static_cast<unsigned>(i.length()), MSG_NOSIGNAL );
    }
//    write( fd_out, "\r\n", 2 );
    send( fd_out, "\r\n", 2, MSG_NOSIGNAL );
    if( !response_data.empty() )
    {
        for( auto i: response_data )
//            write( fd_out, i.data(), static_cast<unsigned>(i.size()) );
//        write( fd_out, "\r\n", 2 );
            send( fd_out, i.data(), static_cast<unsigned>(i.size()), MSG_NOSIGNAL );
        send( fd_out, "\r\n", 2, MSG_NOSIGNAL );
    }
    return 0;
}
