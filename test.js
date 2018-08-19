const assert = require('assert');
const cluster = require('cluster');
const fs = require('fs');
const util = require('util');

const fsOpen = util.promisify(fs.open);
const fsClose = util.promisify(fs.close);
const fsUnlink = util.promisify(fs.unlink);

const { lock, unlock } = require('.');

async function removeFile(filename) {
  try {
    await fsUnlink(filename);
  } catch (e) {
    if (e.code != 'ENOENT')
      throw e;
  }
}

const filename = 'file';

async function single() {
  try {
    const fd = await fsOpen(filename, 'wx');

    // Basic test
    await assert.doesNotReject(lock(fd, { exclusive: true }));
    await assert.doesNotReject(unlock(fd));

    await fsClose(fd);
    // Cannot lock closed file
    await assert.rejects(lock(fd), { code: 'EBADF' });
    // Cannot unlock closed file
    await assert.rejects(unlock(fd), { code: 'EBADF' });
  } finally {
    await removeFile(filename);
  }

  console.log('Single process tests passed');
}

async function master() {
  await single();

  const fd = await fsOpen(filename, 'wx');

  // Get an exclusive lock
  assert.doesNotReject(lock(fd, { exclusive: true }));
  const worker = cluster.fork();
  // Pretend to do work and unlock the file after 1 second
  setTimeout(async () => {
    await assert.doesNotReject(unlock(fd));
    await fsClose(fd);
  }, 1000);
  worker.on('disconnect', async () => {
    await removeFile(filename);
    console.log('Multi process tests passed');
  });
}

async function worker() {
  const fd = await fsOpen(filename, 'r+');
  await assert.rejects(
    lock(fd, { immediate: true }), { code: /^(EACCES|EAGAIN|EBUSY)$/ }
  );
  await assert.doesNotReject(lock(fd));
  await assert.doesNotReject(unlock(fd));
  await fsClose(fd);
  cluster.worker.disconnect();
}

if (cluster.isMaster)
  master();
else
  worker();
