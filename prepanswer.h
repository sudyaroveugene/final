#ifndef PREPANSWER_H
#define PREPANSWER_H
#include <list>
#include <string>

using namespace std;

int prepare_answer( std::list<std::string> &answer, std::list<std::string> &answer_data );
int send_answer( int fd_out, std::list<std::string> &answer, std::list<std::string> &answer_data );

#endif // PREPANSWER_H
