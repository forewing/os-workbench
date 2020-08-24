#include <assert.h>
#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#define BUFFER_SIZE (1 << 20)
char buffer[BUFFER_SIZE];

#define NAME_SIZE 64
typedef struct syscall_stat_t {
    char name[NAME_SIZE];
    double time;
} syscall_stat_t;
syscall_stat_t syscall_stat[512];
int syscall_stat_nr = 0;
double syscall_time_total = 0.0;
char* argv_n[128];
time_t timer = 0;
struct winsize window_info;

int cmp(const void* a, const void* b) {
    return ((syscall_stat_t*)a)->time < ((syscall_stat_t*)b)->time;
}
void clear() {
    printf("\033[H");
    printf("\033[2J");
}
void draw_raw(int max_h) {
    int h = syscall_stat_nr;
    if (max_h != 0 && syscall_stat_nr > max_h) {
        h = max_h;
    }
    qsort(syscall_stat, syscall_stat_nr, sizeof(syscall_stat_t), cmp);
    for (int i = 0; i < h; i++) {
        printf("%d\t%s\t%lf%%\n", i, syscall_stat[i].name,
               syscall_stat[i].time / syscall_time_total * 100);
    }
    fflush(stdout);
}
void draw() {
    int time_now;
    if ((time_now = time(0)) != timer) {
        timer = time_now;
        clear();
        draw_raw(window_info.ws_row - 1);
    }
}
int main(int argc, char* argv[]) {
    if (argc < 2) {
        printf("Usage: ./sperf CMD [ARG]\n");
        return 0;
    }

    int pipefd[2];  // 0 for read, 1 for write
    if (pipe(pipefd) != 0) {
        perror("pipe");
        assert(0);
    }

    int pid = fork();
    if (pid < 0) {
        perror("fork");
        assert(0);
    } else if (pid == 0) {  // child
        close(pipefd[0]);
        // dup2(pipefd[1], STDOUT_FILENO);
        freopen("/dev/null", "w", stdout);
        dup2(pipefd[1], STDERR_FILENO);
        close(pipefd[1]);
        argv_n[0] = "strace";
        argv_n[1] = "-T";
        for (int i = 1; i <= argc; i++) {
            argv_n[i + 1] = argv[i];
        }
        argv_n[argc + 1] = NULL;
        if (execv("/usr/bin/strace", argv_n) != 0) {
            perror("execve");
            assert(0);
        }
    } else {  // parent
        ioctl(STDOUT_FILENO, TIOCGWINSZ, &window_info);
        memset(syscall_stat, 0, sizeof(syscall_stat));
        dup2(pipefd[0], STDIN_FILENO);
        while (1) {
            int ptr_line = 0;
            int ptr_name = 0;
            int ptr_syscall = -1;
            char name[NAME_SIZE];
            double time_inc = 0.0;

            while ((buffer[ptr_line++] = getchar()) != '\n') {
                if (ptr_line >= BUFFER_SIZE) {
                    printf("line too big\n");
                    break;
                }
            }
            buffer[ptr_line - 1] = '\0';
            if (buffer[0] == '+' && buffer[1] == '+' && buffer[2] == '+') {
                break;
            }
            if (buffer[ptr_line - 2] != '>') {
                continue;
            }
            for (int i = 0; i < ptr_line && i < NAME_SIZE - 1; i++) {
                if (buffer[i] == '(') {
                    break;
                }
                name[ptr_name++] = buffer[i];
            }
            name[ptr_name] = '\0';
            for (int i = 0; i < syscall_stat_nr; i++) {
                if (strcmp(name, syscall_stat[i].name) == 0) {
                    ptr_syscall = i;
                    break;
                }
            }
            if (ptr_syscall == -1) {
                ptr_syscall = (syscall_stat_nr++);
                strcpy(syscall_stat[ptr_syscall].name, name);
                syscall_stat[ptr_syscall].time = 0.0;
            }
            for (int i = ptr_line; i >= 0; i--) {
                if (buffer[i] == '<') {
                    char time_str[NAME_SIZE];
                    int ptr_time = 0;
                    for (int j = i + 1; j < ptr_line - 2; j++) {
                        if (ptr_time >= NAME_SIZE - 1) {
                            break;
                        }
                        time_str[ptr_time++] = buffer[j];
                    }
                    time_str[ptr_time] = '\0';
                    time_inc = atof(time_str);
                    break;
                }
            }
            syscall_stat[ptr_syscall].time += time_inc;
            syscall_time_total += time_inc;
            if (isatty(STDOUT_FILENO)) {
                draw();
            }
        }
        if (isatty(STDOUT_FILENO)) {
            clear();
        }
        draw_raw(0);
    }
    return 0;
}
