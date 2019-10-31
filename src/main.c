#include <stdio.h>
#include <signal.h>
#include "../include/server.h"



int main(int argc, char **argv)
{
    signal(SIGPIPE, SIG_IGN);
    server(argc, argv);

    return 0;
}