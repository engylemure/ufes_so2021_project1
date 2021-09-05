//
// Created by jordao on 04/09/2021.
//

#include "new_lib.h"
#include "util/vec/vec.h"
#include <stdio.h>
#include <limits.h>
#include <string.h>
#include <stdlib.h>
#include <pwd.h>
#include <dirent.h>

#define str_equals(self, other) strcmp(self, other) == 0

char *fmt_string(void *data) {
    char *str = (char *) data;
    uint64 fmt_len = strlen(str) + 3;
    char *fmt = malloc(fmt_len);
    fmt[0] = '"';
    fmt[1] = '\0';
    strcat(fmt, str);
    fmt[fmt_len - 2] = '"';
    fmt[fmt_len - 1] = '\0';
    return fmt;
}

ShellState *new_shell_state() {
    ShellState *self = malloc(sizeof(ShellState));
    self->cwd = shell_state_cwd;
    self->home = shell_state_home;
    self->change_pwd = shell_state_change_pwd;
    self->simple_pwd = shell_state_simple_pwd;
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

void shell_state_change_pwd(char *new_dir) {
    DIR *dir = opendir(new_dir);
    char real_dir_path[PATH_MAX];
    if (dir != NULL && realpath(new_dir, real_dir_path)) {
        chdir(real_dir_path);
    } else {
        printf("\"%s\" is a invalid directory\n", new_dir);
    }
    closedir(dir);
}

char *shell_state_simple_pwd() {
    char *cwd = shell_state_cwd();
    char *home = shell_state_home();
    if (str_equals(cwd, home)) {
        return strdup("~/");
    }
    char *cwd_simplified = strstr(cwd, home);
    bool cwd_has_home = cwd_simplified != NULL;
    if (cwd_simplified != NULL) {
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
    bool large_dir = last_last_ptr != aux_pwd;
    uint cwd_simplified_len = strlen(last_last_ptr) + (cwd_has_home ? 3 : 1) + (large_dir ? 3 : 0);
    cwd_simplified = malloc(sizeof(char) * cwd_simplified_len);
    cwd_simplified[0] = '\0';
    if (cwd_has_home) {
        strcat(cwd_simplified, "~/");
    }
    if (large_dir) {
        strcat(cwd_simplified, "../");
    }
    strcat(cwd_simplified, last_last_ptr);
    return cwd_simplified;
}

void shell_state_drop(ShellState *self) {
    free(self);
}

CallResult *new_call_result(enum ShellBehavior s_behavior, char *additional_data, bool is_parent, pid_t child_pid) {
    CallResult *self = malloc(sizeof(CallResult));
    self->shell_behavior = s_behavior;
    self->additional_data = additional_data;
    self->is_parent = is_parent;
    self->child_pid = child_pid;
    self->drop = call_result_drop;
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
    self->drop = exec_args_drop;
    self->fmt = exec_args_fmt;
    self->print = exec_args_print;
    return self;
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
    ExecArgs *exec_args = data;
    uint64 argv_len = 15;
    int i;
    for (i = 0; i < exec_args->argc; i++) {
        argv_len += strlen(exec_args->argv[i]) + 1;
    }
    char *argv = malloc(sizeof(char) * argv_len);
    argv[0] = '[';
    argv[1] = '\0';
    for (i = 0; i < exec_args->argc; i++) {
        char *formatted_call = fmt_string(exec_args->argv[i]);
        strcat(argv, formatted_call);
        free(formatted_call);
        if (i != exec_args->argc - 1)
            strcat(argv, ",");
    }
    strcat(argv, "]");
    return argv;
}

void exec_args_print(ExecArgs *self) {
    char* str = self->fmt(self);
    printf("%s", str);
    free(str);
}