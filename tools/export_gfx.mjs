// Renders every gfx/src/*.svg to gfx/<name>.png at its own size (1 SVG unit = 1 3DS pixel)
// and writes gfx/ui.t3s, the tex3ds atlas description. Called by tools/export_gfx.sh.
import { createRequire } from 'node:module';
import { readFileSync, readdirSync, writeFileSync } from 'node:fs';
import path from 'node:path';
import { fileURLToPath } from 'node:url';

const { chromium } = createRequire(import.meta.url)('playwright');
const root = path.resolve(path.dirname(fileURLToPath(import.meta.url)), '..');
const srcDir = path.join(root, 'gfx/src');
const names = readdirSync(srcDir).filter(f => f.endsWith('.svg')).sort();

const browser = await chromium.launch();
const page = await browser.newPage({ deviceScaleFactor: 1 });
for (const file of names) {
  const svg = readFileSync(path.join(srcDir, file), 'utf8');
  const w = +svg.match(/<svg[^>]*\swidth="(\d+)"/)[1];
  const h = +svg.match(/<svg[^>]*\sheight="(\d+)"/)[1];
  await page.setViewportSize({ width: w, height: h });
  await page.setContent(`<html><body style="margin:0;background:transparent">${svg}</body></html>`);
  const out = path.join(root, 'gfx', file.replace(/\.svg$/, '.png'));
  await page.screenshot({ path: out, omitBackground: true, clip: { x: 0, y: 0, width: w, height: h } });
  console.log(`gfx/${path.basename(out)}  ${w}x${h}`);
}
await browser.close();

// rgba8 keeps the soft gradients; edge borders stop neighbours bleeding in with linear filtering.
const t3s = ['--atlas -f rgba8 -z auto --border=edge', ...names.map(f => f.replace(/\.svg$/, '.png'))];
writeFileSync(path.join(root, 'gfx/ui.t3s'), t3s.join('\n') + '\n');
console.log(`gfx/ui.t3s  ${names.length} images`);
