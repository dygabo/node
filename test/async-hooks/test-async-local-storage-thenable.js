// Flags: --expose-internals
'use strict';

const common = require('../common');

const assert = require('assert');
const { AsyncLocalStorage } = require('async_hooks');

// TODO(qard): This is known to fail with ContinuationPreservedEmbedderData
// as thenables are not yet supported in V8. A fix should hopefully land soon.
//
// See: https://chromium-review.googlesource.com/c/v8/v8/+/4674242
const { internalBinding } = require('internal/test/binding');
const { AsyncContextFrame } = internalBinding('async_context_frame');
const hasAsyncContextFrame = typeof AsyncContextFrame === 'function';
if (hasAsyncContextFrame) {
  return;
}

// This test verifies that async local storage works with thenables

const store = new AsyncLocalStorage();
const data = Symbol('verifier');

const then = common.mustCall((cb) => {
  assert.strictEqual(store.getStore(), data);
  setImmediate(cb);
}, 4);

function thenable() {
  return {
    then,
  };
}

// Await a thenable
store.run(data, async () => {
  assert.strictEqual(store.getStore(), data);
  await thenable();
  assert.strictEqual(store.getStore(), data);
});

// Returning a thenable in an async function
store.run(data, async () => {
  try {
    assert.strictEqual(store.getStore(), data);
    return thenable();
  } finally {
    assert.strictEqual(store.getStore(), data);
  }
});

// Resolving a thenable
store.run(data, () => {
  assert.strictEqual(store.getStore(), data);
  Promise.resolve(thenable());
  assert.strictEqual(store.getStore(), data);
});

// Returning a thenable in a then handler
store.run(data, () => {
  assert.strictEqual(store.getStore(), data);
  Promise.resolve().then(() => thenable());
  assert.strictEqual(store.getStore(), data);
});
