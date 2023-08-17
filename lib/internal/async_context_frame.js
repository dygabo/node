'use strict';

const {
  GetContinuationPreservedEmbedderData,
  SetContinuationPreservedEmbedderData,
} = internalBinding('async_context_frame');

// Existence of AsyncContextFrame is used to signal the feature is turned on.
if (typeof GetContinuationPreservedEmbedderData !== 'function') {
  return;
}

class AsyncContextFrame extends Map {
  constructor(store, data) {
    super(AsyncContextFrame.current());
    this.set(store, data);
  }

  static current() {
    return GetContinuationPreservedEmbedderData();
  }

  static exchange(frame) {
    const prior = this.current();
    SetContinuationPreservedEmbedderData(frame);
    return prior;
  }

  static disable(store) {
    const frame = this.current();
    frame.disable(store);
  }

  disable(store) {
    this.delete(store);
  }
}

module.exports = { AsyncContextFrame };
