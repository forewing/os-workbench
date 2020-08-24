#include <stdio.h>
#include <stdlib.h>
#include <sys/file.h>
#include <sys/types.h>
#include <unistd.h>
#include "kvdb.h"

int main() {
    kvdb_t db;
    const char* key = "operating-systems";
    char* value;

    kvdb_open(&db, "a.db");  // BUG: should check for errors
    kvdb_put(&db, key, "three-easy-pieces");
    value = kvdb_get(&db, key);
    kvdb_close(&db);
    printf("[%s]: [%s]\n", key, value);
    free(value);

    // char fuck1[1024], fuck2[1024];
    // scanf("key:%s val:%s", fuck1, fuck2);
    // printf("%s, %s\n", fuck1, fuck2);

    // FILE* fp = fopen("fuck.txt", "a+");
    // int type;
    // scanf("%d", &type);
    // if (type == 0) {
    //     printf("Add lock 1\n");
    //     flock(fileno(fp), LOCK_SH);
    //     while (1) {
    //         int tmp;
    //         scanf("%d", &tmp);
    //         if (tmp == 1) {
    //             flock(fileno(fp), LOCK_UN);
    //         }
    //     }
    // } else {
    //     printf("AQUARING\n");
    //     flock(fileno(fp), LOCK_SH);
    //     printf("GET\n");
    //     flock(fileno(fp), LOCK_UN);
    //     printf("UNLOCK\n");
    // }
    return 0;
}