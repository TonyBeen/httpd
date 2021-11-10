/*************************************************************************
    > File Name: main.cpp
    > Author: hsz
    > Mail:
    > Created Time: Tue 17 Aug 2021 06:16:08 PM PDT
 ************************************************************************/

#include "application.h"

int main(int argc, char **argv)
{
    Jarvis::Application app;
    app.init(argc, argv);

    return app.run();
}