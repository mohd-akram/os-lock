const util = require('util');

const addon = require('./build/Release/addon');

const _lock = util.promisify(addon.lock);
const _unlock = util.promisify(addon.unlock);

function lock(
  fd, startOrOptions, length = 0,
  options = { exclusive: false, immediate: false }
) {
  let start = 0;
  if (typeof startOrOptions == 'number')
    start = startOrOptions;
  else if (startOrOptions)
    options = startOrOptions;
  const { exclusive, immediate } = options;
  return _lock(fd, start, length, exclusive, immediate);
}

function unlock(fd, start = 0, length = 0) {
  return _unlock(fd, start, length);
}

module.exports.lock = lock;
module.exports.unlock = unlock;
