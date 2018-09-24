#include <stdbool.h>

#ifdef _WIN32
#include <windows.h>
#include <io.h>
#else
#include <errno.h>
#include <fcntl.h>
#endif

#include "lock.h"

int lock(
	int fd, long long start, long long length, bool exclusive,
	bool immediate
)
{
#ifdef _WIN32
	HANDLE hFile = (HANDLE) _get_osfhandle(fd);
	if (hFile == INVALID_HANDLE_VALUE)
		return ERROR_INVALID_HANDLE;
	DWORD dwFlags = (
		(exclusive ? LOCKFILE_EXCLUSIVE_LOCK : 0) |
		(immediate ? LOCKFILE_FAIL_IMMEDIATELY : 0)
	);
	DWORD dwReserved = 0;
	if (!length)
		length = ULLONG_MAX;
	DWORD nNumberOfBytesToLockLow = (DWORD) length;
	DWORD nNumberOfBytesToLockHigh = (DWORD) (length >> 32);
	OVERLAPPED sOverlapped = { 0 };
	sOverlapped.Offset = (DWORD) start;
	sOverlapped.OffsetHigh = (DWORD) (start >> 32);
	LPOVERLAPPED lpOverlapped = &sOverlapped;
	BOOL fSuccess = LockFileEx(
		hFile, dwFlags, dwReserved, nNumberOfBytesToLockLow,
		nNumberOfBytesToLockHigh, lpOverlapped
	);
	return fSuccess ? 0 : GetLastError();
#else
	struct flock flock = {
		.l_start=start,
		.l_len=length,
		.l_pid=0,
		.l_type=(short)(exclusive ? F_WRLCK : F_RDLCK),
		.l_whence=SEEK_SET
	};
	int res = fcntl(fd, immediate ? F_SETLK : F_SETLKW, &flock);
	return res == -1 ? errno : 0;
#endif
}

int unlock(int fd, long long start, long long length)
{
#ifdef _WIN32
	HANDLE hFile = (HANDLE) _get_osfhandle(fd);
	if (hFile == INVALID_HANDLE_VALUE)
		return ERROR_INVALID_HANDLE;
	DWORD dwReserved = 0;
	if (!length)
		length = ULLONG_MAX;
	DWORD nNumberOfBytesToLockLow = (DWORD) length;
	DWORD nNumberOfBytesToLockHigh = (DWORD) (length >> 32);
	OVERLAPPED sOverlapped = { 0 };
	sOverlapped.Offset = (DWORD) start;
	sOverlapped.OffsetHigh = (DWORD) (start >> 32);
	LPOVERLAPPED lpOverlapped = &sOverlapped;
	BOOL fSuccess = UnlockFileEx(
		hFile, dwReserved, nNumberOfBytesToLockLow,
		nNumberOfBytesToLockHigh, lpOverlapped
	);
	return fSuccess ? 0 : GetLastError();
#else
	struct flock flock = {
		.l_start=start,
		.l_len=length,
		.l_pid=0,
		.l_type=F_UNLCK,
		.l_whence=SEEK_SET
	};
	int res = fcntl(fd, F_SETLK, &flock);
	return res == -1 ? errno : 0;
#endif
}
