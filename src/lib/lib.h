#ifndef VSH_NEW_LIB_H
#define VSH_NEW_LIB_H

#include "vec/vec.h"
#include <stdbool.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>

/*
 * error codes
 */
enum ErrorCodes {
    ForkFailed = 101,
    PipeFailed = 102,
};

/*
 * utility functions
 */
Vec *new_vec_string();
char *fmt_string(void *);
void debug_lib(bool);
bool is_debug();
#define str_equals(self, other) strcmp(self, other) == 0
#define ptr_to_type(type) (type)(uintptr_t)
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

    void (*change_cwd)(char *new_dir);

    char *(*simple_cwd)();
    char *(*prompt_user)();
    void (*drop)(struct shellState *state);
} ShellState;

ShellState *new_shell_state();

char *shell_state_cwd();

char *shell_state_home();

void shell_state_change_cwd(char *);

char *shell_state_simple_cwd();

void shell_state_drop(ShellState *);

#define MAX_SHELL_INPUT 8192
char *shell_state_prompt_user();

/*
 * ShellBehavior
 * description: enum to describe the behavior that the shell should have after a call
 */
enum ShellBehavior {
    Continue,
    Exit,
    ClearBackground,
    ClearBackgroundAndExit,
    Cd,
    UnknownCommand = 99
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
 * call: function to fork the actual process and execute the program provided by argc and argv values
 */
typedef struct execArgs {
    unsigned int argc;
    char **argv;
} ExecArgs;

ExecArgs *new_exec_args(unsigned int, char **);
ExecArgs *new_exec_args_from_vec_str(Vec*);
void exec_args_drop(ExecArgs *);
char *exec_args_fmt(ExecArgs *);
void exec_args_print(ExecArgs *);
CallResult *exec_args_call(struct execArgs *, bool, bool, bool);
Vec *new_vec_exec_args();

/*
 * CallGroup
 * description: struct to store a group of calls in a similar context.
 * exec_amount: amount of commands to execute.
 * type: type of execution
 * is_background: is this a background call?
 * file_name: 
 * exec_arr: array of ExecArgs
 * drop: function to dealocate this struct
 */
typedef struct callGroup {
    unsigned int exec_amount;
    enum CallType type;
    bool is_background;
    char *file_name;
    ExecArgs **exec_arr;
} CallGroup;

CallGroup *new_call_group(Vec*, enum CallType, bool);
char *call_group_fmt(CallGroup *);
void call_group_print(CallGroup *);
void call_group_drop(CallGroup *);
Vec *new_vec_call_group();

/*
 * CallGroups
 * description: struct to store a array of CallGroup
 * has_parsing_error: if the call has some parsing error
 * len: amount of CallGroup
 * groups: array of CallGroup
 * drop: function to deallocate this struct
 */
typedef struct callGroups {
    bool has_parsing_error;
    int len;
    CallGroup **groups;
} CallGroups;

CallGroups *new_call_groups(Vec*, bool);
CallGroups *call_groups_from_input(char *);
void call_groups_drop(CallGroups *);
char *call_groups_fmt(CallGroups *);
void call_groups_print(CallGroups *);

/*
 * ArgParseState
 * description: enum used to specify the actual state of the parsing of a shell command string
 */
enum ArgParseState {
    Ignore,
    Word,
    LeftQuote,
};

/*
 * ArgType
 * description: enum used to specify the type of the argument parsed by the parser of a shell command string
 */
enum ArgType {
    Simple,
    Quoted,
    Bar,
    At,
    DoubleAt,
};

/*
 * ParseArgRes
 * description: struct used to store the result of a argument parsed which specify it's value and type
 */
typedef struct parseArgRes {
    char *arg;
    enum ArgType type;
} ParseArgRes;

// Function that describe the parsing of the shell input string
Vec* vec_parse_arg_res_from_shell_input(char*);
ParseArgRes *new_parse_arg_res(char *, enum ArgType);
void parse_arg_res_drop(ParseArgRes *);
char *parse_arg_res_take_arg(ParseArgRes *);
char *parse_arg_res_fmt(ParseArgRes*);
void parse_arg_res_print(ParseArgRes*);

#endif//VSH_NEW_LIB_H
