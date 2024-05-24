'use strict';

const { addHooks } = require('node:module');

function resolve(specifier, context, nextResolve) {
  // needed to build the context on the module during resolution.
  // should be revisited
  return nextResolve(specifier, context);
}

function export_hook(specifier, exports_obj, context, nextExports) {
  if (specifier === './dummy.js') {
    // context contains:
    const  { filename, format, isMain, parent, specifier } = context;
    const orig = exports_obj;
    exports_obj = () => orig() + 1;
  }
  return nextExports(specifier, exports_obj, context);
}

addHooks({
  resolve,
  export_hook,
});
