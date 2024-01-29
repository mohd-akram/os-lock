# os-lock

Cross-platform file locking. Uses `fcntl` on UNIX and
`LockFileEx`/`UnlockFileEx` on Windows.

## Install

    npm install os-lock

## Usage

### Synopsis

```javascript
await lock(fd, options);
await lock(
  fd,
  (start = 0),
  (length = 0),
  (options = { exclusive: false, immediate: false })
);
await unlock(fd, (start = 0), (length = 0));
```

### Arguments

- `fd` - File descriptor.
- `start` - Starting offset for lock.
- `length` - Number of bytes to lock. If 0, lock till end of file.
- `exclusive` - Acquire an exclusive lock (i.e. write lock).
- `immediate` - Throw if a conflicting lock is present instead of waiting.

An error thrown due to `immediate` will have its `code` property set to
`EACCES`, `EAGAIN` or `EBUSY`.

### Example

```javascript
const cluster = require("cluster");
const fs = require("fs");
const util = require("util");

const { lock, unlock } = require("os-lock");

const fsOpen = util.promisify(fs.open);
const fsClose = util.promisify(fs.close);
const fsUnlink = util.promisify(fs.unlink);

const filename = "file";

async function master() {
  const fd = await fsOpen(filename, "wx");
  // Get an exclusive lock
  await lock(fd, { exclusive: true });
  // Fork a new process which will attempt to get a lock for itself
  // (see the worker() function below)
  const worker = cluster.fork();
  // Pretend to do work and unlock the file after 3 seconds
  setTimeout(async () => {
    await unlock(fd);
    await fsClose(fd);
  }, 3000);
  // Delete the file after the worker is done with it
  worker.on("disconnect", async () => await fsUnlink(filename));
}

async function worker() {
  const fd = await fsOpen(filename, "r+");
  console.time("got lock after");
  try {
    // Try (and fail) to get a lock immediately
    await lock(fd, { immediate: true });
  } catch (e) {
    if (!/^(EACCES|EAGAIN|EBUSY)$/.test(e.code)) throw e;
    console.warn("failed to get lock immediately");
  }
  // Try again, this time with patience
  await lock(fd);
  console.timeEnd("got lock after");
  await unlock(fd);
  cluster.worker.disconnect();
}

if (cluster.isMaster) master();
else worker();
```
