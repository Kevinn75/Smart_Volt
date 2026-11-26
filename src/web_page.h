#ifndef WEB_PAGE_H
#define WEB_PAGE_H

#include <Arduino.h>

// Página completa (HTML + CSS + JS) sin dependencias externas: funciona también
// conectado a la red propia del medidor, sin internet.
static const char WEB_PAGE[] PROGMEM = R"rawliteral(<!DOCTYPE html>
<html lang="es">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>Smart Volt</title>
<style>
:root{
  --bg:#f4f4f2; --surface:#fcfcfb; --border:#e3e2de;
  --text:#0b0b0b; --text2:#52514e; --muted:#8a8984; --grid:#ecebe7;
  --series:#2a78d6; --series-fill:rgba(42,120,214,.10); --series2:#d96b18;
  --series3:#7a4fd0; --series3-fill:rgba(122,79,208,.16); --neg-fill:rgba(208,59,59,.18);
  --good:#0ca30c; --warn:#b77a00; --crit:#d03b3b;
  --btn:#0b0b0b; --btn-text:#fcfcfb;
  --money:#0a7d4f; --money-tint:rgba(10,125,79,.07); --money-line:rgba(10,125,79,.25);
}
@media (prefers-color-scheme: dark){
  :root:not([data-theme="light"]){
    --bg:#111110; --surface:#1a1a19; --border:#2e2e2b;
    --text:#f0efec; --text2:#c3c2b7; --muted:#8f8e86; --grid:#2a2a27;
    --series:#3987e5; --series-fill:rgba(57,135,229,.16); --series2:#f59e0b;
    --series3:#a78bfa; --series3-fill:rgba(167,139,250,.18); --neg-fill:rgba(230,103,103,.22);
    --good:#0ca30c; --warn:#fab219; --crit:#e66767;
    --btn:#f0efec; --btn-text:#1a1919;
    --money:#2fbf84; --money-tint:rgba(47,191,132,.09); --money-line:rgba(47,191,132,.3);
  }
}
*{box-sizing:border-box}
body{margin:0;background:var(--bg);color:var(--text);
  font:15px/1.4 system-ui,-apple-system,"Segoe UI",Roboto,sans-serif}
.wrap{max-width:1040px;margin:0 auto;padding:16px}
header{display:flex;flex-wrap:wrap;align-items:baseline;justify-content:space-between;gap:8px;margin-bottom:14px}
h1{font-size:20px;margin:0;font-weight:650;letter-spacing:-.01em}
.conn{font-size:13px;color:var(--text2);display:flex;align-items:center;gap:6px}
.dot{width:8px;height:8px;border-radius:50%;background:var(--muted);display:inline-block}
.dot.ok{background:var(--good)} .dot.bad{background:var(--crit)}
.hero{background:var(--surface);border:1px solid var(--border);border-radius:12px;padding:16px 18px;margin-bottom:12px}
.hero .lbl{font-size:13px;color:var(--text2)}
.hero .big{font-size:44px;font-weight:650;letter-spacing:-.02em;font-variant-numeric:tabular-nums;line-height:1.1}
.hero .big small{font-size:20px;font-weight:500;color:var(--text2);margin-left:4px}
.tiles{display:grid;grid-template-columns:repeat(auto-fit,minmax(140px,1fr));gap:10px;margin-bottom:12px}
.tile{background:var(--surface);border:1px solid var(--border);border-radius:12px;padding:12px 14px}
.tile .lbl{font-size:12px;color:var(--text2)}
.tile .val{font-size:24px;font-weight:600;font-variant-numeric:tabular-nums}
.tile .val small{font-size:13px;font-weight:500;color:var(--text2);margin-left:3px}
.alerts{margin-bottom:12px;display:flex;flex-direction:column;gap:6px}
.alert{border:1px solid var(--border);border-left:4px solid var(--warn);background:var(--surface);
  border-radius:8px;padding:8px 12px;font-size:14px}
.alert.crit{border-left-color:var(--crit)}
.alert b{margin-right:4px}
.toolbar{display:flex;flex-wrap:wrap;align-items:center;justify-content:space-between;gap:8px;margin:18px 0 8px}
.toolbar h2{font-size:16px;margin:0;font-weight:600}
.seg{display:inline-flex;border:1px solid var(--border);border-radius:8px;overflow:hidden;background:var(--surface)}
.seg button{border:0;background:transparent;color:var(--text2);padding:6px 12px;font:inherit;font-size:13px;cursor:pointer}
.seg button.on{background:var(--btn);color:var(--btn-text)}
.charts{display:grid;gap:10px}
.card{background:var(--surface);border:1px solid var(--border);border-radius:12px;padding:12px 12px 6px}
.card h3{font-size:13px;font-weight:600;margin:0 0 4px 4px;color:var(--text)}
.card h3 span{color:var(--text2);font-weight:500}
.plot{position:relative;height:170px}
.plot canvas{width:100%;height:100%;display:block;touch-action:pan-y}
.tip{position:absolute;pointer-events:none;background:var(--surface);color:var(--text);border:1px solid var(--border);
  border-radius:8px;padding:6px 9px;font-size:12px;box-shadow:0 4px 14px rgba(0,0,0,.12);white-space:nowrap;display:none;
  font-variant-numeric:tabular-nums}
.tip .t{color:var(--text2)}
details{background:var(--surface);border:1px solid var(--border);border-radius:12px;padding:12px 16px;margin-top:18px}
summary{cursor:pointer;font-weight:600}
.cfg{display:grid;gap:16px;margin-top:12px}
.cfg h4{margin:0 0 6px;font-size:14px}
.cfg p{margin:0 0 8px;color:var(--text2);font-size:13px}
.row{display:flex;flex-wrap:wrap;gap:8px;align-items:center}
input{font:inherit;padding:7px 10px;border:1px solid var(--border);border-radius:8px;background:var(--bg);color:var(--text);min-width:0}
input[type=number]{width:80px}
.btn{font:inherit;font-size:14px;padding:7px 14px;border-radius:8px;border:1px solid var(--btn);background:var(--btn);color:var(--btn-text);cursor:pointer}
.btn.ghost{background:transparent;color:var(--text)}
.btn.active{background:var(--crit);border-color:var(--crit);color:#fff}
.panel{background:var(--surface);border:1px solid var(--border);border-radius:12px;padding:12px 16px;margin-bottom:12px}
.panel h3{font-size:14px;font-weight:600;margin:0 0 8px}
.chk{display:inline-flex;align-items:center;gap:5px;font-size:13px;color:var(--text2);cursor:pointer}
.legend{display:flex;gap:14px;font-size:12px;color:var(--text2);margin:0 0 4px 4px}
.legend span{display:inline-flex;align-items:center;gap:5px}
.sw{display:inline-block;width:12px;height:3px;border-radius:2px}
.msg{font-size:13px;color:var(--text2)}
select{font:inherit;padding:7px 10px;border:1px solid var(--border);border-radius:8px;background:var(--bg);color:var(--text)}
.bill{background:var(--surface);border:1px solid var(--border);border-radius:16px;padding:18px;margin:18px 0 12px;
  background-image:linear-gradient(135deg,var(--money-tint),transparent 55%)}
.bill-head{display:flex;justify-content:space-between;align-items:flex-start;gap:10px;margin-bottom:14px}
.bill-head h2{font-size:18px;margin:0 0 2px;font-weight:650}
.badge{font-size:12px;font-weight:600;padding:4px 10px;border-radius:999px;background:var(--money-tint);color:var(--money);
  border:1px solid var(--money-line);white-space:nowrap}
.bill-grid{display:grid;grid-template-columns:1.3fr 1fr;gap:12px}
@media (max-width:620px){.bill-grid{grid-template-columns:1fr}}
.bill-main,.mini{background:var(--bg);border:1px solid var(--border);border-radius:12px;padding:14px 16px}
.bill-side{display:grid;gap:12px}
.bill .lbl{font-size:12px;color:var(--text2)}
.money{font-size:42px;font-weight:700;letter-spacing:-.02em;color:var(--money);font-variant-numeric:tabular-nums;line-height:1.15}
.money.sm{font-size:24px;font-weight:650}
.meter{margin-top:12px}
.meter-bar{height:8px;border-radius:999px;background:var(--grid);overflow:hidden;margin-bottom:5px}
.meter-bar i{display:block;height:100%;width:0;background:var(--money);border-radius:999px;transition:width .4s}
.bill-cfg{display:flex;flex-wrap:wrap;gap:14px 20px;margin-top:16px;padding-top:14px;border-top:1px solid var(--border)}
.bill-cfg .lbl{margin-bottom:5px}
.seg.est button.on{background:var(--money);color:#fff}
.bill-note{font-size:12px;color:var(--muted);margin-top:12px}
@media (max-width:520px){.money{font-size:34px}}
footer{color:var(--muted);font-size:12px;margin:16px 0 8px;text-align:center}
@media (max-width:520px){.hero .big{font-size:36px}.tile .val{font-size:21px}}
</style>
</head>
<body>
<div class="wrap">
  <header>
    <h1>Smart Volt</h1>
    <div class="conn"><span class="dot" id="dot"></span><span id="conn">Conectando…</span></div>
  </header>

  <div class="hero">
    <div class="lbl">Potencia activa</div>
    <div class="big" id="p">–<small>W</small></div>
  </div>

  <div class="tiles">
    <div class="tile"><div class="lbl">Voltaje</div><div class="val" id="v">–<small>V</small></div></div>
    <div class="tile"><div class="lbl">Corriente</div><div class="val" id="i">–<small>A</small></div></div>
    <div class="tile"><div class="lbl">Potencia aparente</div><div class="val" id="s">–<small>VA</small></div></div>
    <div class="tile"><div class="lbl">Factor de potencia</div><div class="val" id="fp">–</div></div>
    <div class="tile"><div class="lbl">Energía acumulada</div><div class="val" id="kwh">–<small>kWh</small></div></div>
  </div>

  <div class="alerts" id="alerts"></div>

  <div class="panel">
    <h3>Límites y salidas (Buzzer / LED)</h3>
    <div class="row" style="margin-bottom:8px">
      <span class="msg">I máx (A):</span>
      <input type="number" id="iLim" min="0" max="100" step="0.1" value="0">
      <span class="msg">V mín (V):</span>
      <input type="number" id="vMin" min="0" max="300" step="1" value="0">
      <span class="msg">V máx (V):</span>
      <input type="number" id="vMax" min="0" max="300" step="1" value="0">
      <span class="msg">(0 = desactivado)</span>
    </div>
    <div class="row" style="margin-bottom:8px">
      <label class="chk"><input type="checkbox" id="buzEn" checked> Buzzer en alarma</label>
      <label class="chk"><input type="checkbox" id="ledEn" checked> LED en alarma</label>
      <button class="btn" id="limSave">Guardar</button>
      <span class="msg" id="limMsg"></span>
    </div>
    <div class="row">
      <span class="msg">Prueba manual:</span>
      <button class="btn ghost" id="buzBtn">Buzzer: OFF</button>
      <button class="btn ghost" id="ledBtn">LED: OFF</button>
      <span class="msg" id="outMsg"></span>
    </div>
  </div>

  <div class="toolbar">
    <h2>Potencia en el tiempo</h2>
    <div class="seg" id="range">
      <button data-r="60">1 min</button><button data-r="300" class="on">5 min</button><button data-r="600">10 min</button>
    </div>
  </div>
  <div class="card">
    <h3>Potencia activa <span>(W)</span></h3>
    <div class="plot" style="height:240px"><canvas id="cP" aria-label="Gráfica de potencia activa"></canvas><div class="tip"></div></div>
  </div>

  <div class="toolbar">
    <h2>Formas de onda</h2>
    <div class="row">
      <span class="msg" id="phaseInfo">Sin carga</span>
      <button class="btn ghost" id="wavePause" style="padding:5px 10px;font-size:13px">Pausar</button>
    </div>
  </div>
  <div class="charts">
    <div class="card">
      <h3>Voltaje vs corriente</h3>
      <div class="legend">
        <span><i class="sw" style="background:var(--series)"></i>Voltaje (V)</span>
        <span><i class="sw" style="background:var(--series2)"></i>Corriente (A)</span>
      </div>
      <div class="plot" style="height:210px"><canvas id="cWave" aria-label="Formas de onda de voltaje y corriente"></canvas><div class="tip"></div></div>
    </div>
    <div class="card">
      <h3>Potencia instantánea <span>p(t) = v(t) · i(t)</span></h3>
      <div class="legend">
        <span><i class="sw" style="background:var(--series3)"></i>Potencia (W)</span>
        <span><i class="sw" style="background:var(--muted)"></i>Promedio = potencia activa</span>
        <span><i class="sw" style="background:var(--crit)"></i>Vuelve a la red (p &lt; 0)</span>
      </div>
      <div class="plot" style="height:210px"><canvas id="cPw" aria-label="Forma de onda de la potencia instantánea"></canvas><div class="tip"></div></div>
    </div>
  </div>

  <section class="bill">
    <div class="bill-head">
      <div>
        <h2>¿Cuánto vas a pagar?</h2>
        <div class="msg">Estimado según tu estrato y la tarifa de tu recibo</div>
      </div>
      <span class="badge" id="bBadge">Estrato 3</span>
    </div>

    <div class="bill-grid">
      <div class="bill-main">
        <div class="lbl">Por pagar hasta ahora</div>
        <div class="money" id="cAcc">$ –</div>
        <div class="msg" id="cAccSub">– kWh consumidos</div>
        <div class="meter" id="subsBox">
          <div class="meter-bar"><i id="subsFill"></i></div>
          <div class="msg" id="subsTxt"></div>
        </div>
      </div>
      <div class="bill-side">
        <div class="mini">
          <div class="lbl">Si sigues así, al mes</div>
          <div class="money sm" id="cMonth">$ –</div>
          <div class="msg" id="cMonthSub">–</div>
        </div>
        <div class="mini">
          <div class="lbl">Cada hora, con el consumo actual</div>
          <div class="money sm" id="cHour">$ –</div>
          <div class="msg" id="cHourSub">–</div>
        </div>
      </div>
    </div>

    <div class="bill-cfg">
      <div>
        <div class="lbl">Estrato</div>
        <div class="seg est" id="estrato">
          <button data-e="1">1</button><button data-e="2">2</button><button data-e="3">3</button>
          <button data-e="4">4</button><button data-e="5">5</button><button data-e="6">6</button>
          <button data-e="c">Comercial</button>
        </div>
      </div>
      <div>
        <div class="lbl">Tarifa del recibo ($/kWh)</div>
        <input type="number" id="tarifa" min="0" step="1" style="width:120px">
      </div>
      <div>
        <div class="lbl">Consumo de subsistencia</div>
        <select id="subs">
          <option value="173">173 kWh · bajo 1000 m</option>
          <option value="130">130 kWh · sobre 1000 m</option>
        </select>
      </div>
    </div>
    <div class="bill-note" id="bNote"></div>
  </section>

  <details>
    <summary>Configuración</summary>
    <div class="cfg">
      <div>
        <h4>Media móvil</h4>
        <p>Promedia las lecturas de los últimos N segundos para que no salten por cambios pequeños.
           Si se enciende o apaga una carga, el valor se actualiza enseguida.</p>
        <div class="row"><input type="number" id="avgN" min="1" max="30" value="5"> <span class="msg">segundos</span>
          <button class="btn" id="avgSave">Guardar</button><span class="msg" id="avgMsg"></span></div>
      </div>
      <div>
        <h4>Energía</h4>
        <p>Pone en cero el contador de kWh.</p>
        <div class="row"><button class="btn ghost" id="eReset">Poner en 0</button><span class="msg" id="eMsg"></span></div>
      </div>
      <div>
        <h4>Wi-Fi de la casa</h4>
        <p id="wifiInfo">Para ver la página desde la red de la casa. La red propia "SmartVolt" sigue activa.</p>
        <div class="row">
          <input id="ssid" placeholder="Nombre de la red" maxlength="32">
          <input id="pass" type="password" placeholder="Clave" maxlength="64">
          <button class="btn" id="wSave">Conectar</button>
        </div>
        <div class="msg" id="wMsg" style="margin-top:6px"></div>
      </div>
    </div>
  </details>

  <footer id="foot">Actualiza cada segundo</footer>
</div>

<script>
(function(){
  const MAXPTS = 600;
  let range = 300;              // segundos visibles
  let offset = null;            // reloj del navegador - segundos del medidor
  let lastT = -1;
  const T = [], V = [], I = [], P = [];

  const $ = id => document.getElementById(id);
  const css = n => getComputedStyle(document.documentElement).getPropertyValue(n).trim();
  const fmt = (x, d) => (x == null || isNaN(x)) ? '–' : x.toLocaleString('es-CO', {minimumFractionDigits:d, maximumFractionDigits:d});
  const clock = t => { const d = new Date((t + (offset || 0)) * 1000);
    return d.toLocaleTimeString('es-CO', {hour:'2-digit', minute:'2-digit', second:'2-digit', hour12:false}); };

  // ---------------- Gráficas (canvas, una serie por gráfica) ----------------
  function niceScale(lo, hi, minSpan, zero){
    if (zero){ lo = Math.min(0, lo); hi = Math.max(0, hi); }
    // Rango mínimo: así el ruido pequeño no se ve como un cambio grande
    if (hi - lo < minSpan){ const c = (hi + lo) / 2; lo = zero && lo >= 0 ? 0 : c - minSpan/2; hi = lo + minSpan; }
    const raw = (hi - lo) / 4, mag = Math.pow(10, Math.floor(Math.log10(raw))), r = raw / mag;
    const step = (r <= 1 ? 1 : r <= 2 ? 2 : r <= 2.5 ? 2.5 : r <= 5 ? 5 : 10) * mag;
    return {lo: Math.floor(lo/step)*step, hi: Math.ceil(hi/step)*step, step};
  }

  function makeChart(id, data, unit, dec, minSpan, zero){
    const cv = $(id), tip = cv.parentNode.querySelector('.tip');
    const ch = {cv, tip, data, unit, dec, minSpan, zero, hover:null, geom:null};
    const move = e => { const r = cv.getBoundingClientRect(); ch.hover = e.clientX - r.left; draw(ch); };
    cv.addEventListener('pointermove', move);
    cv.addEventListener('pointerdown', move);
    cv.addEventListener('pointerleave', () => { ch.hover = null; draw(ch); });
    return ch;
  }

  function draw(ch){
    const cv = ch.cv, ctx = cv.getContext('2d'), dpr = window.devicePixelRatio || 1;
    const w = cv.clientWidth, h = cv.clientHeight;
    if (cv.width !== Math.round(w*dpr) || cv.height !== Math.round(h*dpr)){ cv.width = Math.round(w*dpr); cv.height = Math.round(h*dpr); }
    ctx.setTransform(dpr,0,0,dpr,0,0); ctx.clearRect(0,0,w,h);

    const L = 52, R = 10, Tp = 8, B = 22, pw = w - L - R, ph = h - Tp - B;
    const tMax = T.length ? T[T.length-1] : 0, tMin = tMax - range;
    let i0 = 0; while (i0 < T.length && T[i0] < tMin) i0++;
    const vals = ch.data.slice(i0), ts = T.slice(i0);

    let lo = Infinity, hi = -Infinity;
    for (const v of vals){ if (v < lo) lo = v; if (v > hi) hi = v; }
    if (!vals.length){ lo = 0; hi = ch.minSpan; }
    const sc = niceScale(lo, hi, ch.minSpan, ch.zero);
    const X = t => L + (t - tMin) / range * pw;
    const Y = v => Tp + (1 - (v - sc.lo) / (sc.hi - sc.lo)) * ph;

    // Rejilla y ejes (recesivos)
    ctx.font = '11px system-ui,-apple-system,Segoe UI,Roboto,sans-serif';
    ctx.fillStyle = css('--muted'); ctx.strokeStyle = css('--grid'); ctx.lineWidth = 1;
    ctx.textAlign = 'right'; ctx.textBaseline = 'middle';
    const yd = sc.step < 1 ? (sc.step < 0.1 ? 2 : 1) : 0;
    for (let v = sc.lo; v <= sc.hi + sc.step/2; v += sc.step){
      const y = Math.round(Y(v)) + .5;
      ctx.beginPath(); ctx.moveTo(L, y); ctx.lineTo(w - R, y); ctx.stroke();
      ctx.fillText(fmt(v, yd), L - 8, y);
    }
    ctx.textAlign = 'center'; ctx.textBaseline = 'top';
    const nX = w < 420 ? 3 : 5;
    for (let k = 0; k <= nX; k++){
      const t = tMin + range * k / nX; let x = X(t);
      ctx.textAlign = k === 0 ? 'left' : k === nX ? 'right' : 'center';
      if (offset !== null && t >= 0) ctx.fillText(clock(t).slice(0, range > 120 ? 5 : 8), x, h - B + 6);
    }

    // Línea (se corta si faltan más de 3 s de datos)
    if (vals.length > 1){
      // Relleno suave bajo la línea
      const y0 = Y(Math.max(sc.lo, Math.min(sc.hi, 0)));
      ctx.fillStyle = css('--series-fill'); ctx.beginPath();
      ctx.moveTo(X(ts[0]), y0);
      for (let k = 0; k < vals.length; k++) ctx.lineTo(X(ts[k]), Y(vals[k]));
      ctx.lineTo(X(ts[ts.length-1]), y0); ctx.closePath(); ctx.fill();
    }
    if (vals.length){
      ctx.strokeStyle = css('--series'); ctx.lineWidth = 2; ctx.lineJoin = 'round'; ctx.lineCap = 'round';
      ctx.beginPath(); let prev = null;
      for (let k = 0; k < vals.length; k++){
        const x = X(ts[k]), y = Y(vals[k]);
        if (prev === null || ts[k] - prev > 3) ctx.moveTo(x, y); else ctx.lineTo(x, y);
        prev = ts[k];
      }
      ctx.stroke();
    }

    // Cruz y etiqueta al pasar el mouse / tocar
    if (ch.hover !== null && vals.length && ch.hover >= L && ch.hover <= w - R){
      const tH = tMin + (ch.hover - L) / pw * range;
      let best = 0, bd = Infinity;
      for (let k = 0; k < ts.length; k++){ const d = Math.abs(ts[k] - tH); if (d < bd){ bd = d; best = k; } }
      const x = X(ts[best]), y = Y(vals[best]);
      ctx.strokeStyle = css('--muted'); ctx.lineWidth = 1; ctx.setLineDash([3,3]);
      ctx.beginPath(); ctx.moveTo(Math.round(x)+.5, Tp); ctx.lineTo(Math.round(x)+.5, Tp + ph); ctx.stroke(); ctx.setLineDash([]);
      ctx.fillStyle = css('--series'); ctx.strokeStyle = css('--surface'); ctx.lineWidth = 2;
      ctx.beginPath(); ctx.arc(x, y, 4.5, 0, Math.PI*2); ctx.fill(); ctx.stroke();
      ch.tip.innerHTML = '<div class="t">' + clock(ts[best]) + '</div><b>' + fmt(vals[best], ch.dec) + ' ' + ch.unit + '</b>';
      ch.tip.style.display = 'block';
      const tw = ch.tip.offsetWidth; let tx = x + 12; if (tx + tw > w) tx = x - tw - 12;
      ch.tip.style.left = tx + 'px'; ch.tip.style.top = Math.max(0, Math.min(y - 20, h - 44)) + 'px';
    } else {
      ch.tip.style.display = 'none';
    }
  }

  const charts = [
    makeChart('cP', P, 'W', 1, 50, true),
  ];

  // ---------------- Formas de onda: V vs I y potencia instantánea ----------------
  let wave = {dt_ms:0.25, v:[], i:[], fp:0}, wavePaused = false;
  const hoverOf = (cv, set) => {
    const mv = e => { const r = cv.getBoundingClientRect(); set(e.clientX - r.left); };
    cv.addEventListener('pointermove', mv);
    cv.addEventListener('pointerdown', mv);
    cv.addEventListener('pointerleave', () => set(null));
  };
  const wCv = $('cWave'), wTip = wCv.parentNode.querySelector('.tip');
  const pCv = $('cPw'),  pTip = pCv.parentNode.querySelector('.tip');
  let waveHover = null, pHover = null;
  hoverOf(wCv, x => { waveHover = x; drawWave(); });
  hoverOf(pCv, x => { pHover = x; drawPWave(); });

  function prepCanvas(cv){
    const ctx = cv.getContext('2d'), dpr = window.devicePixelRatio || 1;
    const w = cv.clientWidth, h = cv.clientHeight;
    if (cv.width !== Math.round(w*dpr) || cv.height !== Math.round(h*dpr)){ cv.width = Math.round(w*dpr); cv.height = Math.round(h*dpr); }
    ctx.setTransform(dpr,0,0,dpr,0,0); ctx.clearRect(0,0,w,h);
    ctx.font = '11px system-ui,-apple-system,Segoe UI,Roboto,sans-serif';
    return {ctx, w, h};
  }
  function timeAxis(ctx, w, h, L, R, B, n){
    const pw = w - L - R, totalMs = (n > 1 ? n - 1 : 133) * wave.dt_ms, nX = w < 420 ? 3 : 5;
    ctx.fillStyle = css('--muted'); ctx.textBaseline = 'top';
    for (let k = 0; k <= nX; k++){
      ctx.textAlign = k === 0 ? 'left' : k === nX ? 'right' : 'center';
      ctx.fillText(fmt(totalMs * k / nX, 1) + ' ms', L + pw * k / nX, h - B + 6);
    }
  }
  function cursor(ctx, x, Tp, ph){
    ctx.strokeStyle = css('--muted'); ctx.lineWidth = 1; ctx.setLineDash([3,3]);
    ctx.beginPath(); ctx.moveTo(Math.round(x)+.5, Tp); ctx.lineTo(Math.round(x)+.5, Tp + ph); ctx.stroke(); ctx.setLineDash([]);
  }
  function placeTip(tip, x, w, y){
    tip.style.display = 'block';
    const tw = tip.offsetWidth; let tx = x + 12; if (tx + tw > w) tx = x - tw - 12;
    tip.style.left = tx + 'px'; tip.style.top = y + 'px';
  }

  function drawWave(){
    const {ctx, w, h} = prepCanvas(wCv);
    const L = 48, R = 48, Tp = 10, B = 22, pw = w - L - R, ph = h - Tp - B;
    const n = wave.v.length;
    let maxV = 20, maxI = 0.5;
    for (let k = 0; k < n; k++){
      maxV = Math.max(maxV, Math.abs(wave.v[k]));
      maxI = Math.max(maxI, Math.abs(wave.i[k]));
    }
    maxV = Math.ceil(maxV * 1.15 / 10) * 10;
    const iStep = maxI < 2 ? 0.5 : maxI < 10 ? 1 : 5;
    maxI = Math.ceil(maxI * 1.15 / iStep) * iStep;

    const yMid = Tp + ph / 2;
    const X = k => L + (n > 1 ? k / (n - 1) : 0) * pw;
    const YV = v => yMid - (v / maxV) * (ph / 2);
    const YI = i => yMid - (i / maxI) * (ph / 2);

    ctx.strokeStyle = css('--grid'); ctx.lineWidth = 1;
    for (let s = -2; s <= 2; s++){
      const y = Math.round(yMid - s * (ph / 4)) + .5;
      ctx.setLineDash(s === 0 ? [4,3] : []);
      ctx.beginPath(); ctx.moveTo(L, y); ctx.lineTo(w - R, y); ctx.stroke(); ctx.setLineDash([]);
      ctx.fillStyle = css('--series'); ctx.textAlign = 'right'; ctx.textBaseline = 'middle';
      ctx.fillText(fmt(s * maxV / 2, 0), L - 6, y);
      ctx.fillStyle = css('--series2'); ctx.textAlign = 'left';
      ctx.fillText(fmt(s * maxI / 2, maxI < 2 ? 2 : 1), w - R + 6, y);
    }
    timeAxis(ctx, w, h, L, R, B, n);

    if (n > 1){
      const line = (arr, Yfn, color) => {
        ctx.strokeStyle = color; ctx.lineWidth = 2; ctx.lineJoin = 'round'; ctx.beginPath();
        for (let k = 0; k < n; k++){ const x = X(k), y = Yfn(arr[k]); if (k === 0) ctx.moveTo(x, y); else ctx.lineTo(x, y); }
        ctx.stroke();
      };
      line(wave.v, YV, css('--series'));
      line(wave.i, YI, css('--series2'));
    }

    if (waveHover !== null && n > 1 && waveHover >= L && waveHover <= w - R){
      const k = Math.max(0, Math.min(n - 1, Math.round((waveHover - L) / pw * (n - 1))));
      cursor(ctx, X(k), Tp, ph);
      wTip.innerHTML = '<div class="t">' + fmt(k * wave.dt_ms, 2) + ' ms</div>' +
        '<div style="color:var(--series)"><b>' + fmt(wave.v[k], 1) + ' V</b></div>' +
        '<div style="color:var(--series2)"><b>' + fmt(wave.i[k], 3) + ' A</b></div>';
      placeTip(wTip, X(k), w, 8);
    } else wTip.style.display = 'none';
  }

  // p(t) = v(t)·i(t): oscila al doble de la frecuencia de la red; su promedio es la potencia activa
  function drawPWave(){
    const {ctx, w, h} = prepCanvas(pCv);
    const L = 52, R = 12, Tp = 10, B = 22, pw = w - L - R, ph = h - Tp - B;
    const n = Math.min(wave.v.length, wave.i.length);
    const pw_ = new Array(n);
    let lo = 0, hi = 0, sum = 0;
    for (let k = 0; k < n; k++){
      const p = wave.v[k] * wave.i[k];
      pw_[k] = p; sum += p;
      if (p < lo) lo = p; if (p > hi) hi = p;
    }
    const avg = n ? sum / n : 0;
    const sc = niceScale(lo, hi, 20, true);
    const X = k => L + (n > 1 ? k / (n - 1) : 0) * pw;
    const Y = p => Tp + (1 - (p - sc.lo) / (sc.hi - sc.lo)) * ph;

    ctx.strokeStyle = css('--grid'); ctx.lineWidth = 1;
    ctx.fillStyle = css('--muted'); ctx.textAlign = 'right'; ctx.textBaseline = 'middle';
    const yd = sc.step < 1 ? 1 : 0;
    for (let v = sc.lo; v <= sc.hi + sc.step/2; v += sc.step){
      const y = Math.round(Y(v)) + .5;
      ctx.setLineDash(Math.abs(v) < sc.step/2 ? [4,3] : []);
      ctx.beginPath(); ctx.moveTo(L, y); ctx.lineTo(w - R, y); ctx.stroke(); ctx.setLineDash([]);
      ctx.fillText(fmt(v, yd), L - 8, y);
    }
    timeAxis(ctx, w, h, L, R, B, n);

    if (n > 1){
      const y0 = Y(0);
      // Área positiva (consumo) y negativa (energía que vuelve a la red)
      const area = (sel, color) => {
        ctx.fillStyle = color; ctx.beginPath(); ctx.moveTo(X(0), y0);
        for (let k = 0; k < n; k++) ctx.lineTo(X(k), Y(sel(pw_[k])));
        ctx.lineTo(X(n-1), y0); ctx.closePath(); ctx.fill();
      };
      area(p => Math.max(0, p), css('--series3-fill'));
      area(p => Math.min(0, p), css('--neg-fill'));

      ctx.strokeStyle = css('--series3'); ctx.lineWidth = 2; ctx.lineJoin = 'round'; ctx.beginPath();
      for (let k = 0; k < n; k++){ const x = X(k), y = Y(pw_[k]); if (k === 0) ctx.moveTo(x, y); else ctx.lineTo(x, y); }
      ctx.stroke();

      // Línea del promedio (potencia activa)
      const ya = Math.round(Y(avg)) + .5;
      ctx.strokeStyle = css('--text2'); ctx.lineWidth = 1.5; ctx.setLineDash([6,4]);
      ctx.beginPath(); ctx.moveTo(L, ya); ctx.lineTo(w - R, ya); ctx.stroke(); ctx.setLineDash([]);
      ctx.fillStyle = css('--text2'); ctx.textAlign = 'right'; ctx.textBaseline = 'bottom';
      ctx.fillText('P = ' + fmt(avg, avg >= 100 ? 0 : 1) + ' W', w - R - 4, ya - 3);
    }

    if (pHover !== null && n > 1 && pHover >= L && pHover <= w - R){
      const k = Math.max(0, Math.min(n - 1, Math.round((pHover - L) / pw * (n - 1))));
      cursor(ctx, X(k), Tp, ph);
      pTip.innerHTML = '<div class="t">' + fmt(k * wave.dt_ms, 2) + ' ms</div>' +
        '<div style="color:var(--series3)"><b>' + fmt(pw_[k], 1) + ' W</b></div>' +
        '<div class="t">' + fmt(wave.v[k], 1) + ' V · ' + fmt(wave.i[k], 3) + ' A</div>';
      placeTip(pTip, X(k), w, 8);
    } else pTip.style.display = 'none';
  }

  $('wavePause').onclick = () => {
    wavePaused = !wavePaused;
    $('wavePause').textContent = wavePaused ? 'Reanudar' : 'Pausar';
  };

  const drawAll = () => { charts.forEach(draw); drawWave(); drawPWave(); };
  window.addEventListener('resize', drawAll);
  if (window.matchMedia) window.matchMedia('(prefers-color-scheme: dark)').addEventListener('change', drawAll);

  document.querySelectorAll('#range button').forEach(b => b.addEventListener('click', () => {
    document.querySelectorAll('#range button').forEach(x => x.classList.toggle('on', x === b));
    range = +b.dataset.r; drawAll();
  }));

  function push(t, v, i, p){
    if (t <= lastT) return;
    lastT = t; T.push(t); V.push(v); I.push(i); P.push(p);
    while (T.length > MAXPTS){ T.shift(); V.shift(); I.shift(); P.shift(); }
  }

  // ---------------- Costo según estrato ----------------
  // Estratos 1-3: subsidio sobre el consumo de subsistencia (lo demás a tarifa plena).
  // Estrato 4: tarifa plena. Estratos 5, 6 y comercial: tarifa + 20 % de contribución.
  const SUBSIDIO = {'1':0.60, '2':0.50, '3':0.15};
  const CONTRIB = {'5':0.20, '6':0.20, 'c':0.20};
  let bill = {e:'3', t:950, s:173};
  try { Object.assign(bill, JSON.parse(localStorage.getItem('sv_bill') || '{}')); } catch(e){}
  const saveBill = () => { try { localStorage.setItem('sv_bill', JSON.stringify(bill)); } catch(e){} };
  const money = x => (x == null || isNaN(x)) ? '$ –' :
    '$ ' + Math.round(x).toLocaleString('es-CO');
  let lastData = null;

  // Precio de un kWh cuando ya se llevan "kwh" consumidos
  function rate(kwh){
    const e = bill.e, t = bill.t;
    if (SUBSIDIO[e] !== undefined) return kwh < bill.s ? t * (1 - SUBSIDIO[e]) : t;
    return t * (1 + (CONTRIB[e] || 0));
  }
  function cost(kwh){
    const e = bill.e, t = bill.t;
    if (SUBSIDIO[e] !== undefined){
      const a = Math.min(kwh, bill.s), b = Math.max(0, kwh - bill.s);
      return a * t * (1 - SUBSIDIO[e]) + b * t;
    }
    return kwh * t * (1 + (CONTRIB[e] || 0));
  }

  function renderBill(){
    const e = bill.e, lbl = e === 'c' ? 'Comercial' : 'Estrato ' + e;
    $('bBadge').textContent = lbl;
    document.querySelectorAll('#estrato button').forEach(b => b.classList.toggle('on', b.dataset.e === e));
    if (document.activeElement !== $('tarifa')) $('tarifa').value = bill.t;
    $('subs').value = String(bill.s);

    const sub = SUBSIDIO[e];
    $('subs').disabled = sub === undefined;
    $('bNote').textContent = sub !== undefined
      ? 'Subsidio del ' + Math.round(sub*100) + ' % en los primeros ' + bill.s + ' kWh del mes. Pon la energía en 0 (Configuración) el día que llega el recibo.'
      : (CONTRIB[e] ? 'Incluye 20 % de contribución sobre la tarifa. ' : 'Pagas la tarifa plena, sin subsidio ni contribución. ')
        + 'Pon la energía en 0 (Configuración) el día que llega el recibo.';

    if (!lastData) return;
    const kwh = Math.max(0, lastData.kwh || 0);
    $('cAcc').textContent = money(cost(kwh));
    $('cAccSub').textContent = fmt(kwh, 2) + ' kWh consumidos';

    if (sub !== undefined){
      const pct = Math.min(100, kwh / bill.s * 100);
      $('subsBox').style.display = '';
      $('subsFill').style.width = pct + '%';
      $('subsTxt').textContent = kwh < bill.s
        ? 'Llevas ' + fmt(kwh, 1) + ' de ' + bill.s + ' kWh con subsidio'
        : 'Ya pasaste los ' + bill.s + ' kWh con subsidio: el resto va a tarifa plena';
    } else {
      $('subsBox').style.display = 'none';
    }

    // Proyección del mes con el promedio de potencia que muestra la gráfica
    let n = 0, sum = 0;
    const tMin = (T.length ? T[T.length-1] : 0) - range;
    for (let k = 0; k < T.length; k++) if (T[k] >= tMin){ sum += Math.max(0, P[k]); n++; }
    const avgW = n ? sum / n : Math.max(0, lastData.p);
    const monthKwh = avgW * 24 * 30 / 1000;
    $('cMonth').textContent = money(cost(monthKwh));
    $('cMonthSub').textContent = '≈ ' + fmt(monthKwh, 1) + ' kWh con ' + fmt(avgW, 0) + ' W promedio';

    const nowW = Math.max(0, lastData.p);
    $('cHour').textContent = money(nowW / 1000 * rate(kwh));
    $('cHourSub').textContent = fmt(nowW, 0) + ' W · ' + money(rate(kwh)) + ' por kWh';
  }

  document.querySelectorAll('#estrato button').forEach(b => b.addEventListener('click', () => {
    bill.e = b.dataset.e; saveBill(); renderBill();
  }));
  $('tarifa').addEventListener('input', () => {
    const t = parseFloat($('tarifa').value);
    if (t >= 0){ bill.t = t; saveBill(); renderBill(); }
  });
  $('subs').addEventListener('change', () => { bill.s = +$('subs').value; saveBill(); renderBill(); });
  renderBill();

  // ---------------- Datos ----------------
  function setVal(id, txt, unit){ $(id).innerHTML = txt + (unit ? '<small>' + unit + '</small>' : ''); }

  function render(d){
    setVal('p', fmt(d.p, Math.abs(d.p) >= 100 ? 0 : 1), 'W');
    setVal('v', fmt(d.v, 1), 'V');
    setVal('i', fmt(d.i, d.i < 10 ? 3 : 2), 'A');
    setVal('s', fmt(d.s, 1), 'VA');
    setVal('fp', d.s > 0.5 ? fmt(d.fp, 2) : '–', '');
    setVal('kwh', fmt(d.kwh, 3), 'kWh');

    const a = [];
    if (!d.v_ok) a.push(['crit', 'Voltaje:', 'el centro del pin es ' + Math.round(d.vb) + ' mV (esperado ~1470). Revisa los 5 V del ZMPT101B, el divisor o el cable.']);
    else if (!d.v_sig) a.push(['', 'Voltaje:', 'no hay señal de la red. ¿El ZMPT101B está conectado al tomacorriente?']);
    else if (d.v_clip) a.push(['crit', 'Voltaje recortado:', 'pico + ' + d.vpk_p + ' mV / pico − ' + d.vpk_n + ' mV. Gira el potenciómetro del ZMPT101B para bajar la ganancia hasta que los dos picos queden casi iguales, y luego calibra con "cal v".']);
    if (!d.i_ok) a.push(['crit', 'Corriente:', 'el centro del pin es ' + Math.round(d.ib) + ' mV (esperado ~1650). Revisa el divisor 10k/10k, el condensador o el cable.']);
    else if (d.p < -1) a.push(['', 'Corriente:', 'la potencia sale negativa. Gira la pinza o escribe "invertir" en la consola.']);
    if (d.over) a.push(['crit', 'Sobrecarga:', 'la corriente (' + fmt(d.i, 2) + ' A) supera el límite de ' + fmt(d.i_lim, 1) + ' A.']);
    if (d.uv) a.push(['crit', 'Bajo voltaje:', 'el voltaje (' + fmt(d.v, 1) + ' V) está por debajo de ' + fmt(d.v_min, 0) + ' V.']);
    if (d.ov) a.push(['crit', 'Sobrevoltaje:', 'el voltaje (' + fmt(d.v, 1) + ' V) supera el límite de ' + fmt(d.v_max, 0) + ' V.']);
    $('alerts').innerHTML = a.map(x => '<div class="alert ' + x[0] + '"><b>' + x[1] + '</b>' + x[2] + '</div>').join('');

    if (document.activeElement !== $('iLim')) $('iLim').value = d.i_lim;
    if (document.activeElement !== $('vMin')) $('vMin').value = d.v_min;
    if (document.activeElement !== $('vMax')) $('vMax').value = d.v_max;
    if (document.activeElement !== $('buzEn')) $('buzEn').checked = !!d.buz_en;
    if (document.activeElement !== $('ledEn')) $('ledEn').checked = !!d.led_en;
    buzMan = !!d.buz_man; ledMan = !!d.led_man;
    $('buzBtn').textContent = 'Buzzer: ' + (d.buz_on ? 'ON' : 'OFF');
    $('buzBtn').classList.toggle('active', !!d.buz_on);
    $('ledBtn').textContent = 'LED: ' + (d.led_on ? 'ON' : 'OFF');
    $('ledBtn').classList.toggle('active', !!d.led_on);

    if (d.s > 0.5 && d.i > 0.05){
      const deg = Math.acos(Math.min(1, Math.abs(d.fp))) * 180 / Math.PI;
      $('phaseInfo').textContent = 'FP ' + fmt(d.fp, 2) + ' · Desfase ~' + Math.round(deg) + '°';
    } else {
      $('phaseInfo').textContent = 'Sin carga';
    }

    lastData = d;
    renderBill();

    $('conn').textContent = d.sta ? 'Wi-Fi ' + d.ssid + ' · ' + d.ip : 'Red SmartVolt · ' + d.ap_ip;
    $('wifiInfo').textContent = (d.sta ? 'Conectado a "' + d.ssid + '" (' + d.ip + ', también http://smartvolt.local). '
      : (d.ssid ? 'Sin conexión a "' + d.ssid + '". ' : 'Sin red guardada. ')) + 'La red propia "SmartVolt" sigue activa.';
    if (document.activeElement !== $('avgN')) $('avgN').value = d.avg;
    $('foot').textContent = 'Media móvil de ' + d.avg + ' s · actualiza cada segundo';
  }

  let fails = 0, buzMan = false, ledMan = false;
  async function getJson(url, ms){
    const ac = window.AbortController ? new AbortController() : null;
    const t = ac ? setTimeout(() => ac.abort(), ms || 3000) : null;
    try {
      const r = await fetch(url, {cache:'no-store', signal: ac ? ac.signal : undefined});
      if (!r.ok) throw new Error('HTTP ' + r.status);
      return await r.json();
    } finally { if (t) clearTimeout(t); }
  }
  async function poll(){
    try {
      const d = await getJson('/api/data', 3000);
      if (!wavePaused){
        try { wave = await getJson('/api/wave', 3000); } catch(e){}
      }
      fails = 0; $('dot').className = 'dot ok';
      if (d.valid){
        offset = Date.now()/1000 - d.t;
        push(d.t, d.v, d.i, d.p);
        render(d); drawAll();
      } else {
        $('conn').textContent = (d.sta ? 'Wi-Fi ' + d.ssid + ' · ' + d.ip : 'Red SmartVolt · ' + d.ap_ip) + ' · esperando lectura…';
      }
    } catch(e){
      if (++fails > 2){ $('dot').className = 'dot bad'; $('conn').textContent = 'Sin conexión con el medidor'; }
    }
    setTimeout(poll, 1000);
  }

  async function loadHistory(){
    try {
      const h = await getJson('/api/history', 4000);
      for (const x of h) push(x[0], x[1], x[2], x[3]);
      drawAll();
    } catch(e){}
  }

  // ---------------- Configuración ----------------
  async function post(url, body){
    const r = await fetch(url, {method:'POST', headers:{'Content-Type':'application/x-www-form-urlencoded'},
      body: new URLSearchParams(body || {})});
    if (!r.ok) throw new Error((await r.json()).error || 'Error');
  }
  $('avgSave').onclick = async () => {
    try { await post('/api/avg', {n: $('avgN').value}); $('avgMsg').textContent = 'Guardado'; }
    catch(e){ $('avgMsg').textContent = e.message; }
  };
  $('limSave').onclick = async () => {
    try {
      await post('/api/limit', {limit: $('iLim').value, vmin: $('vMin').value, vmax: $('vMax').value, buz_en: $('buzEn').checked ? 1 : 0, led_en: $('ledEn').checked ? 1 : 0});
      $('limMsg').textContent = 'Guardado';
    } catch(e){ $('limMsg').textContent = e.message; }
  };
  const toggleOut = async (b, l) => {
    try {
      await post('/api/outputs', {buz: b ? 1 : 0, led: l ? 1 : 0});
      const r = await fetch('/api/data', {cache:'no-store'});
      render(await r.json());
      $('outMsg').textContent = '';
    } catch(e){ $('outMsg').textContent = e.message; }
  };
  $('buzBtn').onclick = () => toggleOut(!buzMan, ledMan);
  $('ledBtn').onclick = () => toggleOut(buzMan, !ledMan);
  $('eReset').onclick = async () => {
    if (!confirm('¿Poner la energía acumulada en 0 kWh?')) return;
    try { await post('/api/energy/reset'); $('eMsg').textContent = 'Energía en 0'; }
    catch(e){ $('eMsg').textContent = e.message; }
  };
  $('wSave').onclick = async () => {
    const ssid = $('ssid').value.trim();
    if (!ssid){ $('wMsg').textContent = 'Escribe el nombre de la red.'; return; }
    try {
      await post('/api/wifi', {ssid, pass: $('pass').value});
      $('wMsg').textContent = 'Conectando a "' + ssid + '"… En unos segundos la página estará en http://smartvolt.local (o en la IP que muestra la consola).';
      $('pass').value = '';
    } catch(e){ $('wMsg').textContent = e.message; }
  };

  loadHistory();
  poll();
})();
</script>
</body>
</html>
)rawliteral";

#endif // WEB_PAGE_H
