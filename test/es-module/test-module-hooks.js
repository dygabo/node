'use strict';

require('../common');
const fixtures = require('../common/fixtures');
const { addHooks } = require('module');
const { spawnSyncAndAssert } = require('../common/child_process');

spawnSyncAndAssert(process.execPath,
  [
    '--require',
    fixtures.path('transpiler-hook.js'),
    fixtures.path('ts-log.ts')
  ], {
    trim: true,
    stdout: 'VirtualPoint { x: 13, y: 56 }'
  }
);

spawnSyncAndAssert(process.execPath,
  [
    '--require',
    fixtures.path('exports-hook.js'),
    fixtures.path('require-dummy.js'),
  ], {
    trim: true,
    stdout: '43'
  }
);
  
// TODO(joyeecheung): require('xx.ts') here?
