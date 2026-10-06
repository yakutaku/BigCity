// BigCity skyline v3 generator (JS port of skyline.py) for use_figma. Deterministic.
// Returns { baseSvg, litSvg(citySize), order, candCount, colourCount }. Same code runs in Figma (use_figma) and Node.
function buildSkyline() {
  const W = 112, H = 95, HOR = 58, S = 3;
  const hx = h => [parseInt(h.slice(1, 3), 16), parseInt(h.slice(3, 5), 16), parseInt(h.slice(5, 7), 16)];
  const PH = { sky0: '#8CC3E6', sky1: '#A2CFEB', sky2: '#B9DBEC', sky3: '#CFE5EA', sky4: '#E4ECDF', sky5: '#F6E3C2', sun: '#F8DC8A', sunhi: '#FCEDB3', far: '#C9C3E4', pink: '#F1B3C3', pinkdk: '#E49CB1', lav: '#B8AAE4', lavdk: '#A394D6', win: '#9F90D2', tower: '#EE9E8E', towerlt: '#FAEBDD', water0: '#B4D2EE', water1: '#A5C8E8', water2: '#97BEE2', glint: '#F9E39A' };
  const P = {}; for (const k in PH) P[k] = hx(PH[k]);
  const img = new Array(W * H), city = new Uint8Array(W * H);
  const put = (x, y, c) => { if (x >= 0 && x < W && y >= 0 && y < H) img[y * W + x] = c; };
  const rect = (x0, y0, x1, y1, c, isCity) => { for (let y = y0; y < y1; y++) for (let x = x0; x < x1; x++) { if (x < 0 || x >= W || y < 0 || y >= H) continue; img[y * W + x] = P[c]; if (isCity) city[y * W + x] = 1; } };
  const bands = ['sky0', 'sky1', 'sky2', 'sky3', 'sky4', 'sky5'], bayer = [[0, .5], [.75, .25]];
  for (let y = 0; y < HOR; y++) { const t = y / (HOR - 1) * 5, i = Math.floor(t), f = t - i; for (let x = 0; x < W; x++) img[y * W + x] = P[bands[Math.min(i + (f > bayer[y % 2][x % 2] ? 1 : 0), 5)]]; }
  const cx = 57, cy = 50, r = 15;
  for (let y = 0; y < HOR; y++) for (let x = 0; x < W; x++) { const d = Math.hypot(x - cx, y - cy); if (d <= r) img[y * W + x] = P.sun; if (d <= r - 4 && y < cy - 4) img[y * W + x] = P.sunhi; }
  rect(14, 4, 15, 20, 'far'); rect(13, 20, 16, 24, 'far'); rect(12, 24, 17, 26, 'far'); rect(13, 26, 16, HOR, 'far');
  for (const [x0, h] of [[0, 30], [6, 34], [18, 32], [44, 36], [70, 34], [100, 30]]) rect(x0, HOR - h + 20, x0 + 8, HOR, 'far');
  for (const [x0, x1, top] of [[38, 46, 42], [46, 51, 36], [63, 68, 34], [68, 76, 44], [52, 56, 54], [58, 62, 53]]) { rect(x0, top, x1, HOR, 'pink'); rect(x1 - 1, top, x1, HOR, 'pinkdk'); }
  rect(64, 26, 65, 32, 'pink');
  const tx = 30; rect(tx, 2, tx + 1, 8, 'tower');
  for (let y = 8; y < HOR; y++) {
    const half = y < 14 ? 0 : y < 16 ? 1 : y < 30 ? 1 + Math.floor((y - 16) / 7) : y < 33 ? 4 : 3 + Math.floor((y - 33) / 4);
    for (let x = tx - half; x <= tx + half; x++) put(x, y, Math.floor(y / 2) % 2 === 0 ? P.tower : P.towerlt);
  }
  for (const y of [14, 15, 30, 31, 32]) { const w = y > 20 ? 5 : 2; for (let x = tx - w; x <= tx + w; x++) put(x, y, P.towerlt); }
  for (const [x0, x1, top] of [[0, 8, 40], [8, 14, 46], [16, 22, 44], [22, 27, 50], [35, 41, 48], [41, 49, 52], [49, 56, 55], [56, 63, 55], [63, 70, 53], [70, 77, 46]]) { rect(x0, top, x1, HOR, 'lav', true); rect(x1 - 1, top, x1, HOR, 'lavdk'); }
  rect(80, 22, 96, HOR, 'lav', true); rect(80, 16, 85, 22, 'lav', true); rect(91, 16, 96, 22, 'lav', true);
  rect(81, 13, 84, 16, 'lav'); rect(92, 13, 95, 16, 'lav'); rect(85, 26, 91, 28, 'lavdk'); rect(95, 16, 96, HOR, 'lavdk');
  [97, 101, 105].forEach((x0, i) => { const top = 24 + i * 3; rect(x0, top, x0 + 4, HOR, 'lav', true); rect(x0 + 1, top - 2, x0 + 3, top, 'lav'); rect(x0 + 1, top - 3, x0 + 2, top - 2, 'lav'); });
  rect(109, 30, 112, HOR, 'lav', true); rect(76, 32, 80, HOR, 'lav', true); rect(79, 32, 80, HOR, 'lavdk');
  const cand = [];
  for (let y = 0; y < HOR - 1; y++) for (let x = 0; x < W; x++) { const i = y * W + x; if (city[i] && y % 2 === 1 && x % 2 === 1 && img[i][0] === P.lav[0]) { img[i] = P.win; cand.push([x, y]); } }
  const N = H - HOR;
  for (let y = HOR; y < H; y++) { const t = (y - HOR) / N; for (let x = 0; x < W; x++) img[y * W + x] = t < .35 ? P.water0 : t < .7 ? P.water1 : P.water2; }
  for (let k = 0; k < N; k++) {
    const m = Math.floor(k / 2) % 3, shift = m === 1 ? 1 : m === 2 ? -1 : 0;
    let a = 0.42 * (1 - k / N) + 0.08; if (k % 3 === 2) a *= 0.4;
    const row = [];
    for (let x = 0; x < W; x++) { const sx = ((x - shift) % W + W) % W, src = img[(HOR - 1 - k) * W + sx], base = img[(HOR + k) * W + x]; row.push([0, 1, 2].map(c => Math.floor(base[c] * (1 - a) + src[c] * a))); }
    for (let x = 0; x < W; x++) img[(HOR + k) * W + x] = row[x];
  }
  for (const [k, half] of [[3, 14], [6, 10], [9, 7], [12, 4], [15, 2]]) for (let x = cx - half; x < cx + half; x++) img[(HOR + k) * W + x] = P.glint;
  for (let x = 0; x < W; x++) img[HOR * W + x] = img[HOR * W + x].map(v => Math.floor(v * .6 + 102));
  const key = c => '#' + c.map(v => v.toString(16).padStart(2, '0')).join('');
  const groups = new Map();
  for (let y = 0; y < H; y++) { let x = 0; while (x < W) { const k = key(img[y * W + x]); let n = 1; while (x + n < W && key(img[y * W + x + n]) === k) n++; groups.set(k, (groups.get(k) || '') + `M${x * S} ${y * S}h${n * S}v${S}h-${n * S}z`); x += n; } }
  const head = `<svg xmlns="http://www.w3.org/2000/svg" width="${W * S}" height="${H * S}" viewBox="0 0 ${W * S} ${H * S}">`;
  let baseSvg = head; for (const [c, d] of groups) baseSvg += `<path d="${d}" fill="${c}"/>`; baseSvg += '</svg>';
  // deterministic lighting order (mulberry32, seed 4); fraction of windows lit = 0.6 * citySize/100
  let s = 4; const rnd = () => { s |= 0; s = s + 0x6D2B79F5 | 0; let t = Math.imul(s ^ s >>> 15, 1 | s); t = t + Math.imul(t ^ t >>> 7, 61 | t) ^ t; return ((t ^ t >>> 14) >>> 0) / 4294967296; };
  const order = cand.slice(); for (let i = order.length - 1; i > 0; i--) { const j = Math.floor(rnd() * (i + 1)); [order[i], order[j]] = [order[j], order[i]]; }
  const litSvg = (citySize, colour = '#FBE08A') => { const n = Math.floor(order.length * 0.6 * citySize / 100); let d = ''; for (const [x, y] of order.slice(0, n)) d += `M${x * S} ${y * S}h${S}v${S}h-${S}z`; return head + (d ? `<path d="${d}" fill="${colour}"/>` : '') + '</svg>'; };
  return { baseSvg, litSvg, order, candCount: cand.length, colourCount: groups.size };
}

// Node: `node skyline_gen.js` writes the window lighting order for the JUCE build.
if (typeof module !== 'undefined' && require.main === module) {
  const { order } = buildSkyline();
  const out = {
    note: 'BigCity skyline windows in lighting order. Logical pixel grid 112x95; each window is 1 logical px (3 pt, 6 px @2x). Lit count = floor(count * 0.6 * citySize / 100).',
    grid: { width: 112, height: 95, pointsPerPixel: 3 },
    maxLitFraction: 0.6,
    litColour: '#FBE08A',
    count: order.length,
    windows: order,
  };
  require('fs').writeFileSync(process.argv[2] || 'skyline-windows.json', JSON.stringify(out));
  console.log('wrote', order.length, 'windows');
}
