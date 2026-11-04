import fs from 'node:fs';
import path from 'node:path';
import { fileURLToPath } from 'node:url';
import { createHash } from 'node:crypto';
import { createZip } from './package.mjs';

const root = path.resolve(path.dirname(fileURLToPath(import.meta.url)), '..');
const entries = [];
function collect(name) {
  const full = path.join(root, name);
  const stat = fs.lstatSync(full);
  if (stat.isSymbolicLink()) throw new Error(`Unexpected symlink: ${name}`);
  if (stat.isDirectory()) {
    for (const item of fs.readdirSync(full).sort()) {
      if (item === '.git' || item === '__pycache__') continue;
      collect(`${name}/${item}`);
    }
  } else entries.push({ name, data: fs.readFileSync(full), mode: 0o644 });
}
for (const name of ['CMakeLists.txt', 'LICENSE', 'README.md', 'CHANGELOG.md', 'RELEASE_NOTES.md', 'THIRD_PARTY_NOTICES.md',
  '.gitignore', '.gitattributes', 'src', 'assets', 'images', 'cmake', 'scripts', '.github',
  'third_party/json', 'third_party/toolchain-licenses', 'third_party/openvr/src',
  'third_party/openvr/headers', 'third_party/openvr/LICENSE']) collect(name);
const archive = 'frame-passthrough-shortcuts-0.1.1-source.zip';
const bytes = createZip(entries);
fs.mkdirSync(path.join(root, 'dist'), { recursive: true });
fs.writeFileSync(path.join(root, 'dist', archive), bytes);
fs.writeFileSync(path.join(root, 'dist', `${archive}.sha256`), `${createHash('sha256').update(bytes).digest('hex')}  ${archive}\n`);
console.log(`${entries.length} source files: dist/${archive}`);
