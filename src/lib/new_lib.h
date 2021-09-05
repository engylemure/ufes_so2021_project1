//
// Created by jordao on 04/09/2021.
//

#ifndef VSH_NEW_LIB_H
#define VSH_NEW_LIB_H

#include <stdbool.h>
#include <unistd.h>
#include <sys/types.h>
#include "util/vec/vec.h"

/*
 * fmt_string
 * description: utility function to transform a string into a human-readable format
 */
char *fmt_string(void*);

/*
 * CallType
 * description: enum to describe the possible types of a call into the shell
 * Basic: Simple command with its arguments
 * Sequential: Is a list of Command's that will be sequentially executed, one after the other
 * Piped: Command's that will pipe it's output to the input of the other
 */

enum CallType {
    Basic,
    Sequential,
    Piped,
};

/*
 * ShellState
 * description: struct to more easily handle working directory and the home user directory
 * pwd: function to retrieve the working directory
 * home: function to retrieve the user home directory
 * change_pwd: function to change the working directory
 * simple_pwd: function to retrieve simplified information about the working directory
 * drop: function to free the memory allocated by this struct
 */
typedef struct shellState {
    char *(*cwd)();

    char *(*home)();

    void (*change_pwd)(char *new_dir);

    char *(*simple_pwd)();

    void (*drop)(struct shellState *state);
} ShellState;

ShellState *new_shell_state();

char *shell_state_cwd();

char *shell_state_home();

void shell_state_change_pwd(char *);

char *shell_state_simple_pwd();

void shell_state_drop(ShellState *);

/*
 * ShellBehavior
 * description: enum to describe the behavior that the shell should have after a call
 */
enum ShellBehavior {
    Continue, Exit, Cd, UnknownCommand
};

/*
 * CallResult
 * description: struct to store the result data from a call
 * shell_behavior: what should be the behavior of the shell after the call
 * additional_data: information that could store the directory to where the
 * shell should move or the command that it's unknown.
 * is_parent: since we fork the process to execute  a call this attribute
 * describes if the process is the parent or not.
 * child_pid: if it is the parent it will store the pid for the child;
 * drop: function to free the memory allocated by this struct
 */
typedef struct callResult {
    enum ShellBehavior shell_behavior;
    char *additional_data;
    bool is_parent;
    pid_t child_pid;
    void (*drop)(struct callResult *);
} CallResult;

CallResult *new_call_result(enum ShellBehavior, char *, bool, pid_t);

void call_result_drop(CallResult *);

/*
 * ExecArgs
 * description: struct to store information about a call execution
 * argc: number of arguments
 * argv: argument values the first argument stores the program that will be executed
 * drop: function to free the memory allocated by this struct
 * fmt: function to format this struct into a human-readable format
 * print: function to print the fmt return value into the stdout
 */
typedef struct execArgs {
    unsigned int argc;
    char** argv;
    void (*drop)(struct execArgs *self);
    char *(*fmt)(struct execArgs *self);
    void (*print)(struct execArgs *self);
} ExecArgs;

ExecArgs *new_exec_args(unsigned int, char**);
void exec_args_drop(ExecArgs*);
char* exec_args_fmt(ExecArgs*);
void exec_args_print(ExecArgs*);

#endif //VSH_NEW_LIB_H
