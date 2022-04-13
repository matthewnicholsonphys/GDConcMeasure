#ifndef ALGORITHMS_H
#define ALGORITHMS_H
#include <errno.h>     // for WEXITSTATUS etc
#include<strings.h>    // for strerror
#include <string>

int SystemCall(std::string cmd, std::string& errmsg);

#endif
