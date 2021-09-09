#include "handlers.h"

Vec *vec_bg_execution;


BgExecution *new_bg_execution(pid_t pgid, Vec *child_pids) {
    BgExecution *val = malloc(sizeof(BgExecution));
    val->child_pids = child_pids;
    val->pgid = pgid;
    val->clear_child = bg_execution_clear_child;
    val->drop = bg_execution_drop;
    val->print = bg_execution_print;
    val->fmt = bg_execution_fmt;
    return val;
}

char *bg_execution_fmt(BgExecution *self) {
    char *str = malloc(sizeof(char) * 100);
    sprintf(str, "BgExecution { pgid: %d, child_pids_len: %ld }", self->pgid, self->child_pids->length);
    return str;
}

void bg_execution_print(BgExecution *self) {
    printf("BgExecution { pgid: %d }", self->pgid);
}

void bg_execution_drop(BgExecution *self) {
    if (self->child_pids != NULL) {
        Vec *child_pids = self->child_pids;
        child_pids->drop(child_pids);
        self->child_pids = NULL;
    }
    free(self);
}

bool bg_execution_clear_child(BgExecution *self, pid_t child_pid) {
    if (self->child_pids->length) {
        Vec *child_pids = self->child_pids;
        int i;
        for (i = 0; i < child_pids->length; i++) {
            pid_t pid = ptr_to_type(pid_t) child_pids->_arr[i];
            if (pid == child_pid) {
                child_pids->take(child_pids, i);
                return true;
            }
        }
    }
    return false;
}

void start_bg_execution() {
    vec_bg_execution = new_vec(sizeof(BgExecution *));
}

bool clear_child_execution(pid_t child_pid) {
    vec_bg_execution->print(vec_bg_execution, bg_execution_fmt);
    if (vec_bg_execution->length) {
        int i;
        for (i = 0; i < vec_bg_execution->length; i++) {
            BgExecution *bg_execution = vec_bg_execution->_arr[i];
            bool has_child = bg_execution->clear_child(bg_execution, child_pid);
            if (has_child) {
                printf("[%d] %d\n", i + 1, child_pid);
                if (bg_execution->child_pids->length == 0) {
                    vec_bg_execution->take(vec_bg_execution, i);
                    bg_execution->drop(bg_execution);
                    printf("[%d] + Done\n", i + 1);
                }
                return true;
            }
        }
    }
    return false;
}

void clear_bg_execution(pid_t *pgid) {
    int i;
    for (i = 0; i < vec_bg_execution->length; i++) {
        BgExecution *bg_execution = vec_bg_execution->_arr[i];
        if (bg_execution != NULL) {
            bg_execution->print(bg_execution);
            pid_t bg_pgid = bg_execution->pgid;
            if (pgid == NULL) {
                killpg(bg_pgid, SIGTERM);
            } else if (bg_pgid == *pgid) {
                killpg(*pgid, SIGTERM);
                break;
            }
        }
    }
}

void end_bg_execution() {
    if (vec_bg_execution->length) {
        clear_bg_execution(NULL);
    }
    vec_bg_execution->drop(vec_bg_execution);
}

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

void cancel_thread() {
    if (has_input_thread) {
        pthread_cancel(input_thread);
        pthread_detach(input_thread);
        has_input_thread = false;
    }
}

void sig_int_handler(const int signal) {
    // if (child_pgid) {
    //     killpg(child_pgid, signal);
    // }
    cancel_thread();
}

void sig_usr_handler(const int signal) {
    // sig_int_handler(SIGINT);
    clear_bg_execution(NULL);
    print_weird();
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
    CallResult *res = exec_args->call(exec_args, should_fork, !is_background, is_background && is_session_leader);
    switch (res->shell_behavior) {
        case Continue:
            break;
        case Exit:
            *should_continue = false;
            break;
        case ClearBackground:
            clear_bg_execution(NULL);
            break;
        case ClearBackgroundAndExit:
            clear_bg_execution(NULL);
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
    res->drop(res);
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
            vec_child_pids->push(vec_child_pids, child_pid);
            vec_bg_execution->push(vec_bg_execution, new_bg_execution(child_pid, vec_child_pids));
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
            vec_child_pids->push(vec_child_pids, child_pid);
            vec_bg_execution->push(vec_bg_execution, new_bg_execution(child_pid, vec_child_pids));
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
void piped_cmd_handler(ShellState *state, CallGroup *call_group,
                       bool *should_continue, int *status_code) {
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
            vec_child_pids->push(vec_child_pids, (void *) child_pid);
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
            printf("pgid=%d\n", child_pgid);
            while (waitpid(-child_pgid, NULL, WUNTRACED) != -1)
                ;
            vec_child_pids->drop(vec_child_pids);
        } else {
            vec_bg_execution->push(vec_bg_execution, new_bg_execution(child_pgid, vec_child_pids));
        }
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

void print_weird() { printf("%s\n", WEIRD); }