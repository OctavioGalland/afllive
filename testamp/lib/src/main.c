#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

struct array {
    void **elems;
    size_t items;
    size_t size;
};

static void array_init(struct array *arr) {
    arr->elems = malloc(sizeof(void*));
    arr->items = 0;
    arr->size = 1;
}

static void* array_get(struct array *arr, size_t i) {
    if (i > arr->items || !arr->elems)
        return NULL;
    else
        return arr->elems[i];
}

static void array_add(struct array *arr, void *elem) {
    if (arr->elems) {
        arr->items++;
        if (arr->items > arr->size) {
            arr->size *= 2;
            arr->elems = reallocarray(arr->elems, arr->size, sizeof(void*));
        }
        arr->elems[arr->items - 1] = elem;
    }
}

static void array_add_without_repetition(struct array *arr, void *elem, int (*cmp)(void*, void*)) {
    if (arr->elems) {
        int exists = 0;

        for (size_t i = 0; i < arr->items; i++) {
            if (!cmp(array_get(arr, i), elem)) {
                exists = 1;
                break;
            }
        }

        if (!exists) {
            array_add(arr, elem);
        }
    }
}

static void array_free(struct array *arr) {
    if (arr->elems) {
        free(arr->elems);
        arr->elems = NULL;
        arr->items = arr->size = 0;
    }
}

static void addToLog(const char *logName, struct array *arr) {
    size_t fileNameLen = strlen(logName) + strlen("/opt/.log");
    char *fileName = malloc(fileNameLen + 1);
    memset(fileName, 0, fileNameLen + 1);
    snprintf(fileName, fileNameLen + 1, "/opt/%s.log", logName);
    FILE *log = fopen(fileName, "a");
    for (size_t i = 0; i < arr->items; i++) {
        char *sig = array_get(arr, i);
        fwrite(sig, 1, strlen(sig), log);
        fwrite("\n", 1, 1, log);
    }
    fclose(log);
    free(fileName);
}

static struct array lines;
static int recording = 1;

extern char **environ;

static char *nullSeparatedWordsToQuotedWords (char *str, size_t len) {
    char *res = malloc(1);
    size_t resSize = 1;
    res[0] = 0;

    size_t amountOfArgs = 0;
    for (size_t i = 0; i < len; i++) {
        if (str[i] == 0) {
            amountOfArgs++;
        }
    }

    char *nextArg = str;
    for (size_t i = 0; i < amountOfArgs; i++) {
        // we want to wrap every arg between quotes and add a space after all args but the last one
        size_t oldSize = resSize;
        size_t argSize = strlen(nextArg);
        resSize += argSize + 2 + (i != (amountOfArgs - 1));
        res = realloc(res, resSize);
        memset(res + oldSize, 0, resSize - oldSize);
        strcat(res, "'");
        strcat(res, nextArg);
        strcat(res, "'");
        if (i != amountOfArgs - 1) {
            strcat(res, " ");
        }
        nextArg += argSize + 1;
    }

    return res;
}

#ifdef __cplusplus
extern "C"{
#endif

void __ah_init () {
    array_init(&lines);
    array_add(&lines, "FUNCTIONS:");
    recording = 1;
}

void __ah_enter_function (const char *sig) {
    if (recording == 0)
        return;

    array_add_without_repetition(&lines, (void*)sig, (int(*)(void*, void*))strcmp);
}

void __ah_deinit () {
    if (recording) {
        if (lines.items > 1) {
            array_add(&lines, (void*)"ENVIRONMENT:");
            for (char ** s = environ; *s; s++) {
                array_add(&lines, *s);
            }
            FILE *cmdline = fopen("/proc/self/cmdline", "r");
            if (cmdline != NULL) {
                char *cmd = NULL;
                size_t cmdSize = 4096;
                cmd = malloc(cmdSize + 1);
                memset(cmd, 0, cmdSize + 1);
                size_t bytesRead = fread(cmd, 1, cmdSize, cmdline);
                fclose(cmdline);
                char *quotedCmd = nullSeparatedWordsToQuotedWords(cmd, bytesRead);

                array_add(&lines, (void*)"CMDLINE:");
                array_add(&lines, quotedCmd);
                char *cwd =  get_current_dir_name();
                array_add(&lines, (void*)"CWD:");
                array_add(&lines, (void*)cwd);
                array_add(&lines, "\n\n\n");


                int lockFile = -1;
                do {
                    lockFile = open("/opt/autoharness.lck", O_EXCL | O_CREAT);
                    if (lockFile < 0)
                        usleep(50000);
                } while (lockFile < 0);
                close(lockFile);
                addToLog("fuzz", &lines);
                unlink("/opt/autoharness.lck");

                free(quotedCmd);
                free(cmd);
                free(cwd);
            }
        }

        array_free(&lines);
        recording = 0;
    }
}

#ifdef __cplusplus
}
#endif
