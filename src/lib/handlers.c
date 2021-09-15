#include "handlers.h"


const char *SHELL_TYPE_ENV_KEY = "SHELL_TYPE";
bool HAS_DFLT_SHELL_BEHAVIOR = false;
Vec *vec_bg_execution;


BgExecution *new_bg_execution(pid_t pgid, Vec *child_pids) {
    BgExecution *val = malloc(sizeof(BgExecution));
    val->child_pids = child_pids;
    val->pgid = pgid;
    val->child_amount = 0;
    return val;
}

char *bg_execution_fmt(BgExecution *self) {
    char *str = malloc(sizeof(char) * 100);
    int child_pids_len = self->child_pids != NULL ? self->child_pids->length : 0;
    sprintf(str, "BgExecution { pgid: %d, child_pids_len: %ld, child_amount: %d }", self->pgid, child_pids_len, self->child_amount);
    return str;
}

void bg_execution_print(BgExecution *self) {
    char *str = bg_execution_fmt(self);
    printf("%s\n", str);
    free(str);
}

void bg_execution_drop(BgExecution *self) {
    if (self->child_pids != NULL) {
        Vec *child_pids = self->child_pids;
        vec_drop(child_pids);
        self->child_pids = NULL;
    }
    free(self);
}

// Function to allow a more easy handling of a child clean up
// since we are storing the child pid of a background execution
// we can more easily identify to which execution a specific child belongs
bool bg_execution_clear_child(BgExecution *self, pid_t child_pid) {
    pid_t pgid = getpgid(child_pid);
    if (self->child_pids != NULL && self->child_pids->length) {
        if (pgid == self->pgid) {
            Vec *child_pids = self->child_pids;
            int i;
            for (i = 0; i < child_pids->length; i++) {
                pid_t pid = ptr_to_type(pid_t) child_pids->_arr[i];
                if (pid == child_pid) {
                    vec_take(child_pids, i);
                    return true;
                }
            }
        }
    } else {
        if (self->child_amount && self->pgid == pgid) {
            self->child_amount -= 1;
            return true;
        }
    }
    return false;
}

// allocate the global background execution vector
void start_bg_execution() {
    vec_bg_execution = new_vec(sizeof(BgExecution *));
    char *shell_type = getenv(SHELL_TYPE_ENV_KEY);
    if (shell_type != NULL && str_equals(shell_type, "DEFAULT")) {
        HAS_DFLT_SHELL_BEHAVIOR = true;
    }
}

// try to clear a child_pid from the references of stored into the
// background execution vector also cleaning up if the background execution
// is complete
bool clear_child_execution(pid_t child_pid) {
    if (is_debug()) {
        vec_print(vec_bg_execution, bg_execution_fmt);
    }
    if (vec_bg_execution->length) {
        int i;
        for (i = 0; i < vec_bg_execution->length; i++) {
            BgExecution *bg_execution = vec_bg_execution->_arr[i];
            bool has_child = bg_execution_clear_child(bg_execution, child_pid);
            if (has_child) {
                printf("[%d] %d\n", i + 1, child_pid);
                if ((bg_execution->child_pids != NULL && bg_execution->child_pids->length == 0) || bg_execution->child_amount == 0) {
                    vec_take(vec_bg_execution, i);
                    bg_execution_drop(bg_execution);
                    printf("[%d] + Done\n", i + 1);
                }
                return true;
            }
        }
    }
    return false;
}

// function to clear a background execution using a pgid
// if the provided pgid is 0 then we cleanup all background execution
// signaling all process groups that could be active
void clear_bg_execution(pid_t pgid) {
    int i;
    for (i = 0; i < vec_bg_execution->length; i++) {
        BgExecution *bg_execution = vec_bg_execution->_arr[i];
        if (bg_execution != NULL) {
            if (is_debug()) {
                bg_execution_print(bg_execution);
            }
            pid_t bg_pgid = bg_execution->pgid;
            if (bg_execution->child_pids == NULL) {
                vec_take(vec_bg_execution, i);
                i -= 1;
                bg_execution_drop(bg_execution);
            }
            if (pgid == 0) {
                killpg(bg_pgid, SIGTERM);
            } else if (bg_pgid == pgid) {
                killpg(pgid, SIGTERM);
                break;
            }
        }
    }
}

// function to deallocate the vector of background execution
void end_bg_execution() {
    if (vec_bg_execution->length) {
        clear_bg_execution(0);
    }
    vec_drop(vec_bg_execution);
}

// function to handle SIG_CHLD
// it basicallly try to acknowledge the end of a chld process and cleanup it's
// information
void sig_chld_handler(const int signal) {
    int i;
    for (i = 0; i < vec_bg_execution->length; i++) {
        BgExecution *bg_exec = vec_bg_execution->_arr[i];
        pid_t pgid = bg_exec->pgid;
        pid_t child_that_finished = waitpid(-pgid, NULL, WNOHANG);
        if (child_that_finished != -1 && clear_child_execution(child_that_finished)) {
            break;
        }
    }
}

// function to handle with user input
// that should be used as an thread procedure
void *input_thread_func(void *arg) {
    ShellState *state = arg;
    pthread_exit(state->prompt_user());
}

pthread_t input_thread;
bool has_input_thread;

void create_input_thread(ShellState *state) {
    if (has_input_thread) {
        pthread_cancel(input_thread);
    }
    pthread_create(&input_thread, NULL, input_thread_func, (void *) state);
    has_input_thread = true;
}
// function to join the input thread and receive
// the input
char *join_input_thread() {
    char *input;
    pthread_join(input_thread, (void **) &input);
    if (input == PTHREAD_CANCELED) {
        printf("\n");
        input = NULL;
    }
    has_input_thread = false;
    return input;
}
// function to more easily deal with canceling and cleaning
// up of the input thread
void cancel_thread() {
    if (has_input_thread) {
        pthread_cancel(input_thread);
        pthread_detach(input_thread);
        has_input_thread = false;
    }
}

// handler for the SIG_INT
// that will restart the input to the user
void sig_int_handler(const int signal) {
    // if (child_pgid) {
    //     killpg(child_pgid, signal);
    // }
    cancel_thread();
}

void sig_usr_handler(const int signal) {
    // sig_int_handler(SIGINT);
    clear_bg_execution(0);
    print_i_feel_weird();
    cancel_thread();
}

void unknown_cmd_info(CallResult *res, bool *should_continue,
                      int *status_code) {
    if (res->is_parent) {
        printf("Unknown command %s\n", res->additional_data);
    } else {
        *should_continue = false;
        *status_code = UnknownCommand;
    }
}

pid_t cmd_handler(ShellState *state, ExecArgs *exec_args, bool *should_continue, int *status_code, unsigned int opts) {
    bool should_fork = opts % 2 == 1;
    bool is_background = ((opts >> 1) % 2) == 1;
    bool is_session_leader = ((opts >> 2) % 2) == 1;
    CallResult *res = exec_args_call(exec_args, should_fork, !is_background, is_background && is_session_leader);
    switch (res->shell_behavior) {
        case Continue:
            break;
        case Exit:
            *should_continue = false;
            break;
        case ClearBackground:
            clear_bg_execution(0);
            break;
        case ClearBackgroundAndExit:
            clear_bg_execution(0);
            *should_continue = false;
            break;
        case Cd:
            state->change_cwd(res->additional_data);
            break;
        case UnknownCommand:
            unknown_cmd_info(res, should_continue, status_code);
            break;
    }
    pid_t child_pid = res->child_pid;
    call_result_drop(res);
    return child_pid;
}

pid_t basic_cmd_handler(ShellState *state, CallGroup *call_group,
                        bool *should_continue, int *status_code) {
    if (call_group->exec_amount) {
        int opts = CMD_HND_OPT_FORK;
        if (call_group->is_background) {
            opts = opts | CMD_HND_OPT_BG;
        }
        pid_t child_pid = cmd_handler(state, call_group->exec_arr[0],
                                      should_continue, status_code, opts);
        if (call_group->is_background) {
            setpgid(child_pid, child_pid);
            Vec *vec_child_pids = new_vec_with_capacity(sizeof(pid_t), 1);
            vec_push(vec_child_pids, child_pid);
            vec_push(vec_bg_execution, new_bg_execution(child_pid, vec_child_pids));
        }
    }
}

void sequential_cmd_handler(ShellState *state, CallGroup *call_group,
                            bool *should_continue, int *status_code) {
    int i;
    if (!call_group->is_background) {
        for (i = 0; i < call_group->exec_amount; i++) {
            cmd_handler(state, call_group->exec_arr[i], should_continue,
                        status_code, CMD_HND_OPT_FORK);
        }
    } else {
        pid_t child_pid = fork();
        if (child_pid == -1) {
            perror("fork failed!");
            exit(ForkFailed);
        } else if (child_pid > 0) {
            Vec *vec_child_pids = new_vec_with_capacity(sizeof(pid_t), call_group->exec_amount);
            vec_push(vec_child_pids, child_pid);
            vec_push(vec_bg_execution, new_bg_execution(child_pid, vec_child_pids));
            return;
        } else {
            for (i = 0; i < call_group->exec_amount; i++) {
                int opts = i != call_group->exec_amount - 1 ? CMD_HND_OPT_FORK : CMD_HND_OPT_BG;
                cmd_handler(state, call_group->exec_arr[i], should_continue,
                            status_code, opts);
            }
        }
    }
}

#define WRITE_PIPE 1
#define READ_PIPE 0

void project_piped_cmd_handler(ShellState *state, CallGroup *call_group,
                               bool *should_continue, int *status_code) {
    /**
     *  Since the project specifies that we should create another session to execute 
     * the piped command and it should be executed in a background and I've initially coded
     * the default behavior of shell's piped command over the 'piped_cmd_handler' while
     * over here I've added it's specific behavior and over the main I've used a environment variable
     * to handle it's selection
     */
    unsigned int exec_amount = call_group->exec_amount;
    pid_t child_pgid = fork();
    if (child_pgid == -1) {
        perror("fork failed!");
        exit(ForkFailed);
    } else if (child_pgid == 0) {
        child_pgid = setsid();
        int i;
        unsigned int pipes_len = exec_amount - 1;
        int pipes[pipes_len][2];
        for (i = 0; i < pipes_len; i++) {
            if (pipe(pipes[i]) < 0) {
                perror("pipe failed!\n");
                exit(1);
            }
        }
        for (i = 0; i < exec_amount - 1; i++) {
            ExecArgs *exec_args = call_group->exec_arr[i];
            pid_t child_pid = fork();
            if (child_pid == -1) {
                perror("fork failed!\n");
                exit(1);
            }
            if (child_pid) {
                setpgid(child_pid, child_pgid);
            } else {
                dup2(pipes[i][WRITE_PIPE], STDOUT_FILENO);
                close(pipes[i][READ_PIPE]);
                int stdin_fileno_idx = i - 1;
                if (i > 0) {
                    dup2(pipes[i - 1][READ_PIPE], STDIN_FILENO);
                    close(pipes[i - 1][WRITE_PIPE]);
                }
                for (i = 0; i < pipes_len; i++) {
                    if (stdin_fileno_idx >= 0 && stdin_fileno_idx != i) {
                        close(pipes[i][WRITE_PIPE]);
                        close(pipes[i][READ_PIPE]);
                    }
                }
                cmd_handler(state, exec_args, should_continue, status_code, 0);
                *should_continue = false;
                break;
            }
        }
        if (i == exec_amount - 1) {
            ExecArgs *exec_args = call_group->exec_arr[i];
            dup2(pipes[i - 1][READ_PIPE], STDIN_FILENO);
            close(pipes[i - 1][WRITE_PIPE]);
            int stdin_fileno_idx = i - 1;
            for (i = 0; i < pipes_len; i++) {
                if (stdin_fileno_idx >= 0 && stdin_fileno_idx != i) {
                    close(pipes[i][WRITE_PIPE]);
                    close(pipes[i][READ_PIPE]);
                }
            }
            cmd_handler(state, exec_args, should_continue, status_code, 0);
        }
    } else {
        BgExecution *bg_exec = new_bg_execution(child_pgid, NULL);
        bg_exec->child_amount = exec_amount;
        vec_push(vec_bg_execution, bg_exec);
    }
}

void dflt_piped_cmd_handler(ShellState *state, CallGroup *call_group,
                            bool *should_continue, int *status_code) {
    // this was my initial implementation about a implementation of the default behavior
    // from shells when executing a command
    unsigned int exec_amount = call_group->exec_amount;
    int i;
    pid_t child_pgid = 0;
    unsigned int pipes_len = exec_amount - 1;
    int pipes[pipes_len][2];
    Vec *vec_child_pids = new_vec_with_capacity(sizeof(pid_t), exec_amount);
    for (i = 0; i < pipes_len; i++) {
        if (pipe(pipes[i]) < 0) {
            perror("pipe failed!\n");
            exit(1);
        }
    }
    for (i = 0; i < exec_amount; i++) {
        ExecArgs *exec_args = call_group->exec_arr[i];
        pid_t child_pid = fork();
        if (child_pid == -1) {
            perror("fork failed!\n");
            exit(1);
        }
        if (i == 0) {
            child_pgid = child_pid ? child_pid : getpid();
        }
        if (child_pid) {
            vec_push(vec_child_pids, (void *) child_pid);
            setpgid(child_pid, child_pgid);
        } else {
            if (i < exec_amount - 1) {
                dup2(pipes[i][WRITE_PIPE], STDOUT_FILENO);
                close(pipes[i][READ_PIPE]);
            }
            int stdin_fileno_idx = i - 1;
            if (i > 0) {
                dup2(pipes[i - 1][READ_PIPE], STDIN_FILENO);
                close(pipes[i - 1][WRITE_PIPE]);
            }
            for (i = 0; i < pipes_len; i++) {
                if (stdin_fileno_idx >= 0 && stdin_fileno_idx != i) {
                    close(pipes[i][WRITE_PIPE]);
                    close(pipes[i][READ_PIPE]);
                }
            }
            cmd_handler(state, exec_args, should_continue, status_code, 0);
            *should_continue = false;
            break;
        }
    }
    if (i == exec_amount) {
        int j;
        // Closing opened and unused pipes from the parent
        for (j = 0; j < pipes_len; j++) {
            close(pipes[j][WRITE_PIPE]);
            close(pipes[j][READ_PIPE]);
        }
        if (!call_group->is_background) {
            while (waitpid(-child_pgid, NULL, WUNTRACED) != -1)
                ;
            vec_drop(vec_child_pids);
        } else {
            vec_push(vec_bg_execution, new_bg_execution(child_pgid, vec_child_pids));
        }
    }
}

void piped_cmd_handler(ShellState *state, CallGroup *call_group,
                       bool *should_continue, int *status_code) {
    if (HAS_DFLT_SHELL_BEHAVIOR) {
        dflt_piped_cmd_handler(state, call_group, should_continue, status_code);
    } else {
        project_piped_cmd_handler(state, call_group, should_continue, status_code);
    }
}

const char *WEIRD = "\n                                        .--.  .--.\n"
                    "                                       /    \\/    \\\n"
                    "                                      | .-.  .-.   \\\n"
                    "                                      |/_  |/_  |   \\\n"
                    "                                      || `\\|| `\\|    `----.\n"
                    "                                      |\\0_/ \\0_/    --,    \\_\n"
                    "                    .--\"\"\"\"\"-.       /              (` \\     `-.\n"
                    "                   /          \\-----'-.              \\          \\\n"
                    "                   \\  () ()                         /`\\          \\\n"
                    "                   |                         .___.-'   |          \\\n"
                    "                   \\                        /` \\|      /           ;\n"
                    "                    `-.___             ___.' .-.`.---.|             \\\n"
                    "                       \\| ``-..___,.-'`\\| / /   /     |              `\\\n"
                    "                        `      \\|      ,`/ /   /   ,  /\n"
                    "                                `      |\\ /   /    |\\/\n"
                    "    I feel weird...                   ,   .'`-;   '     \\/\n"
                    "                            ,    |\\-'  .'   ,   .-'`\n"
                    "                          .-|\\--;`` .-'     |\\.'\n"
                    "                         ( `\"'-.|\\ (___,.--'`'   \n"
                    "                          `-.    `\"`          _.--'\n"
                    "                             `.          _.-'`-.\n"
                    "                               `''---''``        `.";

void print_i_feel_weird() { printf("%s\n", WEIRD); }