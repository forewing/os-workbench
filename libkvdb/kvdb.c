#include "kvdb.h"
#include <pthread.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/file.h>
#include <sys/types.h>
#include <unistd.h>

#define DB_KEY_MAX_LENGTH 256
#define DB_VAL_MAX_LENGTH (16 * 1024 * 1024 + 1024)

int kvdb_open(kvdb_t* db, const char* filename) {
    db->valid = 1;

    db->filename = filename;

    if (pthread_mutex_init(&db->mutex, NULL) != 0) {
        perror("mutex");
        return -1;
    }

    db->fp = fopen(filename, "a+");
    if (db->fp == NULL) {
        perror("fopen");
        return -1;
    }

    db->fd = fileno(db->fp);
    if (db->fd == -1) {
        perror("fineno");
        return -1;
    }

    return 0;
}

int kvdb_close(kvdb_t* db) {
    db->valid = 0;
    db->fd = -1;
    db->filename = NULL;
    pthread_mutex_destroy(&db->mutex);
    if (fclose(db->fp) != 0) {
        perror("fclose");
        return -1;
    }
    db->fp = NULL;
    return 0;
}

int _kvdb_put(kvdb_t* db, const char* key, const char* value) {
    if (key[0] == ' ' || key[0] == '\n' || key[0] == '\0' || value[0] == ' ' ||
        value[0] == '\n' || value[0] == '\0') {
        return -1;
    }
    if (strlen(key) > DB_KEY_MAX_LENGTH - 1) {
        return -1;
    }
    if (strlen(value) > DB_VAL_MAX_LENGTH - 1) {
        return -1;
    }

    fseek(db->fp, 0, SEEK_END);
    if (fprintf(db->fp, "\nkey:%s val:%s end\n", key, value) < 0) {
        perror("fprintf");
        return -1;
    }
    fflush(db->fp);
    fsync(db->fd);
    return 0;
}

char* _kvdb_get(kvdb_t* db, const char* key) {
    long size = lseek(db->fd, 0, SEEK_END);
    fseek(db->fp, 0, SEEK_SET);
    char* buffer = (char*)malloc(size + 10);
    fread(buffer, size, 1, db->fp);
    buffer[size] = '\n';
    buffer[size + 1] = '\0';

    int flag = 0;
    char* ret = (char*)malloc(DB_VAL_MAX_LENGTH);

    char* ptr = strtok(buffer, "\n");
    while (ptr != NULL) {
        int len = 0;
        int pos_key = -1;
        int pos_val = -1;
        int pos_end = -1;
        while (ptr[len] != '\n' && ptr + len < buffer + size) {
            len++;
        }
        for (int i = 0; i < len - 4; i++) {
            if (strncmp(ptr + i, "key:", 4) == 0) {
                pos_key = i + 4;
                break;
            }
        }
        for (int i = 0; i < len - 5; i++) {
            if (strncmp(ptr + i, " val:", 5) == 0) {
                pos_val = i + 5;
                break;
            }
        }
        for (int i = 0; i < len - 4; i++) {
            if (strncmp(ptr + i, " end", 4) == 0) {
                pos_end = i;
                break;
            }
        }

        if (pos_key >= 0 && pos_val >= 0 && pos_end >= 0) {
            int len_key = strlen(key);
            if (len_key == pos_val - pos_key - 5 &&
                strncmp(ptr + pos_key, key, len_key) == 0) {
                flag = 1;
                memcpy(ret, ptr + pos_val, pos_end - pos_val);
                ret[pos_end - pos_val] = '\0';
            }
        }
        ptr = strtok(NULL, "\n");
    }

    free(buffer);

    if (flag == 0) {
        return NULL;
    }
    return ret;
}

int kvdb_put(kvdb_t* db, const char* key, const char* value) {
    pthread_mutex_lock(&db->mutex);
    flock(db->fd, LOCK_EX);
    int ret = _kvdb_put(db, key, value);
    flock(db->fd, LOCK_UN);
    pthread_mutex_unlock(&db->mutex);
    return ret;
}

char* kvdb_get(kvdb_t* db, const char* key) {
    pthread_mutex_lock(&db->mutex);
    flock(db->fd, LOCK_SH);
    char* ret = _kvdb_get(db, key);
    flock(db->fd, LOCK_UN);
    pthread_mutex_unlock(&db->mutex);
    return ret;
}