#define  _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <assert.h>

#include <unistd.h>
#include <pthread.h>
#include <sys/mman.h>

#include "utils.h"
#include IMPL

#ifdef OPT
#if defined(THREADPOOL_MUTEX)
#include "threadpool.h"
#endif
#include "file.c"
#include "debug.h"
#include <fcntl.h>
#define ALIGN_FILE "align.txt"
#endif

#define DICT_FILE "./dictionary/words.txt"

int main(int argc, char *argv[])
{
#ifndef OPT
    FILE *fp;
    int i = 0;
    char line[MAX_LAST_NAME_SIZE];
#endif
    struct timespec start, end;
    double cpu_time1, cpu_time2;

#ifndef OPT
    /* check file opening */
    fp = fopen(DICT_FILE, "r");
    if (!fp) {
        printf("cannot open the file\n");
        return -1;
    }
#else
    file_align(DICT_FILE, ALIGN_FILE, MAX_LAST_NAME_SIZE);
    int fd = open(ALIGN_FILE, O_RDONLY | O_NONBLOCK);
    off_t fs = fsize(ALIGN_FILE);
#endif

    /* build the entry */
    entry *pHead, *e;
    pHead = (entry *) malloc(sizeof(entry));
    printf("size of entry : %lu bytes\n", sizeof(entry));
    e = pHead;
    e->pNext = NULL;

#if defined(__GNUC__)
    __builtin___clear_cache((char *) pHead, (char *) pHead + sizeof(entry));
#endif

#if defined(OPT)

#ifndef THREAD_NUM
#define THREAD_NUM 4
#endif
    clock_gettime(CLOCK_REALTIME, &start);

    char *map = mmap(NULL, fs, PROT_READ, MAP_SHARED, fd, 0);
    assert(map && "mmap error");

    /* allocate at beginning */
    entry *entry_pool = (entry *) malloc(sizeof(entry) *
                                         fs / MAX_LAST_NAME_SIZE);

    assert(entry_pool && "entry_pool error");

    pthread_setconcurrency(THREAD_NUM + 1);

    append_argument **app = (append_argument **) malloc(sizeof(append_argument *) * THREAD_NUM);
#if defined(THREADPOOL_MUTEX)
    threadpool_t *pool = threadpool_create(THREAD_NUM, 10, 0);
    for (int i = 0; i < THREAD_NUM; i++) {
        app[i] = new_append_argument(map + MAX_LAST_NAME_SIZE * i, map + fs, i,
                                     THREAD_NUM, entry_pool + i);

        threadpool_add(pool, append, app[i], 0);
    }
    threadpool_destroy(pool, 1);
#else
    pthread_t *tid = (pthread_t *) malloc(sizeof(pthread_t) * THREAD_NUM);
    for (int i = 0; i < THREAD_NUM; i++)
        app[i] = new_append_argument(map + MAX_LAST_NAME_SIZE * i, map + fs, i,
                              THREAD_NUM, entry_pool + i);

    for (int i = 0; i < THREAD_NUM; i++)
        pthread_create( &tid[i], NULL, (void *) &append, (void *) app[i]);

    for (int i = 0; i < THREAD_NUM; i++)
        pthread_join(tid[i], NULL);
#endif

    pHead = app[0]->pListHead;
    e = app[0]->pListTail;
    for (int i = 1; i < THREAD_NUM; i++) {
        e->pNext = app[i]->pListHead;
        e = app[i]->pListTail;
    }

    clock_gettime(CLOCK_REALTIME, &end);
    cpu_time1 = diff_in_second(start, end);
#else /* ! OPT */
    clock_gettime(CLOCK_REALTIME, &start);
    while (fgets(line, sizeof(line), fp)) {
        while (line[i] != '\0')
            i++;
        line[i - 1] = '\0';
        i = 0;
        e = append(line, e);
    }

    clock_gettime(CLOCK_REALTIME, &end);
    cpu_time1 = diff_in_second(start, end);
#endif

#ifndef OPT
    /* close file as soon as possible */
    fclose(fp);
#endif

    e = pHead;

    /* the givn last name to find */
    char input[MAX_LAST_NAME_SIZE] = "zyxel";
    e = pHead;

    assert(findName(input, e) &&
           "Did you implement findName() in " IMPL "?");
    assert(0 == strcmp(findName(input, e)->lastName, "zyxel"));

#if defined(__GNUC__)
    __builtin___clear_cache((char *) pHead, (char *) pHead + sizeof(entry));
#endif
    /* compute the execution time */
    clock_gettime(CLOCK_REALTIME, &start);
    findName(input, e);
    clock_gettime(CLOCK_REALTIME, &end);
    cpu_time2 = diff_in_second(start, end);

    FILE *output;
#if defined(THREADPOOL_MUTEX)
    output = fopen("opt_mutex.txt", "a");
#elif defined(OPT)
    output = fopen("opt.txt", "a");
#else
    output = fopen("orig.txt", "a");
#endif
    fprintf(output, "append() findName() %lf %lf\n", cpu_time1, cpu_time2);
    fclose(output);

    printf("execution time of append() : %lf sec\n", cpu_time1);
    printf("execution time of findName() : %lf sec\n", cpu_time2);

#ifndef OPT
    if (pHead->pNext) free(pHead->pNext);
    free(pHead);
#else
    free(entry_pool);
#if !defined(THREADPOOL_MUTEX)
    free(tid);
#endif
    free(app);
    munmap(map, fs);
#endif
    return 0;
}
