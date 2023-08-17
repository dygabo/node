'use strict';

const {
  FunctionPrototypeBind,
  ReflectApply,
} = primordials;

const { validateFunction } = require('internal/validators');
const { AsyncContextFrame } = require('internal/async_context_frame');

class AsyncLocalStorage {
  static bind(fn) {
    validateFunction(fn, 'fn');
    const run = this.snapshot();
    return function bound(...args) {
      return run.call(this, fn, ...args);
    };
  }

  static snapshot() {
    const frame = AsyncContextFrame.current();
    return function runSnapshot(fn, ...args) {
      const bound = FunctionPrototypeBind(fn, this);
      const prior = AsyncContextFrame.exchange(frame);
      try {
        return ReflectApply(bound, undefined, args);
      } finally {
        AsyncContextFrame.exchange(prior);
      }
    };
  }

  disable() {
    AsyncContextFrame.disable(this);
  }

  enterWith(data) {
    const frame = new AsyncContextFrame(this, data);
    AsyncContextFrame.exchange(frame);
  }

  run(data, fn, ...args) {
    const prior = AsyncContextFrame.current();
    const frame = new AsyncContextFrame(this, data);
    AsyncContextFrame.exchange(frame);
    try {
      return ReflectApply(fn, undefined, args);
    } finally {
      AsyncContextFrame.exchange(prior);
    }
  }

  exit(fn, ...args) {
    return ReflectApply(this.run, this, [undefined, fn, ...args]);
  }

  getStore() {
    return AsyncContextFrame.current()?.get(this);
  }
}

module.exports = AsyncLocalStorage;
