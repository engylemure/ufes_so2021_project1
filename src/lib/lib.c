#include "lib.h"
#include "vec/vec.h"
#include <dirent.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>

/**
 * @brief Maximum length for a PATH name considering the null termination character.
 */
#define PATH_MAX 4096
/**
 * @brief Macro to retrieve the human readable value of a "bool"
 */
#define bool_str(val) val ? "true" : "false"

char *fmt_string(void *data) {
    char *str = (char *) data;
    uint64 fmt_len = strlen(str) + 3;
    char *fmt = malloc(fmt_len);
    sprintf(fmt, "\"%s\"", str);
    return fmt;
}

Vec *new_vec_string() { return new_vec(sizeof(char *)); }

bool DEBUG_IS_ON = false;

bool is_debug() {
    return DEBUG_IS_ON;
}

void debug_lib(bool should_debug) {
    if (should_debug) {
        DEBUG_IS_ON = true;
    }
}

ShellState *new_shell_state() {
    ShellState *self = malloc(sizeof(ShellState));
    self->cwd = shell_state_cwd;
    self->home = shell_state_home;
    self->change_cwd = shell_state_change_cwd;
    self->simple_cwd = shell_state_simple_cwd;
    self->prompt_user = shell_state_prompt_user;
    self->drop = shell_state_drop;
    return self;
}

char *shell_state_cwd() {
    char cwd[PATH_MAX];
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        return strdup(cwd);
    } else {
        perror("getcwd() error");
        return NULL;
    }
}

char *shell_state_home() {
    const char *home;
    if ((home = getenv("HOME")) == NULL) {
        home = getpwuid(getuid())->pw_dir;
    }
    return home != NULL ? strdup(home) : NULL;
}

void shell_state_change_cwd(char *new_dir) {
    DIR *dir = opendir(new_dir);
    char real_dir_path[PATH_MAX];
    if (dir != NULL && realpath(new_dir, real_dir_path)) {
        chdir(real_dir_path);
    } else {
        printf("\"%s\" is a invalid directory\n", new_dir);
    }
    closedir(dir);
}


char *shell_state_simple_cwd() {
    // function to simplify the presentation of the working directory
    // hiding the home directory under the '~' and any directory that
    // would be 2 directories above using the '...'
    char *cwd = shell_state_cwd();
    char *home = shell_state_home();
    // verify if the working directory is the same as the home directory
    if (str_equals(cwd, home)) {
        return strdup("~/");
    }
    char *cwd_simplified = strstr(cwd, home);
    bool cwd_has_home = cwd_simplified != NULL;
    // removing home directory from the working directory
    if (cwd_has_home) {
        cwd_simplified = cwd_simplified + strlen(home);
    } else {
        cwd_simplified = cwd;
    }
    char *aux_pwd = cwd_simplified;
    char *last_ptr = aux_pwd;
    char *last_last_ptr = aux_pwd;
    while ((aux_pwd = strchr(aux_pwd, '/')) != NULL) {
        last_last_ptr = last_ptr;
        last_ptr = aux_pwd;
        aux_pwd = aux_pwd + sizeof(char);
    }
    bool large_dir = last_last_ptr != last_ptr;
    unsigned int cwd_simplified_len = strlen(last_last_ptr) + (cwd_has_home ? 3 : 1) + (large_dir ? 3 : 0);
    cwd_simplified = malloc(sizeof(char) * cwd_simplified_len);
    cwd_simplified[0] = '\0';
    // adding the '~' to inform the user that he is under the home directory
    if (cwd_has_home) {
        strcat(cwd_simplified, "~");
    }
    // adding the '/...' to inform the user that he has more than 2 directories
    // above the presented directories
    if (large_dir) {
        strcat(cwd_simplified, "/...");
    }
    strcat(cwd_simplified, last_last_ptr);
    free(cwd);
    free(home);
    return cwd_simplified;
}

void shell_state_drop(ShellState *self) {
    free(self);
}

// Some ansi values to manipulate the shell color presentation
const char BlueAnsi[] = "\033[94m";
const char JojoAnsi[] = "\033[95m";
const char EndAnsi[] = "\033[0m";
const char RedAnsi[] = "\033[91m";
const char YellowAnsi[] = "\033[93m";

char *shell_state_prompt_user(ShellState *state) {
    if (state != NULL) {
        char input[MAX_SHELL_INPUT];
        char *pwd = state->simple_cwd();
        fflush(stdout);
        printf("%s%s > %s%svsh%s%s > %s", BlueAnsi, pwd, EndAnsi, JojoAnsi, EndAnsi,
               BlueAnsi, EndAnsi);
        free(pwd);
        fflush(stdin);
        fgets(input, MAX_SHELL_INPUT, stdin);
        input[strcspn(input, "\n")] = '\0';
        return strdup(input);
    } else {
        perror("provided ShellState is NULL\n");
        exit(1);
    }
}

CallResult *new_call_result(enum ShellBehavior s_behavior, char *additional_data, bool is_parent, pid_t child_pid) {
    CallResult *self = malloc(sizeof(CallResult));
    self->shell_behavior = s_behavior;
    self->additional_data = additional_data;
    self->is_parent = is_parent;
    self->child_pid = child_pid;
    return self;
}

void call_result_drop(CallResult *self) {
    if (self->additional_data != NULL) {
        free(self->additional_data);
        self->additional_data = NULL;
    }
    free(self);
}


ExecArgs *new_exec_args(unsigned int argc, char **argv) {
    ExecArgs *self = malloc(sizeof(ExecArgs));
    self->argc = argc;
    self->argv = argv;
    return self;
}

ExecArgs *new_exec_args_from_vec_str(Vec *vec) {
    int argc = vec->length;
    char **argv = malloc(sizeof(char *) * (argc + 1));
    int i, j;
    for (i = 0, j = 0; i < argc; i++) {
        char *str = vec_get(vec, i);
        if (strlen(str)) {
            argv[j++] = str;
        }
    }
    argv[j] = NULL;
    vec_drop(vec);
    return new_exec_args(argc, argv);
}

Vec *new_vec_exec_args() {
    return new_vec(sizeof(ExecArgs *));
}

void exec_args_drop(ExecArgs *self) {
    if (self->argv != NULL) {
        int i;
        for (i = 0; i < self->argc; i++) {
            free(self->argv[i]);
        }
        free(self->argv);
    }
    free(self);
}

char *exec_args_fmt(ExecArgs *data) {
    char *argv_str = array_fmt(data->argc, (void **) data->argv, fmt_string);
    unsigned int str_len = strlen(argv_str);
    char *str = malloc(sizeof(char) * (str_len + 40));
    sprintf(str, "ExecArgs { argc: %d, argv: %s }", data->argc, argv_str);
    free(argv_str);
    return str;
}

void exec_args_print(ExecArgs *self) {
    char *str = exec_args_fmt(self);
    printf("%s", str);
    free(str);
}

CallResult *exec_args_call(ExecArgs *exec_args, bool should_fork, bool should_wait, bool is_session_leader) {
    enum ShellBehavior shell_behavior = UnknownCommand;
    char *program_name = NULL;
    bool is_parent = true;
    pid_t child_pid = 0;
    // We are considering that an empty ExecArgs* should have a ShellBehavior
    // of continuing it's execution
    if (exec_args->argc == 0) {
        shell_behavior = Continue;
    } else {
        program_name = exec_args->argv[0];
        // Over here since I've tried to mimic the basic behavior of a shell I've kept
        // exit as one of the command's used to finish it's execution but as the project require's
        // that we use the "armageddon" as an internal operation as well I've also used it
        if (str_equals(program_name, "exit")) {
            shell_behavior = Exit;
        } else if (str_equals(program_name, "armageddon")) {
            shell_behavior = ClearBackgroundAndExit;
        } else if (str_equals(program_name, "liberamoita")) {
            // identification of "liberamoita" operation
            shell_behavior = ClearBackground;
        } else if (str_equals(program_name, "cd")) {
            // Over here we are also adding the identification of the internal operation "cd"
            // that has the objective of changing the working directory of the shell
            shell_behavior = Cd;
        } else {
            // Since the program_name isn't equal to it's internal operation
            // we are handling over here execution of it using some of the params
            // to control it's behavior, if we should_fork we fork it and check it's return
            // value other wise we set the child_pid as 0 that would be the same value returned
            // for the child of the fork execution.
            child_pid = should_fork ? fork() : 0;
            if (child_pid == -1) {
                perror("'fork' failed!\n");
                exit(ForkFailed);
            } else if (child_pid) {
                // On the parent we define the shell behavior as continue
                shell_behavior = Continue;
                // and wait if necessary
                if (should_wait) {
                    int wait_status;
                    waitpid(child_pid, &wait_status, WUNTRACED);
                    if (WIFEXITED(wait_status) && (WEXITSTATUS(wait_status) == UnknownCommand)) {
                        shell_behavior = UnknownCommand;
                    }

                    if (WIFSTOPPED(wait_status)) {
                        // since the project didn't require that we handle the
                        // case of stopping the child execution we are ignoring it for now.
                    }
                }
            } else {
                if (is_session_leader) {
                    setsid();
                }
                // we try to execute the program with the provided arguments
                // and otherwise we try to alert the user that the program was not found
                execvp(program_name, exec_args->argv);
                is_parent = false;
            }
        }
    }
    char *aux_str;
    switch (shell_behavior) {
        case Cd: {
            if (exec_args->argc == 1) {
                char *home_env = shell_state_home();
                aux_str = home_env != NULL ? strdup(home_env) : NULL;
            } else if (exec_args->argv[1][0] == '~') {
                char *home_env = shell_state_home();
                unsigned int home_len = strlen(home_env);
                unsigned int aux_str_len = home_len + strlen((exec_args->argv[1]));
                aux_str = malloc(sizeof(char) * aux_str_len);
                sprintf(aux_str, "%s%s", home_env, (exec_args->argv[1] + 1));
                free(home_env);
            } else {
                aux_str = strdup(exec_args->argv[1]);
            }
        } break;
        case UnknownCommand:
            aux_str = strdup(program_name);
            break;
        default:
            aux_str = NULL;
    }
    return new_call_result(shell_behavior, aux_str, is_parent, child_pid);
}

CallGroup *new_call_group(Vec *vec_exec_args, enum CallType type, bool is_background) {
    CallGroup *self = malloc(sizeof(CallGroup));
    unsigned int count = vec_exec_args->length;
    ExecArgs **exec_arr = malloc(sizeof(ExecArgs *) * count);
    int i;
    for (i = 0; i < count; i++) {
        exec_arr[i] = vec_get(vec_exec_args, i);
    }
    vec_drop(vec_exec_args);
    self->type = type;
    self->exec_amount = count;
    self->exec_arr = exec_arr;
    self->is_background = is_background;
    return self;
}

char *call_group_fmt(CallGroup *self) {
    int i;
    char *exec_args_str = array_fmt(self->exec_amount, (void **) self->exec_arr, (char *(*)(void *)) exec_args_fmt);
    char *call_group_type;
    switch (self->type) {
        case Basic:
            call_group_type = "Basic";
            break;
        case Piped:
            call_group_type = "Piped";
            break;
        case Sequential:
            call_group_type = "Sequential";
            break;
    }
    char *str = malloc(sizeof(char) * (strlen(exec_args_str) + 100));
    sprintf(str, "CallGroup { exec_amount: %d, type: %s, is_background: %s, exec_arr: %s }", self->exec_amount, call_group_type, bool_str(self->is_background), exec_args_str);
    free(exec_args_str);
    return str;
}

void call_group_print(CallGroup *self) {
    char *str = call_group_fmt(self);
    printf("%s", str);
    free(str);
}

void call_group_drop(CallGroup *self) {
    int i;
    for (i = 0; i < self->exec_amount; i++) {
        exec_args_drop(self->exec_arr[i]);
    }
    free(self->exec_arr);
    self->exec_arr = NULL;
    free(self);
}

Vec *new_vec_call_group() { return new_vec(sizeof(CallGroup *)); }

CallGroups *new_call_groups(Vec *vec_call_group, bool has_parsing_err) {
    CallGroups *self = malloc(sizeof(CallGroups));
    self->len = has_parsing_err || vec_call_group == NULL ? 0 : vec_call_group->length;
    if (self->len) {
        self->groups = malloc(sizeof(CallGroup **) * self->len);
        int i;
        for (i = 0; i < self->len; i++) {
            self->groups[i] = vec_get(vec_call_group, i);
        }
    } else {
        self->groups = NULL;
    }

    self->has_parsing_error = has_parsing_err;
    return self;
}

void call_group_specific_type(enum CallType expected_type, enum CallType *type,
                              Vec **vec_str, Vec *vec_call_group,
                              Vec **vec_exec_args, bool *is_background) {
    ExecArgs *exec_arg = new_exec_args_from_vec_str(*vec_str);
    *vec_str = new_vec_string();
    if ((*type == Basic || *type == expected_type) && !*is_background) {
        *type = expected_type;
       vec_push(*vec_exec_args, exec_arg);
    } else {
        vec_push(*vec_exec_args, exec_arg);
        vec_push(vec_call_group,
                             new_call_group(*vec_exec_args, *type, *is_background));
        *vec_exec_args = new_vec_exec_args();
        *type = Basic;
        *is_background = false;
    }
}

CallGroups *call_groups_from_input(char *input) {
    Vec *args = vec_parse_arg_res_from_shell_input(input);
    if (args != NULL) {
        Vec *vec_call_group = new_vec_call_group();
        Vec *vec_exec_args = new_vec_exec_args();
        Vec *vec_string = new_vec_string();
        enum CallType type = Basic;
        bool is_background = false;
        int len = args->length;
        ParseArgRes **args_res = (ParseArgRes **) vec_take_arr(args);
        int i;
        for (i = 0; i < len; i++) {
            ParseArgRes *parse_arg_res = args_res[i];
            char *str = parse_arg_res_take_arg(parse_arg_res);
            if (strlen(str) > 2 && str[0] == '$') {
                char *env_value = strdup(getenv(str + 1));
                free(str);
                str = env_value;
            }
            switch (parse_arg_res->type) {
                case Bar:
                    call_group_specific_type(Piped, &type, &vec_string, vec_call_group,
                                             &vec_exec_args, &is_background);
                    free(str);
                    break;
                case At:
                    is_background = true;
                    call_group_specific_type(type, &type, &vec_string, vec_call_group,
                                             &vec_exec_args, &is_background);
                    free(str);
                    break;
                case DoubleAt:
                    call_group_specific_type(Sequential, &type, &vec_string, vec_call_group,
                                             &vec_exec_args, &is_background);
                    free(str);
                    break;
                case Simple:
                default:
                    vec_push(vec_string, str);
                    break;
            }
            parse_arg_res_drop(parse_arg_res);
        }
        free(args_res);
        if (vec_string->length) {
            vec_push(vec_exec_args, new_exec_args_from_vec_str(vec_string));
        } else {
            vec_drop(vec_string);
        }
        vec_push(vec_call_group,
                             new_call_group(vec_exec_args, type, is_background));
        if (DEBUG_IS_ON)
           vec_print(vec_call_group, (char *(*)(void *)) call_group_fmt);
        CallGroups *val = new_call_groups(vec_call_group, false);
        vec_drop(vec_call_group);
        return val;
    } else {
        return new_call_groups(NULL, true);
    }
}

void call_groups_drop(CallGroups *self) {
    if (self->groups != NULL) {
        int i;
        for (i = 0; i < self->len; i++) {
            CallGroup *call_group = self->groups[i];
            call_group_drop(call_group);
        }
        free(self->groups);
        self->groups = NULL;
        self->len = 0;
    }
    free(self);
}

char *call_groups_fmt(CallGroups *self) {
    unsigned int str_len = 58;
    char *c_group_str = array_fmt(self->len, (void **) self->groups, (char *(*)(void *)) call_group_fmt);
    char *str = malloc(sizeof(char) * (strlen(c_group_str) + str_len));
    sprintf(str, "CallGroups { len: %d, has_parsing_error: %s, groups: %s }", self->len, bool_str(self->has_parsing_error), c_group_str);
    free(c_group_str);
    return str;
}

void call_groups_print(CallGroups *self) {
    char *str = call_groups_fmt(self);
    printf("%s", str);
    free(str);
}

char *str_from_vec_char(Vec **vec, bool should_alloc) {
    char *str = malloc(sizeof(char) * ((*vec)->length + 1));
    int i;
    for (i = 0; i < (*vec)->length; i++) {
        str[i] = (char) (uintptr_t) vec_get(*vec, i);
    }
    str[i] = '\0';
    vec_drop(*vec);
    if (should_alloc) {
        *vec = new_vec(sizeof(char));
    }
    return str;
}


char *fmt_char(char data) {
    char *str = malloc(sizeof(char) * 2);
    str[0] = data;
    str[1] = '\0';
    return str;
}

Vec *vec_parse_arg_res_from_shell_input(char *shell_input) {
    Vec *args = new_vec(sizeof(ParseArgRes *));
    Vec *char_buffer = new_vec(sizeof(char));
    bool has_error = false;
    enum ArgParseState arg_parse_state = Ignore;
    int str_len = strlen(shell_input);
    int i = 0;
    char c;
    if (str_len > 0 && (c = shell_input[0]) && (c == '|' || c == '&')) {
        has_error = true;
        i = str_len;
    }
    void (*push_in_buffer)(Vec * vec, char c) =
            (void (*)(Vec *, char)) vec_push;
    for (; i < str_len; i++) {
        c = shell_input[i];
        switch (c) {
            case ' ':
                if (arg_parse_state == Ignore) {
                    continue;
                } else if (arg_parse_state == LeftQuote) {
                    push_in_buffer(char_buffer, c);
                } else if (char_buffer->length) {
                    vec_push(args, new_parse_arg_res(
                                             str_from_vec_char(&char_buffer, true), Simple));
                    arg_parse_state = Ignore;
                };
                break;
            case '|': {
                vec_push(args,
                           new_parse_arg_res(str_from_vec_char(&char_buffer, true), Bar));
                arg_parse_state = Ignore;
            } break;
            case '&': {
                enum ArgType type = At;
                if (i + 1 < str_len && shell_input[i + 1] == '&') {
                    type = DoubleAt;
                    i += 1;
                }
                vec_push(
                        args, new_parse_arg_res(str_from_vec_char(&char_buffer, true), type));
                arg_parse_state = Ignore;
            } break;
            case '"':
                if (char_buffer->length &&
                    ptr_to_type(char) vec_last(char_buffer) == '\\') {
                    vec_pop(char_buffer);
                    push_in_buffer(char_buffer, c);
                } else {
                    if (arg_parse_state == LeftQuote) {
                        vec_push(args, new_parse_arg_res(
                                                 str_from_vec_char(&char_buffer, true), Quoted));
                        arg_parse_state = Ignore;
                    } else {
                        arg_parse_state = LeftQuote;
                    }
                }
                break;
            default:
                if (arg_parse_state == Ignore) {
                    arg_parse_state = Word;
                }
                push_in_buffer(char_buffer, c);
                break;
        }
    }
    if (char_buffer->length) {
        if (arg_parse_state == Word) {
            vec_push(args, new_parse_arg_res(str_from_vec_char(&char_buffer, false),
                                               Simple));
        } else {
            vec_drop(char_buffer);
        }
        if (arg_parse_state == LeftQuote) {
            has_error = true;
        }
    } else {
        vec_drop(char_buffer);
    }
    if (DEBUG_IS_ON)
        vec_print(args, (char *(*)(void *)) parse_arg_res_fmt);
    if (has_error) {
        vec_drop(args);
        return NULL;
    }
    return args;
}

ParseArgRes *new_parse_arg_res(char *arg, enum ArgType type) {
    ParseArgRes *self = malloc(sizeof(ParseArgRes));
    self->arg = arg;
    self->type = type;
    return self;
}

void parse_arg_res_drop(ParseArgRes *self) {
    if (self->arg != NULL) {
        free(self->arg);
        self->arg = NULL;
    }
    free(self);
}

char *parse_arg_res_take_arg(ParseArgRes *self) {
    char *arg = self->arg;
    self->arg = NULL;
    return arg;
}

char *parse_arg_res_fmt(ParseArgRes *self) {
    char *str = malloc(sizeof(char) * (strlen(self->arg) + 55));
    char *type;
    switch (self->type) {
        case Simple:
            type = "Simple";
            break;
        case Quoted:
            type = "Quoted";
            break;
        case Bar:
            type = "Bar";
            break;
        case At:
            type = "At";
            break;
        default:
            type = "DoubleAt";
    }
    sprintf(str, "ParseArgRes { type: %s, arg: \"%s\" }", type, self->arg);
    return str;
}

void parse_arg_res_print(ParseArgRes *self) {
    char *str = parse_arg_res_fmt(self);
    printf("%s", str);
    free(str);
}