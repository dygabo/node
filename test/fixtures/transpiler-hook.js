'use strict';

const ts = require('./snapshot/typescript');
const module = require('node:module');

const extensions = {
  '.cts': 'typescript-commonjs',
  '.mts': 'typescript-esm',
  '.ts': 'typescript',
};

const output = {
  'typescript-commonjs': {
    options: { module: ts.ModuleKind.CommonJS },
    format: 'commonjs',
  },
  'typescript-esm': {
    options: { module: ts.ModuleKind.ESNext },
    format: 'module',
  },
  'typescript': {
    options: { module: ts.ModuleKind.NodeNext },
    format: 'commonjs',
  },
};

function resolve(specifier, context, nextResolve) {
  const resolved = nextResolve(specifier, context);
  const supportedExt = extensions.find((ext) => resolved.url.endsWith(ext));
  if (!supportedExt) {
    return resolved;
  }
  return {
    ...resolved,
    format: extensions[supportedExt],
  };
}

function load(url, context, nextLoad) {
  const { source: rawSource, format } = nextLoad(url, context);
  if (!format.startsWith('typescript')) {
    return loadResult;
  }

  const transpiled = ts.transpileModule(source, {
    compilerOptions: output[format].options
  });

  return {
    format: output[format].format,
    shortCircuit: true,
    source: transpiled,
  };
}

module.register({
  resolve,
  load,
});
