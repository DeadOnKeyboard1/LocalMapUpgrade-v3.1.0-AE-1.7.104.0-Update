const fs = require('node:fs');
const path = require('node:path');
const assert = require('node:assert/strict');
const root = path.resolve(__dirname, '..');
const strings = JSON.parse(fs.readFileSync(path.join(root, 'localization/strings.json'), 'utf8'));
const config = JSON.parse(fs.readFileSync(path.join(root, 'assets/MCM/Config/LocalMapUpgrade/config.json'), 'utf8'));
// SKSE loads only the current language; explicitly provide English for non-German locales.
const languages = ['ENGLISH', 'GERMAN', 'FRENCH', 'ITALIAN', 'SPANISH', 'POLISH', 'CZECH', 'RUSSIAN', 'JAPANESE', 'CHINESE'];
const used = new Set();
for (const page of config.pages) {
  used.add(page.pageDisplayName);
  for (const control of page.content) {
    used.add(control.text);
    if (control.help) used.add(control.help);
    for (const option of control.valueOptions?.options ?? []) used.add(option);
  }
}
assert.deepEqual([...used].sort(), Object.keys(strings).sort(), 'Every UI string must have a unique translation key');
const directory = path.join(root, 'package/Interface/Translations');
fs.mkdirSync(directory, { recursive: true });
for (const language of languages) {
  const locale = language === 'GERMAN' ? 'de' : 'en';
  const lines = [...used].map(key => {
    const value = strings[key][locale];
    assert.ok(key.startsWith('$LMU_') && typeof value === 'string' && value.length > 0);
    assert.ok(!/[\r\n\t]/.test(value), 'Translation values must occupy a single tab-separated line');
    const line = `${key}\t${value}`;
    assert.ok(line.length < 510, 'SKSE uses a 512-character line buffer');
    return line;
  });
  const bytes = Buffer.from('\uFEFF' + lines.join('\r\n') + '\r\n', 'utf16le');
  const file = path.join(directory, `LocalMapUpgrade_${language}.txt`);
  fs.writeFileSync(file, bytes);
  const readback = fs.readFileSync(file);
  assert.equal(readback.readUInt16LE(), 0xFEFF);
  const table = new Map(readback.subarray(2).toString('utf16le').trimEnd().split('\r\n').map(line => line.split('\t')));
  assert.equal(table.size, used.size);
  for (const key of used) assert.equal(table.get(key), strings[key][locale]);
}
console.log(`PASS: ${used.size} strings, ${languages.length} UTF-16LE/BOM tables; German for GERMAN, English for every other packaged locale.`);
