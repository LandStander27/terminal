#ifndef COMMANDS_H
#define COMMANDS_H

typedef int (*command_function)(char**, int);
command_function get_command(char* cmd);

#endif