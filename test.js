const assert = require("assert");
const cluster = require("cluster");
const fs = require("fs/promises");
const { test } = require("node:test");

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

    await test("lock", async () => {
      await assert.doesNotReject(lock(fd, { exclusive: true }));
    });

    await test("unlock", async () => {
      await assert.doesNotReject(unlock(fd));
    });

    await file.close();

    await test("lock closed file", async () => {
      await assert.rejects(lock(fd), { code: "EBADF" });
    });

    await test("unlock closed file", async () => {
      await assert.rejects(unlock(fd), { code: "EBADF" });
    });
  } finally {
    await removeFile(filename);
  }
}

async function primary() {
  await single();

  const file = await fs.open(filename, "wx");
  const { fd } = file;

  await test("exclusive lock", async () => {
    await assert.doesNotReject(lock(fd, { exclusive: true }));
  });
  const worker = cluster.fork();
  // Pretend to do work and unlock the file after 1 second
  setTimeout(async () => {
    await test("exclusive unlock", async () => {
      await assert.doesNotReject(unlock(fd));
    });
    await file.close();
  }, 1000);
  worker.on("disconnect", async () => {
    await removeFile(filename);
  });
}

async function worker() {
  const file = await fs.open(filename, "r+");
  const { fd } = file;
  await test("immediate fail", async () => {
    await assert.rejects(lock(fd, { immediate: true }), {
      code: /^(EACCES|EAGAIN|EBUSY)$/,
    });
  });
  await test("lock wait", async () => {
    await assert.doesNotReject(lock(fd));
  });
  await test("unlock locked", async () => {
    await assert.doesNotReject(unlock(fd));
  });
  await file.close();
  cluster.worker.disconnect();
}

if (cluster.isPrimary) primary();
else worker();
