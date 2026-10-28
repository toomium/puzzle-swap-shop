// Renders assets/src/icon.svg to assets/icon.png (48x48, SMDH) and
// assets/src/banner.svg to assets/banner.png (256x128, CIA banner).
// Usage: node assets/src/export_images.mjs
import { createRequire } from 'node:module';
import { readFileSync } from 'node:fs';
import path from 'node:path';
import { fileURLToPath } from 'node:url';

const { chromium } = createRequire(import.meta.url)('playwright');
const assets = path.resolve(path.dirname(fileURLToPath(import.meta.url)), '..');

const images = [
    { src: 'src/icon.svg', out: 'icon.png', width: 48, height: 48 },
    { src: 'src/banner.svg', out: 'banner.png', width: 256, height: 128 },
];

const browser = await chromium.launch();
for (const { src, out, width, height } of images) {
    const svg = readFileSync(path.join(assets, src), 'utf8');
    const page = await browser.newPage({ viewport: { width, height }, deviceScaleFactor: 1 });
    await page.setContent(`<html><body style="margin:0">${svg}</body></html>`);
    await page.locator('svg').screenshot({ path: path.join(assets, out) });
    await page.close();
    console.log(`wrote assets/${out}`);
}
await browser.close();
