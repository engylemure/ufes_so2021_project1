#ifndef VSH_NEW_LIB_H
#define VSH_NEW_LIB_H

#include "vec/vec.h"
#include <stdbool.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>

/**
 * @brief enum to describe the possible ErrorCodes returned by an exception into our library
 */
enum ErrorCodes {
    ForkFailed = 101, // fork() call failed
    PipeFailed = 102, // pipe() call failed
};

/**
 * @brief Creates a vector of stringss
 * @return Vec* 
 */
Vec *new_vec_string();

/**
 * @brief returns a human readable "string" from a string
 * @return char* 
 */
char *fmt_string(void *);

/**
 * @brief function to enable or disable some debug into our library 
 */
void debug_lib(bool);

/**
 * @brief function to identify if we are on debug mode.
 * @return true 
 * @return false 
 */
bool is_debug();

/**
 * @brief macro to simplify string equality comparison
 */
#define str_equals(self, other) strcmp(self, other) == 0
/**
 * @brief macro to simplify the casting of possible pointer type to integer like types
 */
#define ptr_to_type(type) (type)(uintptr_t)

/**
 * @brief enum to describe the possible types of a call into the shell
 */
enum CallType {
    Basic, // Simple command with its arguments
    Sequential, // Is a list of Command's that will be sequentially executed, one after the other
    Piped, // Command's that will pipe it's output to the input of the other
};

/**
 * @brief struct to more easily deal with handling of the shell state that includes
 * it's working directory and requesting for user input.
 */
typedef struct shellState {
    char *(*cwd)(); // function to retrieve the working directory
    char *(*home)(); // function to retrieve the user home directory
    void (*change_cwd)(char *new_dir); // function to change the working directory
    char *(*simple_cwd)(); // function to retrieve simplified information about the working directory
    char *(*prompt_user)(); // function to request input from the stdin to the user presenting some state information
    void (*drop)(struct shellState *state); // function to free the memory allocated by this struct
} ShellState;

/**
 * @brief Creates a ShellState
 * @return ShellState* 
 */
ShellState *new_shell_state();

/**
 * @brief returns the user current working directory
 * @return char* 
 */
char *shell_state_cwd();

/**
 * @brief returns the home directory from the user that owns the actual session
 * @return char* 
 */
char *shell_state_home();

/**
 * @brief function to change the current working directory 
 */
void shell_state_change_cwd(char *);

/**
 * @brief function to retrieve a simplified and human readable string that shows the
 * current working directory
 * @return char* 
 */
char *shell_state_simple_cwd();

/**
 * @brief function to deallocate the ShellState
 */
void shell_state_drop(ShellState *);

/**
 * @brief macro to define the MAX input that the shell can receive.
 */
#define MAX_SHELL_INPUT 8192

/**
 * @brief function to request from the stdin a command from the user
 * @return char* 
 */
char *shell_state_prompt_user();

/**
 * @brief enum to describe the behavior that the shell should have after a call
 */
enum ShellBehavior {
    Continue,
    Exit,
    ClearBackground,
    ClearBackgroundAndExit,
    Cd,
    UnknownCommand = 99
};

/**
 * @brief struct to store the result data from the ExecArgs call function
 * that would allow us to more easily deal with the result of the execution.
 */
typedef struct callResult {
    enum ShellBehavior shell_behavior; // what should be the behavior of the shell after the call
    char *additional_data; // information that could store the directory to where the shell should 
    // move or the command that it's unknown.
    bool is_parent; // since we use fork in some cases identifying that we are the parent process
    // after calling a foreign program is useful
    pid_t child_pid; // if it is the parent it will store the pid for the child.
} CallResult;

/**
 * @brief "constructor" for the CallResult
 * @param s_b shell behavior
 * @param additional_data additional data related to the result
 * @param is_parent 
 * @param child_pid 
 * @return CallResult* 
 */
CallResult *new_call_result(enum ShellBehavior s_b, char *additional_data, bool is_parent, pid_t child_pid);

/**
 * @brief function to deallocate the CallResult
 */
void call_result_drop(CallResult *self);

/**
 * @brief struct to store information about a call execution as you can check 
 * it's similar to the same arguments that you can usually access by the main function
 * of our program, and by that it should be used to exectue a program considering this same
 * relation.
 */
typedef struct execArgs {
    unsigned int argc; // number of arguments
    char **argv; // argument values the first argument stores the program that will be executed
} ExecArgs;

/**
 * @brief constructor for the ExecArgs struct
 * @param int 
 * @return ExecArgs* 
 */
ExecArgs *new_exec_args(unsigned int argc, char ** argv);

/**
 * @brief constructor for the ExecArgs from a vector of strings.
 * @param vec_str 
 * @return ExecArgs* 
 */
ExecArgs *new_exec_args_from_vec_str(Vec* vec_str);

/**
 * @brief function to deallocate the ExecArgs*
 * @param self 
 */
void exec_args_drop(ExecArgs *self);

/**
 * @brief function to format into a human-readable way the ExecArgs struct
 * @param self 
 * @return char* 
 */
char *exec_args_fmt(ExecArgs *self);

/**
 * @brief function to print into stdout the ExecArgs struct into a human-readable way
 * @param self 
 */
void exec_args_print(ExecArgs *self);

/**
 * @brief function to execute the ExecArgs considering some options 
 * @param exec_args 
 * @param should_fork if we should fork the actual process before executing this ExecArgs
 * returning a CallResult on it.
 * @param should_wait if we should wait for the end of the forked program execution.
 * @param is_session_leader if the forked process should become a session leader program.
 * @return CallResult* 
 */
CallResult *exec_args_call(ExecArgs *exec_args, bool should_fork, bool should_wait, bool is_session_leader);

/**
 * @brief constructor for a vector of ExecArgs
 * @return Vec* 
 */
Vec *new_vec_exec_args();

/**
 * @brief struct to store a group of calls in a similar context.
 */
typedef struct callGroup {
    unsigned int exec_amount;
    enum CallType type;
    bool is_background;
    ExecArgs **exec_arr;
} CallGroup;

/**
 * @brief CallGroup "constructor"
 * 
 * @return CallGroup* 
 */
CallGroup *new_call_group(Vec*, enum CallType, bool);

/**
 * @brief function to retrieve a human readable "string" from CallGroup
 * 
 * @return char* 
 */
char *call_group_fmt(CallGroup *);
/**
 * @brief function to print the human readable "string" from CallGroup into stdout
 * 
 */
void call_group_print(CallGroup *);
/**
 * @brief function to deallocate the CallGroup value
 * 
 */
void call_group_drop(CallGroup *);
/**
 * @brief function to allocate a Vec of CallGroup
 * 
 * @return Vec* 
 */
Vec *new_vec_call_group();

/**
 * @brief struct to store a array of CallGroup and provide 
 * an api for it's usage.
 */
typedef struct callGroups {
    bool has_parsing_error; // if the call has some parsing error
    int len; // amount of CallGroup
    CallGroup **groups; // array of CallGroup
} CallGroups;

/**
 * @brief constructor for the CallGroups
 * @return CallGroups* 
 */
CallGroups *new_call_groups(Vec*, bool);

/**
 * @brief function to parse and process a string into the CallGroups struct
 * @return CallGroups* 
 */
CallGroups *call_groups_from_input(char *);

/**
 * @brief function to deallocate CallGroups
 */
void call_groups_drop(CallGroups *);

/**
 * @brief function to format into a human-readable way the CallGroups struct
 * @return char* 
 */
char *call_groups_fmt(CallGroups *);

/**
 * @brief function to print into a human-readable way the CallGroups struct to the stdout
 */
void call_groups_print(CallGroups *);

/**
 * @brief enum used to more easily handle the state of the parsing of arguments from the provided input.
 */
enum ArgParseState {
    Ignore,
    Word,
    LeftQuote,
};

/**
 * @brief enum used to specify the type of the argument parsed by the parser of a shell command string
 */
enum ArgType {
    Simple,
    Quoted,
    Bar,
    At,
    DoubleAt,
};

/**
 * @brief struct used to store the result of a argument parsed which specify it's value and type
 */
typedef struct parseArgRes {
    char *arg; // parsed argument result 
    enum ArgType type; // type of the parsed argument
} ParseArgRes;

/**
 * @brief Function that returns a vector of ParseArgRes from an input
 * @return Vec* 
 */
Vec* vec_parse_arg_res_from_shell_input(char*);

/**
 * @brief constructor for the ParseArgRes struct
 * @return ParseArgRes* 
 */
ParseArgRes *new_parse_arg_res(char *, enum ArgType);

/**
 * @brief function to deallocate the ParseArgRes
 */
void parse_arg_res_drop(ParseArgRes *);

/**
 * @brief function to retrieve the internal argument and deallocate the ParseArgRes
 * @return char* 
 */
char *parse_arg_res_take_arg(ParseArgRes *);

/**
 * @brief function to format the ParseArgRes struct into a human-readable format
 * @return char* 
 */
char *parse_arg_res_fmt(ParseArgRes*);

/**
 * @brief function to print the formatted ParseArgRes struct from a human-readable format
 * 
 */
void parse_arg_res_print(ParseArgRes*);

#endif//VSH_NEW_LIB_H
