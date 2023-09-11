import '../common/index.mjs';
import { opendir } from 'node:fs/promises';
import { execFile } from 'node:child_process';
import { promisify } from 'node:util';
import { fileURLToPath } from 'node:url';
import { describe, it } from 'node:test';

const execFilePromise = promisify(execFile);

const testRunner = fileURLToPath(
  new URL('../../tools/test.py', import.meta.url)
);

const setNames = ['async-hooks', 'parallel'];

const testSets = await Promise.all(setNames.map(async (name) => {
  const path = fileURLToPath(new URL(`../${name}`, import.meta.url));
  const dir = await opendir(path);

  const tests = [];
  for await (const entry of dir) {
    if (entry.name.startsWith('test-async-local-storage-')) {
      tests.push(entry.name);
    }
  }

  return {
    name,
    tests
  };
}));

describe('AsyncContextFrame', () => {
  for (const { name, tests } of testSets) {
    for (const test of tests) {
      const testPath = `${name}/${test}`;
      it(testPath, async () => {
        execFilePromise(testRunner, [
          '--node-args=--async-context-frame',
          testPath,
        ]);
      });
    }
  }
});
