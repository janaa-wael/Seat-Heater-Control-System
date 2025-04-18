#ifndef MUTEX_STATS_H
#define MUTEX_STATS_H

// Forward declaration to avoid including semphr.h
typedef void * xSemaphoreHandle;

typedef struct {
    xSemaphoreHandle mutex;
    const uint8_t *name;
    uint32_t lockStartTime;
    uint32_t totalLockTime;
} MutexStats;

extern MutexStats mutexStats[11];
int getMutexIndex(void *pxQueue);

#endif
