#ifndef LOCK_H
#define LOCK_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

int lock(
	int fd, long long start, long long length, bool exclusive,
	bool immediate
);

int unlock(int fd, long long start, long long length);

#ifdef __cplusplus
}
#endif

#endif
