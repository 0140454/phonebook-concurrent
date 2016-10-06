#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include "phonebook_opt.h"
#include "utils.h"
#include "debug.h"

entry *findName(char lastname[], entry *pHead)
{
    size_t len = strlen(lastname);
    while (pHead != NULL) {
        if (strncasecmp(lastname, pHead->lastName, len) == 0
                && (pHead->lastName[len] == '\n' ||
                    pHead->lastName[len] == '\0')) {
            pHead->lastName = (char *) malloc(sizeof(char) *
                                              MAX_LAST_NAME_SIZE);
            memset(pHead->lastName, '\0', MAX_LAST_NAME_SIZE);
            strcpy(pHead->lastName, lastname);
            pHead->dtl = (pdetail) malloc(sizeof(detail));
            return pHead;
        }
        dprintf("find string = %s\n", pHead->lastName);
        pHead = pHead->pNext;
    }
    return NULL;
}

append_argument *new_append_argument(char *pDataStart, char *pDataEnd,
                                     int tid, int nThread, entry *pListHead)
{
    append_argument *app = (append_argument *) malloc(sizeof(append_argument));

    app->pDataStart = pDataStart;
    app->pDataEnd = pDataEnd;
    app->tid = tid;
    app->nThread = nThread;
    app->pListTail = app->pListHead = pListHead;

    return app;
}

void append(void *arg)
{
    struct timespec start, end;
#ifdef DEBUG
    double cpu_time;
#else
    double cpu_time __attribute__((unused));
#endif

    clock_gettime(CLOCK_REALTIME, &start);

    append_argument *app = (append_argument *) arg;

    int count = 0;
    entry *new_node = app->pListHead;
    char *cur_last_name = app->pDataStart;

    while (cur_last_name < app->pDataEnd) {
        app->pListTail = app->pListTail->pNext = new_node;

        app->pListTail->lastName = cur_last_name;
        app->pListTail->pNext = NULL;

        cur_last_name += MAX_LAST_NAME_SIZE * app->nThread;
        new_node += app->nThread;
        ++count;
    }

    clock_gettime(CLOCK_REALTIME, &end);
    cpu_time = diff_in_second(start, end);

    dprintf("thread take %lf sec, count %d\n", cpu_time, count);

    pthread_exit(NULL);
}

void show_entry(entry *pHead)
{
    while (pHead != NULL) {
        printf("lastName = %s\n", pHead->lastName);
        pHead = pHead->pNext;
    }
}
