#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define READ_BUF_SIZE 4096
char read_buf[READ_BUF_SIZE];
#define CMD_BUF_SIZE 2048
char cmd_buf[CMD_BUF_SIZE];
#define FUNC_BUF_SIZE 65536
char func_buf[FUNC_BUF_SIZE];
// char func_prefix_buf[FUNC_BUF_SIZE];
// char func_prefix_buf_all[FUNC_BUF_SIZE];
#define NAME_BUF_SIZE 256
char name_buf[NAME_BUF_SIZE];

const char arch_flag[] =
#ifdef __x86_64__
    "-m64"
#else
    "-m32"
#endif
    ;

typedef struct func_inst_t {
    void* func;
    void* handle;
    char name[NAME_BUF_SIZE];
    char src[L_tmpnam + 2];
    char dst[L_tmpnam + 3];
} func_inst_t;
#define FUNC_LIST_SIZE 4096
func_inst_t* func_inst_list[FUNC_LIST_SIZE];
int func_inst_nr = 0;

int gets_s(char* str, int size) {
    char c;
    int ptr = 0, iRet = 0;
    while ((c = getchar()) != '\n' && c != EOF) {
        str[ptr++] = c;
        if (ptr >= size - 1) {
            printf("Line too long, buffer overflow.");
            iRet = -1;
            break;
        }
    }
    if (c == EOF) {
        iRet = 1;
    }
    str[ptr] = '\0';
    return iRet;
}

func_inst_t* compile_so(const char* str, const char* name) {
    char src[L_tmpnam + 2];
    char dst[L_tmpnam + 3];
    char* ptr;
    if ((ptr = tmpnam(NULL)) == NULL) {
        printf("Fail to create tmp file.\n");
        return NULL;
    } else {
        sprintf(src, "%s.c", ptr);
        sprintf(dst, "%s.so", ptr);
    }

    FILE* pSrc = fopen((const char*)src, "w");
    if (pSrc == NULL) {
        printf("fail to open tmp file.\n");
        return NULL;
    }
    fprintf(pSrc, "%s", str);
    fclose(pSrc);

    if (!system(NULL)) {
        printf("System cmd not reachable.\n");
        return NULL;
    }

    sprintf(cmd_buf, "gcc %s -shared -o %s -fPIC %s > /dev/null 2>&1",
            arch_flag, dst, src);
    if (system(cmd_buf) != 0) {
        printf("System failed.\n");
        return NULL;
    }

    void* dl_handle;
    int (*func)();
    char* error;

    dl_handle = dlopen(dst, RTLD_NOW | RTLD_GLOBAL);
    if (!dl_handle) {
        printf("dlopen fail: %s\n", dlerror());
        return NULL;
    }

    func = dlsym(dl_handle, name);
    error = dlerror();
    if (error != NULL) {
        printf("dlsym fail: %s\n", error);
        return NULL;
    }

    func_inst_t* tmp = (func_inst_t*)malloc(sizeof(func_inst_t));
    strcpy(tmp->name, name);
    strcpy(tmp->src, src);
    strcpy(tmp->dst, dst);
    tmp->handle = dl_handle;
    tmp->func = func;

    return tmp;
}

void delete_so(func_inst_t* func_inst) {
    dlclose(func_inst->handle);
    remove(func_inst->src);
    remove(func_inst->dst);
    free(func_inst);
    return;
}

int func_reg(const char* str) {
    if (func_inst_nr >= FUNC_LIST_SIZE) {
        printf("too many func.\n");
        return -1;
    }

    char* func_name_ptr = name_buf;
    int status = 0;
    for (int i = 0; i < strlen(str); i++) {
        int flag = 0;
        switch (status) {
            case 0:
                if (str[i] != 'i' && str[i] != 'n' && str[i] != 't') {
                    status = 1;
                }
                break;
            case 1:
                if (str[i] != ' ' && str[i] != '\t') {
                    status = 2;
                    *func_name_ptr = str[i];
                    func_name_ptr++;
                }
                break;
            case 2:
                if (str[i] == '(') {
                    *func_name_ptr = '\0';
                    func_name_ptr++;
                    flag = 1;
                } else {
                    *func_name_ptr = str[i];
                    func_name_ptr++;
                }
                break;
        }
        if (flag != 0) {
            break;
        }
    }

    func_inst_t* func_inst = compile_so(str, name_buf);
    if (func_inst == NULL) {
        return -1;
    }

    printf("reg func %s", func_inst->name);

    func_inst_list[func_inst_nr] = func_inst;
    func_inst_nr += 1;

    // sprintf(func_prefix_buf, "int (*%s)() = (int (*)())0x%lx;\n", name_buf,
    //         (unsigned long)(func_inst->func));
    // strcat(func_prefix_buf_all, func_prefix_buf);

    return 0;
}

static unsigned int expr_cnt = 0;
int eval(const char* str) {
    expr_cnt += 1;
    sprintf(name_buf, "__expr_wrap_%x_fuckhack", expr_cnt);
    sprintf(func_buf,
            "int %s() { \n"
            "return (%s);\n"
            "}\n",
            name_buf, str);

    func_inst_t* func_inst = compile_so(func_buf, name_buf);
    if (func_inst == NULL) {
        return -1;
    }
    printf("%d", ((int (*)())(func_inst->func))());

    delete_so(func_inst);
    return 0;
}

int main(int argc, char* argv[]) {
    // func_prefix_buf_all[0] = '\0';
    printf("This is Crepl for OS Mini Lab 4.\n$ ");
    while (gets_s(read_buf, READ_BUF_SIZE) == 0) {
        if (strncmp(read_buf, "int ", 3) == 0) {
            int iRet = func_reg((const char*)read_buf);
            if (iRet != 0) {
                printf("func reg error");
            }
        } else {
            int iRet = eval((const char*)read_buf);
            if (iRet != 0) {
                printf("eval error");
            }
        }
        printf("\n$ ");
    }
    printf("\ncleanning...\n");
    for (int i = 0; i < func_inst_nr; i++) {
        delete_so(func_inst_list[i]);
    }
    return 0;
}
