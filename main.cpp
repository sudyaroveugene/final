#include <iostream>
#include <set>
#include <sys/stat.h>
#include <signal.h>
#include <stdio.h>
#if defined(__linux__)
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#endif
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <sys/types.h>

#include "queryparser.h"
#include "prepresponse.h"

int set_nonblock( int fd )
{
    int flags;
#if defined (O_NONBLOCK)
    if( -1==(flags=fcntl( fd, F_GETFL, 0)))
        flags = 0;
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
#else
    flags = 1;
    return ioctl( fd, FIOBIO, &flags );
#endif
}

FILE *log_file;
std::string ip_addr, current_dir, port; // в этих строках будут параметры из командной строки IP-адрес сервера, рабочий каталог и порт
struct sockaddr_in client_name;//--структура sockaddr_in клиентской машины (параметры ее нам неизвестны.
int ret = 0;
                                // Мы не знаем какая машина к нам будет подключаться)
unsigned int client_name_size = sizeof(client_name);//--размер структуры (тоже пока неизвестен)
int client_socket_fd;          //--идентификатор клиентского сокета

int request( int fd )
{
    std::list<std::string> query, response;       // запрос и данные запроса/ответа
    data_type query_data;                //  ответ
    struct query_string req;
    int res;

//    query.clear(); query_data.clear();
//    response.clear();
    do {
        res = parse_query( fd, query, query_data, req );    // получаем запрос
        if( res>1  )    // 1 - ошибка, 2 - на входе нет запросов, 3 - выключить сервер
            return res;  // ошибку игнорируем, остальное - выходим
        prepare_response( response, query_data, req );      // подготавливаем ответ
        send_response( fd, response, query_data );     // отсылаем ответ
        fflush( log_file );
    } while( 1 );
    return 0;
}

//--функция ожидания завершения дочернего процесса
void sig_child(int sig)
{
    pid_t pid;
    int stat;
    while( (pid=waitpid(-1,&stat,WNOHANG))>0 )
    {
        sleep(1);
        if( WIFEXITED(stat) )
            ret = WEXITSTATUS( stat );
    }
    return;
}

int server()
{
    void sig_child(int);//--объявление функции ожидания завершения дочернего процесса
    int res;
    char host[INET_ADDRSTRLEN];     // должно помещаться "ddd.ddd.ddd.ddd"
    const char* char_res;
    std::vector<pid_t> child_pid;   // массив pid для дочерних процессов
    pid_t pid;
    time_t now;
    struct tm *tm_ptr;
    char timebuf[80];
    struct sockaddr_in master; //--структура sockaddr_in для нашего сервера

    fprintf( log_file, "\n[Server] Start\n" );
    int master_socket = socket( AF_INET, SOCK_STREAM, IPPROTO_TCP );

    master.sin_family = AF_INET;    //--говорим что сокет принадлежит к семейству интернет
    master.sin_port = htons( atoi(port.data()) );      //--и прослушивает порт
    master.sin_addr.s_addr = htonl( INADDR_ANY );   //--наш серверный сокет принимает запросы от любых машин с любым IP-адресом
//--функция bind спрашивает у операционной системы,может ли программа завладеть портом с указанным номером
    res = bind( master_socket, (struct sockaddr*) (&master), sizeof(master));
    if( res == -1 )
    {
        fprintf( log_file, "[Server] Error binding port: %s\n", strerror(errno) );
        fflush( log_file );
        exit( 1 );
    }
    set_nonblock( master_socket );
    listen( master_socket, SOMAXCONN ); //--перевод сокета сервера в режим прослушивания с очередью в макс. позиций
    fprintf( log_file, "port=%d ip_addr=%d\n", master.sin_port, master.sin_addr.s_addr );
    fflush( log_file );
    while( 1 )
    {
    //        if( !req.kepp_alive )     // была идея отслеживать keep-alive таймаут или 30 сек по умолчанию
// устанавливаем обработчик завершения клиента (уже поработал и отключился, ждем завершение его дочернего процесса)
        signal( SIGCHLD, sig_child );
        client_socket_fd = accept( master_socket, (struct sockaddr*) (&client_name), &client_name_size ); //--подключение нашего клиента
        if( client_socket_fd>0 ) //--если подключение прошло успешно
        {
//--то мы создаем копию нашего сервера для работы с другим клиентом(то есть один сеанс уже занят дублируем свободный процесс)
            pid=fork();
            if( pid==0 )
            {   // child
                fprintf( log_file, "[Server] Client %d connected\n", getpid() );
//                set_nonblock( client_socket_fd );

                char_res =  inet_ntop( AF_INET, &client_name.sin_addr, host, sizeof(host) ); // --в переменную host заносим IP-клиента
                fprintf( log_file, "[Server] Client %s connected\n", host );
                fflush( log_file );
                if( char_res == nullptr )
                {
                    fprintf( log_file, "[Server] Error сonverts the network address to string format: %s\n", strerror(errno) );
                    fflush( log_file );
                    exit( 1 );
                }
                time(&now);
                tm_ptr = localtime(&now);
                strftime( timebuf, sizeof( timebuf ), "%Y-%B-%e %H:%M:%S", tm_ptr );
                fprintf( log_file,"[Server] %s Connected client:%s in port: %d\n",
                         timebuf, host, ntohs( client_name.sin_port ) );
                fprintf( log_file, "[Server] Waiting request\n" );
                fflush( log_file );
                res = request( client_socket_fd );
                time(&now);
                tm_ptr = localtime(&now);
                strftime( timebuf, sizeof( timebuf ), "%Y-%B-%e %H:%M:%S", tm_ptr);
                fprintf( log_file,"[Server] %s Close session on client: %s\n", timebuf, host);
                shutdown( client_socket_fd, SHUT_RDWR );
                close(client_socket_fd); //--естествено закрываем сокет
//                cout<<"tut4 res="<<res<<endl;
                fprintf( log_file, "[Server] Client %d closed\n" );
                fflush( log_file );
                exit( res ); //--гасим дочерний процесс
            }
            else if( pid>0 )   // parent
            {
                child_pid.push_back( pid ); // добавляем дочерний процесс в список
                fflush( log_file );
            }
            else    // ошибка
            {
                fprintf( log_file, "[Server] Fork failed \"%s\"\n", strerror(errno));
                fflush( log_file );
                exit( 1 );
            }
        }
        sleep( 1 );
        if( ret==3 ) // получена команда завершения сервера
        {
            fprintf( log_file, "[Server] Shutdown server received\n", pid );
            shutdown( master_socket, SHUT_RDWR );
            close( master_socket );
            for( auto i: child_pid )
                kill( i, SIGTERM );
            fprintf( log_file, "[Server] Closed\n" );
            fflush( log_file );
            break;
        }
    }

    fprintf( log_file, "[Server] Stop\n\n" );
    fflush( log_file );
    return 0;
}

// монитор отслеживает сигналы, может прервать работу сервера
int server_monitor()
{
    int pid, status;
    sigset_t sigset;
    siginfo_t siginfo;

// настраиваем сигналы которые будем обрабатывать (добавляем их в sigset)
    sigemptyset(&sigset);    
    sigaddset(&sigset, SIGQUIT);    // сигнал остановки процесса пользователем
    sigaddset(&sigset, SIGINT);     // сигнал для остановки процесса пользователем с терминала
    sigaddset(&sigset, SIGTERM);    // сигнал запроса завершения процесса
    sigaddset(&sigset, SIGCHLD);    // сигнал посылаемый при изменении статуса дочернего процесса
    sigprocmask(SIG_BLOCK, &sigset, NULL);

    // бесконечный цикл работы
    while( 1 )
    {
        // создаём потомка
        pid = fork();
        if (pid == -1) // если произошла ошибка
        {
            // запишем в лог сообщение об этом
            fprintf( log_file, "[MONITOR] Server Fork failed \"%s\"\n", strerror(errno));
        }
        else if (!pid) // если мы потомок
        {            
            status = server();          // запустим функцию отвечающую за работу демона
            exit(status);   // завершим процесс
        }
        else // если мы родитель
        {            
            sigwaitinfo(&sigset, &siginfo);     // ожидаем поступление сигнала
            if( siginfo.si_signo == SIGCHLD )       // если пришел сигнал от потомка
            {                
                wait( &status );    // получаем статус завершение
                    // запишем в лог сообщение об этом
                fprintf( log_file, "[MONITOR] Child stopped\n");
                break;         // прервем цикл
            }
            else // если пришел какой-либо другой ожидаемый сигнал
            {
                // запишем в лог информацию о пришедшем сигнале
                fprintf( log_file, "[MONITOR] Signal %s\n", strsignal(siginfo.si_signo));                
                kill(pid, SIGTERM);     // убьем потомка
                status = 0;
                break;
            }
        }
    }
    // запишем в лог, что мы остановились
    fprintf( log_file, "[MONITOR] Stop %d\n", status );
    fflush( log_file );
    return status;
}

int main( int argc, char** argv )
{
    int optnum, pid;

// задаем параметры по умолчанию
    current_dir = getcwd( nullptr, 0 ); // рабочий каталог - текущий
    port = "12345"; // порт
    ip_addr = "127.0.0.1";  // IP-адрес сервера
// читаем параметры из командной строки
    while( (optnum=getopt( argc, argv, "p:h:d:"))>0 )
    {
        switch( optnum )
        {
            case 'p':
                port = optarg; break;
            case 'h':
                ip_addr = optarg; break;
            case 'd':
                current_dir = optarg; break;
        }
    }

    pid = fork();
    if( pid<0 )
    {
        perror( "fork");
        exit( 1 );
    }
    else if( pid>0 )  // parent
    {
        return 0;   // закрываем родительский процесс
    }
    else       // child
    {
        umask(0);   /* Изменяем файловую маску */
    // открываем файл лога
        log_file = fopen( "final.log", "wb" /*"a" */);
        if( log_file==nullptr )
        {
            perror( "Error open final.log");
            exit( 1 );
        }
        fprintf( log_file, "current_dir=%s port=%s ip_addr=%s\n", current_dir.data(), port.data(), ip_addr.data() );
        if( setsid()<0 )    /* Создание нового SID для дочернего процесса */
        {
            perror( "setsid");
            exit( 1 );
        }

        if( (chdir("/home/box/")) < 0) /* Изменяем текущий рабочий каталог */
        {
            perror( "chdir" );
            exit( 1 );
        }
        /* Закрываем стандартные файловые дескрипторы */
        close(STDIN_FILENO);
        close(STDOUT_FILENO);
        close(STDERR_FILENO);

        fflush( log_file );
//        pid = server_monitor();   // оставляем наш сервер в виде демона
        pid = server();
        return pid;
    }
    return 0;
}

