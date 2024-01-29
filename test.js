const assert = require("assert");
const cluster = require("cluster");
const fs = require("fs/promises");

const { lock, unlock } = require(".");

async function removeFile(filename) {
  try {
    await fs.unlink(filename);
  } catch (e) {
    if (e.code != "ENOENT") throw e;
  }
}

const filename = "file";

async function single() {
  try {
    const file = await fs.open(filename, "wx");
    const { fd } = file;

    // Basic test
    await assert.doesNotReject(lock(fd, { exclusive: true }));
    await assert.doesNotReject(unlock(fd));

    await file.close();
    // Cannot lock closed file
    await assert.rejects(lock(fd), { code: "EBADF" });
    // Cannot unlock closed file
    await assert.rejects(unlock(fd), { code: "EBADF" });
  } finally {
    await removeFile(filename);
  }

  console.log("Single process tests passed");
}

async function master() {
  await single();

  const file = await fs.open(filename, "wx");
  const { fd } = file;

  // Get an exclusive lock
  assert.doesNotReject(lock(fd, { exclusive: true }));
  const worker = cluster.fork();
  // Pretend to do work and unlock the file after 1 second
  setTimeout(async () => {
    await assert.doesNotReject(unlock(fd));
    await file.close();
  }, 1000);
  worker.on("disconnect", async () => {
    await removeFile(filename);
    console.log("Multi process tests passed");
  });
}

async function worker() {
  const file = await fs.open(filename, "r+");
  const { fd } = file;
  await assert.rejects(lock(fd, { immediate: true }), {
    code: /^(EACCES|EAGAIN|EBUSY)$/,
  });
  await assert.doesNotReject(lock(fd));
  await assert.doesNotReject(unlock(fd));
  await file.close();
  cluster.worker.disconnect();
}

process.on("unhandledRejection", (err) => {
  throw err;
});

if (cluster.isMaster) master();
else worker();
