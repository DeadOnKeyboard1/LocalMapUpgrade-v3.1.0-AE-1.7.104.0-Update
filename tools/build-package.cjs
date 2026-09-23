const fs = require('node:fs');
const path = require('node:path');
const assert = require('node:assert/strict');

const root = path.resolve(__dirname, '..');
const output = path.join(root, 'package');
const assets = path.join(root, 'assets');

if (!fs.existsSync(assets)) throw new Error(`Missing assets directory: ${assets}`);
fs.rmSync(output, { recursive: true, force: true });
fs.cpSync(assets, output, { recursive: true });

const u8 = n => Buffer.from([n]);
const u16 = n => { const b = Buffer.alloc(2); b.writeUInt16LE(n); return b; };
const u32 = n => { const b = Buffer.alloc(4); b.writeUInt32LE(n); return b; };
const f32 = n => { const b = Buffer.alloc(4); b.writeFloatLE(n); return b; };
const cat = (...b) => Buffer.concat(b);
const zstr = s => Buffer.from(s + '\0');
const str = s => cat(u16(Buffer.byteLength(s)), Buffer.from(s));
const sub = (tag, data) => cat(Buffer.from(tag), u16(data.length), data);
function record(tag, data, id = 0, flags = 0) {
  return cat(Buffer.from(tag), u32(data.length), u32(flags), u32(id), u32(0), u16(44), u16(0), data);
}
function write(relative, content) {
  const dest = path.join(output, relative);
  fs.mkdirSync(path.dirname(dest), { recursive: true });
  fs.writeFileSync(dest, content);
}

// QUST and VMAD layouts follow xEdit Core/wbDefinitionsTES5.pas.
// The quest is owned by this plugin (master index 1), not a Skyrim.esm override.
const questID = 0x01000800;
const prop = cat(str('ModName'), u8(2), u8(1), str('LocalMapUpgrade'));
const script = cat(str('LMU_MCM'), u8(0), u16(1), prop);
const aliasScript = cat(str('SKI_PlayerLoadGameAlias'), u8(0), u16(0));
const vmad = cat(u16(5), u16(2), u16(1), script,
  u8(2), u16(0), str(''),
  u16(1), u16(0), u16(0), u32(questID),
  u16(5), u16(2), u16(1), aliasScript);
const quest = record('QUST', cat(
  sub('EDID', zstr('LMU_MCM_Quest')), sub('VMAD', vmad),
  sub('FULL', zstr('Local Map Upgrade MCM')),
  sub('DNAM', cat(u16(0x111), u8(0), u8(0xFF), u32(0), u32(0))),
  sub('NEXT', Buffer.alloc(0)), sub('ANAM', u32(1)),
  sub('ALST', u32(0)), sub('ALID', zstr('PlayerAlias')), sub('FNAM', u32(0)),
  sub('ALFR', u32(0x14)), sub('VTCK', u32(0)), sub('ALED', Buffer.alloc(0))
), questID);
const header = record('TES4', cat(
  sub('HEDR', cat(f32(1.7), u32(1), u32(0x801))),
  sub('CNAM', zstr('Local Map Upgrade - local MCM integration')),
  sub('SNAM', zstr('MCM registration only; no gameplay record overrides.')),
  sub('MAST', zstr('Skyrim.esm')), sub('DATA', Buffer.alloc(8))
), 0, 0x200);
const group = cat(Buffer.from('GRUP'), u32(24 + quest.length), Buffer.from('QUST'), u32(0), u32(0), u16(0), u16(0), quest);
write('LocalMapUpgrade.esp', cat(header, group));
write('SEQ/LocalMapUpgrade.seq', u32(questID));

function findBuildFile(filename, required) {
  const dirs = [
    process.env.LMU_BUILD_DIR,
    path.join(root, 'build', 'relwithdebinfo-se-only'),
    path.join(root, 'build', 'Release'),
    path.join(root, 'build', 'relwithdebinfo')
  ].filter(Boolean);
  for (const dir of dirs) {
    const candidate = path.join(dir, filename);
    if (fs.existsSync(candidate)) return candidate;
  }
  if (required) {
    throw new Error(`Could not find ${filename}. Build the 1.7.104.0 plugin first or set LMU_BUILD_DIR.`);
  }
  return null;
}

const dll = findBuildFile('LocalMapUpgrade.dll', true);
write('SKSE/Plugins/LocalMapUpgrade.dll', fs.readFileSync(dll));
const pdb = findBuildFile('LocalMapUpgrade.pdb', false);
if (pdb) write('SKSE/Plugins/LocalMapUpgrade.pdb', fs.readFileSync(pdb));

write('MCM/Config/LocalMapUpgrade/settings.ini',
  fs.readFileSync(path.join(assets, 'SKSE/Plugins/LocalMapUpgrade.ini')));

const docsDir = path.join(output, 'Docs');
fs.mkdirSync(docsDir, { recursive: true });
for (const name of ['LICENSE', 'NOTICE.md', 'THIRD_PARTY_NOTICES.md']) {
  fs.copyFileSync(path.join(root, name), path.join(docsDir, name));
}
fs.cpSync(path.join(root, 'licenses'), path.join(docsDir, 'licenses'), { recursive: true });

// Generate localization only after the clean package tree exists.
require('./build-localization.cjs');

const config = JSON.parse(fs.readFileSync(path.join(output, 'MCM/Config/LocalMapUpgrade/config.json')));
const controls = config.pages.flatMap(p => p.content).filter(c => c.id);
const ini = fs.readFileSync(path.join(output, 'MCM/Config/LocalMapUpgrade/settings.ini'), 'utf8');
const settings = new Map();
let section;
for (const line of ini.split(/\r?\n/)) {
  if (/^\[/.test(line)) section = line.slice(1, -1);
  else if (/^\w+=/.test(line)) {
    const [key, value] = line.split('=');
    settings.set(`${key}:${section}`, Number(value));
  }
}
assert.equal(controls.length, 11);
assert.equal(new Set(controls.map(c => c.id)).size, settings.size);
for (const control of controls) {
  assert.ok(settings.has(control.id), control.id);
  assert.equal(Number(control.valueOptions.defaultValue), settings.get(control.id));
  assert.ok(fs.readFileSync(path.join(root, 'source/Settings.cpp'), 'utf8').includes('"' + control.id.split(':')[0] + '"'));
}

// Independent readback checks for record bounds, ownership, and the quest script.
const esp = fs.readFileSync(path.join(output, 'LocalMapUpgrade.esp'));
const groupPos = 24 + esp.readUInt32LE(4);
assert.equal(groupPos + esp.readUInt32LE(groupPos + 4), esp.length);
const questPos = groupPos + 24;
assert.equal(esp.readUInt32LE(questPos + 12), questID);
let pos = questPos + 24;
const fields = new Map();
while (pos < esp.length) {
  const name = esp.toString('ascii', pos, pos + 4);
  const len = esp.readUInt16LE(pos + 4);
  fields.set(name, esp.subarray(pos + 6, pos + 6 + len));
  pos += 6 + len;
}
assert.equal(pos, esp.length);
assert.equal(fields.get('ALFR').readUInt32LE(), 0x14);
assert.ok(fields.get('DNAM').readUInt16LE() & 1);
const v = fields.get('VMAD');
let cursor = 6;
function readString() {
  const n = v.readUInt16LE(cursor); cursor += 2;
  const s = v.toString('utf8', cursor, cursor + n); cursor += n;
  assert.ok(!s.includes('\0'));
  return s;
}
assert.equal(readString(), 'LMU_MCM');
cursor += 3;
assert.equal(readString(), 'ModName');
cursor += 2;
assert.equal(readString(), 'LocalMapUpgrade');
assert.equal(v[cursor++], 2);
cursor += 2;
assert.equal(readString(), '');
assert.equal(v.readUInt16LE(cursor), 1); cursor += 2;
assert.equal(v.readUInt32LE(cursor + 4), questID); cursor += 8;
assert.equal(v.readUInt16LE(cursor), 5); cursor += 4;
assert.equal(v.readUInt16LE(cursor), 1); cursor += 2;
assert.equal(readString(), 'SKI_PlayerLoadGameAlias'); cursor += 3;
assert.equal(cursor, v.length);
assert.equal(fs.readFileSync(path.join(output, 'Scripts/LMU_MCM.pex')).readUInt32BE(), 0xFA57C0DE);

console.log(`PASS: package built from ${path.dirname(dll)}; 11 settings/defaults and ESL quest/VMAD/player alias/PEX/SEQ validated.`);
