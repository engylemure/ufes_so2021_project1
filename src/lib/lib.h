#ifndef LIB_VSH_H
#define LIB_VSH_H

#include <stdbool.h>
#include <stdio.h>
#include <sys/types.h>
#include "util/vec/vec.h"

enum CallType {
    Basic,
    Sequential,
    Piped,
    RedirectStdout,
    RedirectStdIn,
};

typedef struct shellState {
    char *pwd;
    char *home;

    void (*change_dir)(struct shellState *state, char *new_dir);

    char *(*pretty_pwd)(struct shellState *state);

    void (*drop)(struct shellState *state);
} ShellState;

enum CallStatus {
    Continue, Exit, Cd, UnknownCommand
};

typedef struct callResult {
    char *additional_data;
    enum CallStatus status;
    bool is_parent;
    pid_t child_pid;

    void (*drop)(struct callResult *self);
} CallResult;

typedef struct execArgs {
    unsigned int argc;
    char **argv;

    void (*drop)(struct execArgs *self);

    char *(*fmt)(struct execArgs *self);

    CallResult *(*call)(struct execArgs *self, bool should_fork, bool should_wait);
} ExecArgs;

typedef struct callGroup {
    unsigned int exec_amount;
    enum CallType type;
    bool is_background;
    char *file_name;
    ExecArgs **exec_arr;

    void (*drop)(struct callGroup *self);
} CallGroup;

typedef struct callGroups {
    bool has_parsing_error;
    int len;
    CallGroup **groups;

    void (*drop)(struct callGroups *self);
} CallGroups;

typedef struct callArg {
    CallGroups *(*call_groups)(struct callArg *self);

    void (*drop)(struct callArg *self);

    char *arg;
} CallArg;


enum ArgParseState {
    Ignore,
    Word,
    LeftQuote,
};

enum ArgType {
    Simple,
    Quoted,
    Bar,
    At,
    DoubleAt,
};

typedef struct parseArgRes {
    char *arg;
    enum ArgType type;

    void (*drop)(struct parseArgRes *self);

    char *(*take_arg)(struct parseArgRes *self);
} ParseArgRes;

CallArg *prompt_user(ShellState *state);

ShellState *initialize_shell_state();

void todo(char *msg);

void debug_lib(bool should_debug);


/*
 * Utility functions
 */
char *read_env(char *name);

void todo(char *msg);

Vec *new_vec_char();

Vec *new_vec_string();

char *fmt_string(void *str);

bool is_ignorable_call(char *str);

/*
 * ShellState functions
 */
ShellState *initialize_shell_state();

void drop_shell_state(ShellState *self);

char *pretty_pwd(ShellState *self);

void shell_state_change_dir(ShellState *self, char *new_dir);

/*
 * CallArg functions
 */
CallArg *initialize_call_arg(char *arg);

void drop_call_arg(CallArg *self);

/*
 * ExecArgs functions
 */
void drop_exec_args(ExecArgs *self);

char *fmt_exec_arg(void *data);

Vec *new_vec_exec_args();

CallResult *basic_exec_args_call(ExecArgs *exec_args, bool should_fork,
                                 bool should_wait);

/*
 * CallRes functions
 */
CallResult *new_call_result(enum CallStatus status, bool is_parent,
                            char *program_name, pid_t child_pid);

void drop_call_res(CallResult *self);

/*
 * CallGroup functions
 */
CallGroup *call_group_from_vec_exec_args(Vec *vec_exec_args,
                                         enum CallType type, bool is_background);

char *fmt_call_group(void *data);

void drop_call_group(CallGroup *self);

Vec *new_vec_call_group();

/*
 * CallGroups functions
 */
CallGroups *call_groups(CallArg *call_arg);

void drop_call_groups(CallGroups *self);


/*
 * ParseArgRes functions
 */
char *fmt_parse_arg_res(void *data);

#endif