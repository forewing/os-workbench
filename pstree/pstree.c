#include <assert.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

const char* PROC_DIR = "/proc";
const char* MAXPID_FILE = "/proc/sys/kernel/pid_max";

void init();
void scan_root();
void scan_task(const char* dir_current, int ppid);
void draw(int r);

void tree_add_child(int parent, int child);
void tree_sort(int root);
int tree_cmp(const void* a, const void* b);

int arg_n = 0;
int arg_p = 0;

typedef struct Process {
    char* comm;
    pid_t ppid;
    int l, r;
    int is_root;
} Process;

pid_t pid_max;
Process* proc;

int* list;

char tabs[1024];
int tabs_ptr = 0;

int main(int argc, char* argv[]) {
    // printf("Hello, World!\n");
    int i;
    for (i = 0; i < argc; i++) {
        assert(argv[i]);  // always true
        // printf("argv[%d] = %s\n", i, argv[i]);
        if (argv[i][0] == '-') {
            if (argv[i][1] == '-') {
                if (strcmp(argv[i], "--numeric-sort") == 0) {
                    arg_n = 1;
                } else if (strcmp(argv[i], "--show-pids") == 0) {
                    arg_p = 1;
                } else if (strcmp(argv[i], "--version") == 0) {
                    printf("pstree for OSLab M1\n***REMOVED*** ***REMOVED*** ***REMOVED***\n");
                    return 0;
                }
            } else {
                for (int j = 1; j < strlen(argv[i]); j++) {
                    if (argv[i][j] == 'n') {
                        arg_n = 1;
                    } else if (argv[i][j] == 'p') {
                        arg_p = 1;
                    } else if (argv[i][j] == 'V') {
                        printf("pstree for OSLab M1\n***R***REMOV***REMOVED*****REMOVED*** ***REMOVED***\n");
                        return 0;
                    }
                }
            }
        }
    }

    assert(!argv[argc]);  // always true

    // printf("n: %d, p: %d\n", arg_n, arg_p);

    init();
    scan_root();
    memset(tabs, 0, sizeof(tabs_ptr));
    draw(1);

    free(list);
    free(proc);
    return 0;
}

void init() {
    FILE* pFile;
    pFile = fopen(MAXPID_FILE, "r");
    assert(pFile);
    fscanf(pFile, "%d", &pid_max);  // Get the max pid
    fclose(pFile);

    // printf("Init with pid_max = %d\n", (int)pid_max);

    list = (int*)malloc(sizeof(int) * ((int)pid_max + 1));
    memset(list, 0, sizeof(int) * ((int)pid_max + 1));

    proc = (Process*)malloc(sizeof(Process) * (pid_max + 1));
    for (int i = 0; i < pid_max; i++) {
        proc[i].comm = NULL;
        proc[i].ppid = 0;
        proc[i].l = proc[i].r = 0;
        proc[i].is_root = 0;
    }

    return;
}

void scan_root() {
    DIR* dir = opendir(PROC_DIR);
    assert(dir != NULL);
    struct dirent* p = NULL;
    while ((p = readdir(dir)) != NULL) {
        if (!(p->d_name[0] <= '9' && p->d_name[0] >= '0')) {
            continue;
        }

        char dir_proc[256];
        char stat_file[256];
        snprintf(dir_proc, 256, "%s%s%s", PROC_DIR, "/", p->d_name);
        snprintf(stat_file, 256, "%s%s", dir_proc, "/stat");
        FILE* pFile;
        pFile = fopen(stat_file, "r");

        if (pFile == NULL) {
            continue;
        }

        int pid;
        char comm[256];
        char state[16];
        int ppid;
        fscanf(pFile, "%d (%[^)]) %s %d", &pid, comm, state, &ppid);
        fclose(pFile);

        if ((ppid == 2 || pid == 2) ||  // filter of kthreadd(2)
            !(pid < pid_max && ppid < pid_max)) {
            continue;
        }

        int comm_len = strlen(comm) + 1;
        proc[pid].comm = (char*)malloc(sizeof(char) * (comm_len));
        strncpy(proc[pid].comm, comm, comm_len);
        proc[pid].comm[comm_len - 1] = '\0';

        proc[pid].is_root = 1;

        tree_add_child(ppid, pid);

        char task_dir[256];  // go for tasks
        snprintf(task_dir, 256, "%s%s", dir_proc, "/task");
        scan_task(task_dir, pid);
    }
    closedir(dir);
    return;
}

void scan_task(const char* dir_current, int ppid) {
    // printf("Scanning %s\n", dir_current);
    DIR* dir = opendir(dir_current);
    if (!dir) {
        return;
    }
    struct dirent* p = NULL;
    while ((p = readdir(dir)) != NULL) {
        char dir_proc[256];
        char stat_file[256];
        snprintf(dir_proc, 256, "%s%s%s", dir_current, "/", p->d_name);
        snprintf(stat_file, 256, "%s%s", dir_proc, "/stat");
        FILE* pFile;
        pFile = fopen(stat_file, "r");

        // printf("%s\n", stat_file);
        if (pFile == NULL) {
            continue;
        }

        int pid;
        char comm[256];
        fscanf(pFile, "%d (%[^)])", &pid, comm);
        fclose(pFile);

        if (!(pid < pid_max && ppid < pid_max) || !(proc[pid].comm == NULL) || (pid == ppid)) {
            continue;
        }

        int comm_len = strlen(comm) + 1;
        proc[pid].comm = (char*)malloc(sizeof(char) * (comm_len));
        strncpy(proc[pid].comm, comm, comm_len);
        proc[pid].comm[comm_len - 1] = '\0';

        tree_add_child(ppid, pid);
    }
    closedir(dir);
    return;
}

void draw(int r) {
    if (r == 0) {
        return;
    }

    tree_sort(r);

    int tabs_ptr_backup = tabs_ptr;

    if (r != 1) {
        printf("-");
        tabs_ptr += 1;
    }
    if (proc[r].is_root == 0) {  // have no child, print within {}
        printf("{%s}", proc[r].comm);
        tabs_ptr += (strlen(proc[r].comm) + 2);
    } else {
        printf("%s", proc[r].comm);
        tabs_ptr += strlen(proc[r].comm);
    }
    if (arg_p == 1) {
        printf("(%d)", r);
        char pid_str[32];
        sprintf(pid_str, "%d", r);
        tabs_ptr += (2 + strlen(pid_str));
    }
    if (proc[r].l != 0) {
        if (proc[proc[r].l].r != 0) {
            tabs[tabs_ptr + 1] = 1;
            printf("-+");
        } else {
            tabs[tabs_ptr + 1] = 0;
            printf("--");
        }
        tabs_ptr += 2;
        // tabs[tabs_ptr - 1] = 1;
        draw(proc[r].l);
        int son_ptr = proc[proc[r].l].r;
        while (son_ptr != 0) {
            for (int i = 0; i < tabs_ptr - 1; i++) {
                if (tabs[i] == 0) {
                    printf(" ");
                } else {
                    printf("|");
                }
            }
            if (proc[son_ptr].r != 0) {
                printf("|");
            } else {
                tabs[tabs_ptr - 1] = 0;
                printf("`");
            }
            draw(son_ptr);
            son_ptr = proc[son_ptr].r;
        }
    } else {
        printf("\n");
    }
    for (int i = tabs_ptr_backup; i < tabs_ptr; i++) {
        tabs[i] = 0;
    }
    tabs_ptr = tabs_ptr_backup;
    return;
}

void tree_add_child(int parent, int child) {
    proc[child].ppid = parent;

    if (proc[parent].l != 0) {  // Building the binary tree, left is son, right is bro.
        int child_ptr = proc[parent].l;
        while (proc[child_ptr].r != 0) {
            child_ptr = proc[child_ptr].r;
        }
        proc[child_ptr].r = child;
    } else {
        proc[parent].l = child;
    }
}

void tree_sort(int root) {
    if (proc[root].l == 0) {
        return;
    }

    int ptr = 0;
    list[ptr] = proc[root].l;
    while (proc[list[ptr]].r != 0) {
        list[ptr + 1] = proc[list[ptr]].r;
        ptr += 1;
    }

    qsort(list, ptr + 1, sizeof(int), tree_cmp);

    proc[root].l = list[0];
    for (int i = 0; i < ptr; i++) {
        proc[list[i]].r = list[i + 1];
    }
    proc[list[ptr]].r = 0;

    memset(list, 0, sizeof(int) * (ptr + 1));

    return;
}

int tree_cmp(const void* a, const void* b) {  // return 1 if a should be placed after b
    int _a = *(int*)a;
    int _b = *(int*)b;

    if (arg_n == 0) {
        return strcmp(proc[_a].comm, proc[_b].comm) > 0;
    } else {
        return _a > _b;
    }
}
