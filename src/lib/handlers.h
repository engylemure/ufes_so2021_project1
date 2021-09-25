#ifndef LIB_HANDLERS_H
#define LIB_HANDLERS_H

#include <pthread.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>
#include <string.h>
#include <unistd.h>
#include "lib.h"

#define SHELL_TYPE_ENV_KEY "SHELL_TYPE"
/**
 *  CMD to print the current stored bg execution information, since we
 * use the setsid() cmd to change the piped commands session and group
 * by the project specification this can more easily allow us to identify
 * them
 */
#define DBG_BG_EXECUTION_CMD "bolsonaro_genocida"

/**
 * @brief function to start the background execution mapping
 * it's usage is strictly necessary at the beginning of our program.
 */
void start_bg_execution();

/**
 * @brief function to end and deallocate any data used by the background
 * execution mapping.
 */
void end_bg_execution();

/**
 * @brief function to print into stdout that our shell is felling weird
 */
void print_i_feel_weird();

/**
 * @brief function to create a thread to handle input
 * it will cancel the thread if it exists
 * @param state 
 */
void create_input_thread(ShellState *state);

/**
 * @brief function to allow us to wait and retrieve the output of the 
 * created input thread execution.
 * @return char* 
 */
char *join_input_thread();

/**
 * @brief struct to store and more easily
 * handle with the background execution of processes
 * we can use a vector of child_pids to control it's execution
 * or the child_amount with the pgid considering that all children
 * would be at the same session and process group
 */
typedef struct bgExecution {
    pid_t pgid; // pgid of the execution
    Vec* child_pids; // possible vector of child_pid values 
    unsigned int child_amount; // amount of childs created into this background execution
} BgExecution;

/**
 * @brief creates a BgExecution object
 * @param pgid 
 * @param child_pids 
 * @return BgExecution* 
 */
BgExecution* new_bg_execution(pid_t pgid, Vec* child_pids);

/**
 * @brief deallocate the BgExecution
 * @param self 
 */
void bg_execution_drop(BgExecution* self);
/**
 * @brief print the BgExectuion object into a human-readable format
 * 
 * @param self 
 */
void bg_execution_print(BgExecution* self);

/**
 * @brief tries to clear a child_pid from the child_pids (if it exists over there) 
 * or child_amount (if it has the same pgid as the BgExecution) value
 * @param self 
 * @param child_pid 
 * @return true 
 * @return false 
 */
bool bg_execution_clear_child(BgExecution* self, pid_t child_pid);

/**
 * @brief function to format the BgExecution object into a human-readable string
 * @param self 
 * @return char* 
 */
char* bg_execution_fmt(BgExecution* self);

/**
 * @brief handler for our SIGINT signal, it has the objective 
 * of disabling the default behavior of this signal.
 * @param signal 
 */
void sig_int_handler(int signal);

/**
 * @brief handler for the SIGCHLD signal, it has the objective
 * of acknowledging and handling the background execution mapping cleanup.
 * @param signal
 * @param info
 * @param ucontext
 */
void sig_child_handler(const int signal, siginfo_t *info, void* ucontext) ;

/**
 * @brief handler for the SIGUSR1 or SIGUSR2 signal, it has the objective of
 * allowing us to provide a custom handling of this signal cleaning up any background execution.
 * @param signal 
 */
void sig_usr_handler(int signal);

// cmd_handler option to enable the fork before the execution of the provided ExecArgs object.
#define CMD_HND_OPT_FORK 1 
// cmd_handler option to enable the execution of the provided ExecArgs object.
#define CMD_HND_OPT_BG 2
// cmd_handler option to allow the process of the ExecArgs execution to become a session leader.
#define CMD_HND_OPT_LEADER 4

/**
 * @brief 
 * 
 * @param state 
 * @param exec_args 
 * @param should_continue 
 * @param status_code 
 * @param opts 
 * @return pid_t 
 */
pid_t cmd_handler(ShellState *state, ExecArgs *exec_args, bool *should_continue, int *status_code, unsigned int opts);

/**
 * @brief function to abstract the execution of a basic CallGroup
 * @param state 
 * @param call_group 
 * @param should_continue 
 * @param status_code 
 * @return pid_t 
 */
pid_t basic_cmd_handler(ShellState *state, CallGroup *call_group,
                        bool *should_continue, int *status_code);

/**
 * @brief function to abstract the handling of a unknown command by using the CallResult data
 * @param res 
 * @param should_continue 
 * @param status_code 
 */
void unknown_cmd_info(CallResult *res, bool *should_continue,
                      int *status_code);

/**
 * @brief function to abstract the execution of sequential commands by using the CallGroup object
 * @param state 
 * @param call_group 
 * @param should_continue 
 * @param status_code 
 */
void sequential_cmd_handler(ShellState *state, CallGroup *call_group,
                            bool *should_continue, int *status_code);

/**
 * @brief function to abstract the execution of piped commands by using the CallGroup object
 * @param state 
 * @param call_group 
 * @param should_continue 
 * @param status_code 
 */
void piped_cmd_handler(ShellState *state, CallGroup *call_group,
                       bool *should_continue, int *status_code);

/**
 * @brief function to describe the stored information related to background processes in execution
 */
 void debug_bg_execution();

#endif