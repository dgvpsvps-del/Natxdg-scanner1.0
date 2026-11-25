// Dibuat otomatis oleh tools/gen_web.py dari web/index.html - jangan diedit manual
#pragma once
#include <Arduino.h>

const char INDEX_HTML[] PROGMEM = R"NXDG(<!doctype html>
<html lang="id">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1,viewport-fit=cover">
<meta name="theme-color" content="#0a101c">
<title>Nat X Dg Scanner</title>
<style>
:root{
  --bg:#0a101c;--card:#111a2b;--card2:#16213a;--bd:#22304c;--tx:#e8eefb;--mu:#8fa0bd;
  --acc1:#14b8f0;--acc2:#22c55e;
  --g:#22c55e;--l:#a3e635;--y:#facc15;--o:#fb923c;--r:#f87171;
  --sh:0 10px 30px rgba(0,0,0,.45);
}
@media (prefers-color-scheme:light){
  :root{--bg:#eef2f8;--card:#fff;--card2:#f4f7fc;--bd:#d9e1ee;--tx:#0e1b33;--mu:#5d6d89;--sh:0 10px 30px rgba(20,40,80,.18)}
}
*{box-sizing:border-box;-webkit-tap-highlight-color:transparent}
html,body{margin:0}
body{background:var(--bg);color:var(--tx);font:15px/1.4 system-ui,-apple-system,"Segoe UI",Roboto,sans-serif;
  padding:env(safe-area-inset-top) 16px calc(24px + env(safe-area-inset-bottom));min-height:100vh}
.wrap{max-width:720px;margin:0 auto}
button,select,input{font:inherit;color:inherit}
header{display:flex;align-items:center;gap:12px;padding:18px 0 14px}
.logo{background:#fff;border-radius:12px;padding:5px 8px;display:flex;box-shadow:0 2px 10px rgba(0,0,0,.25)}
.logo img{width:46px;height:auto;display:block}
.ttl{flex:1;min-width:0}
.ttl h1{margin:0;font-size:18px;letter-spacing:.2px;font-weight:800;white-space:nowrap;overflow:hidden;text-overflow:ellipsis}
.ttl p{margin:1px 0 0;color:var(--mu);font-size:12.5px;white-space:nowrap;overflow:hidden;text-overflow:ellipsis}
.pill{display:inline-flex;align-items:center;gap:7px;padding:6px 10px;border-radius:99px;background:var(--card);flex:none;
  border:1px solid var(--bd);font-size:12.5px;color:var(--mu);white-space:nowrap}
.pill i{width:8px;height:8px;border-radius:50%;background:var(--g)}
.pill.scan i{background:var(--acc1);animation:pulse 1s infinite}
.pill.err i{background:var(--r)}
@keyframes pulse{50%{opacity:.25;transform:scale(.7)}}
.stats{display:grid;grid-template-columns:repeat(3,1fr);gap:10px;margin-bottom:14px}
.stat{background:var(--card);border:1px solid var(--bd);border-radius:16px;padding:12px 13px;min-width:0}
.stat small{display:block;white-space:nowrap;color:var(--mu);font-size:11.5px;text-transform:uppercase;letter-spacing:.6px}
.stat b{display:block;font-size:24px;font-weight:800;margin-top:3px;font-variant-numeric:tabular-nums;white-space:nowrap}
.stat em{display:block;font-style:normal;font-size:12px;color:var(--mu);overflow:hidden;text-overflow:ellipsis;white-space:nowrap}
.tabs{display:flex;gap:6px;background:var(--card);border:1px solid var(--bd);border-radius:14px;padding:4px;margin-bottom:12px}
.tabs button{flex:1;border:0;background:none;padding:9px;border-radius:10px;font-weight:700;color:var(--mu);cursor:pointer}
.tabs button.on{background:linear-gradient(135deg,var(--acc1),var(--acc2));color:#04121f}
.bar{display:flex;flex-wrap:wrap;gap:8px;margin-bottom:12px}
.bar input[type=search]{flex:1 1 150px;min-width:0;background:var(--card);border:1px solid var(--bd);border-radius:12px;padding:10px 12px;outline:none}
.bar input[type=search]:focus,.bar select:focus{border-color:var(--acc1)}
.bar select{background:var(--card);border:1px solid var(--bd);border-radius:12px;padding:10px 10px}
.btn{border:0;border-radius:12px;padding:10px 15px;font-weight:800;cursor:pointer;background:linear-gradient(135deg,var(--acc1),var(--acc2));color:#04121f}
.btn:disabled{opacity:.5}
.btn.ghost{background:var(--card);color:var(--tx);border:1px solid var(--bd)}
.range{display:flex;align-items:center;gap:10px;background:var(--card);border:1px solid var(--bd);border-radius:12px;padding:8px 12px;width:100%;color:var(--mu);font-size:13px}
.range input{flex:1;accent-color:var(--acc1)}
.range b{color:var(--tx);min-width:62px;text-align:right;font-variant-numeric:tabular-nums}
.list{display:grid;gap:8px}
.row{display:flex;align-items:center;gap:12px;width:100%;text-align:left;background:var(--card);border:1px solid var(--bd);
  border-radius:16px;padding:11px 13px;cursor:pointer}
.row:active{background:var(--card2)}
.sig{display:flex;align-items:flex-end;gap:2px;height:22px;width:26px;flex:none}
.sig span{flex:1;background:var(--bd);border-radius:2px}
.sig span:nth-child(1){height:30%}.sig span:nth-child(2){height:55%}.sig span:nth-child(3){height:78%}.sig span:nth-child(4){height:100%}
.sig span.on{background:var(--c)}
.main{flex:1;min-width:0}
.ssid{font-weight:700;overflow:hidden;text-overflow:ellipsis;white-space:nowrap}
.ssid.h{color:var(--mu);font-style:italic;font-weight:500}
.meta{display:flex;gap:6px;flex-wrap:wrap;margin-top:3px;font-size:12px;color:var(--mu)}
.meta span{background:var(--card2);border-radius:6px;padding:1px 6px}
.meta .open{color:#1a1200;background:var(--y)}
.db{text-align:right;flex:none}
.db b{font-size:24px;font-weight:800;font-variant-numeric:tabular-nums;line-height:1}
.db small{display:block;color:var(--mu);font-size:11px;margin-top:2px}
.dl{font-size:11px;margin-left:4px}
.empty{text-align:center;color:var(--mu);padding:36px 10px;background:var(--card);border:1px dashed var(--bd);border-radius:16px}
.chart{background:var(--card);border:1px solid var(--bd);border-radius:18px;padding:16px 12px 12px}
.cols{display:grid;grid-template-columns:repeat(13,1fr);gap:4px;height:190px;align-items:end}
.col{display:flex;flex-direction:column;justify-content:flex-end;align-items:center;height:100%;gap:5px;min-width:0}
.col .b{width:100%;border-radius:6px 6px 3px 3px;min-height:3px;background:var(--c);transition:height .4s}
.col .n{font-size:11px;color:var(--mu);font-variant-numeric:tabular-nums}
.col .k{font-size:12px;font-weight:800}
.rec{margin-top:12px;padding:12px 14px;border-radius:14px;background:var(--card2);border:1px solid var(--bd);font-size:14px}
.rec b{color:var(--g)}
.hint{color:var(--mu);font-size:12.5px;margin:10px 2px 0}
footer{margin-top:20px;text-align:center;color:var(--mu);font-size:12px;line-height:1.6}
.sheet{position:fixed;inset:0;background:rgba(3,8,18,.62);display:none;align-items:flex-end;justify-content:center;z-index:9}
.sheet.on{display:flex}
.panel{width:100%;max-width:560px;background:var(--card);border:1px solid var(--bd);border-radius:24px 24px 0 0;padding:18px 18px calc(18px + env(safe-area-inset-bottom));
  box-shadow:var(--sh);max-height:92vh;overflow:auto;animation:up .22s ease-out}
@keyframes up{from{transform:translateY(30px);opacity:0}}
@media (min-width:640px){.sheet{align-items:center}.panel{border-radius:24px}}
.ph{display:flex;justify-content:space-between;gap:10px;align-items:flex-start}
.ph h2{margin:0;font-size:18px;word-break:break-word}
.x{border:0;background:var(--card2);color:var(--tx);width:34px;height:34px;border-radius:50%;font-size:18px;cursor:pointer;flex:none}
.big{display:flex;align-items:baseline;gap:8px;margin:12px 0 4px}
.big b{font-size:56px;line-height:1;font-weight:800;font-variant-numeric:tabular-nums}
.big span{color:var(--mu)}
.big em{margin-left:auto;font-style:normal;font-weight:800;padding:4px 10px;border-radius:99px;color:#04121f;background:var(--c)}
.q{height:9px;background:var(--card2);border-radius:9px;overflow:hidden;margin:8px 0 14px}
.q i{display:block;height:100%;border-radius:9px;background:var(--c);transition:width .4s}
.spark{background:var(--card2);border-radius:14px;padding:8px 6px 4px}
.spark svg{display:block;width:100%;height:96px}
.mm{display:grid;grid-template-columns:repeat(3,1fr);gap:8px;margin:10px 0}
.mm div{background:var(--card2);border-radius:12px;padding:8px 10px;text-align:center}
.mm small{display:block;color:var(--mu);font-size:11px}
.mm b{font-size:18px;font-variant-numeric:tabular-nums}
.kv{display:grid;grid-template-columns:auto 1fr;gap:6px 14px;font-size:14px;margin-top:4px}
.kv dt{color:var(--mu)}.kv dd{margin:0;text-align:right;word-break:break-all}
</style>
</head>
<body>
<div class="wrap">
  <header>
    <div class="logo"><img alt="DG" src="data:image/png;base64,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"></div>
    <div class="ttl"><h1>Nat X Dg Scanner</h1><p id="sub">Scanner WiFi offline</p></div>
    <div class="pill scan" id="pill"><i></i><span id="pillt">Memulai…</span></div>
  </header>

  <div class="stats">
    <div class="stat"><small>Jaringan</small><b id="sN">0</b><em id="sN2">&nbsp;</em></div>
    <div class="stat"><small>Terkuat</small><b id="sB">--</b><em id="sB2">&nbsp;</em></div>
    <div class="stat"><small>CH sepi</small><b id="sC">--</b><em id="sC2">&nbsp;</em></div>
  </div>

  <div class="tabs">
    <button id="tW" class="on">WiFi</button>
    <button id="tC">Channel</button>
  </div>

  <div class="bar">
    <button class="btn" id="bScan">Scan sekarang</button>
    <select id="auto" aria-label="Auto scan">
      <option value="0">Auto: mati</option>
      <option value="3000">Auto: 3 dtk</option>
      <option value="5000">Auto: 5 dtk</option>
      <option value="10000">Auto: 10 dtk</option>
    </select>
    <button class="btn ghost" id="bCsv">CSV</button>
  </div>

  <section id="vW">
    <div class="bar">
      <input type="search" id="q" placeholder="Cari nama / BSSID" autocomplete="off">
      <select id="sort" aria-label="Urutkan">
        <option value="rssi">Sinyal</option>
        <option value="name">Nama</option>
        <option value="ch">Channel</option>
      </select>
      <label class="range">Sembunyikan &lt;<input type="range" id="min" min="-100" max="-40" step="5" value="-100"><b id="minT">semua</b></label>
    </div>
    <div class="list" id="list"></div>
  </section>

  <section id="vC" hidden>
    <div class="chart"><div class="cols" id="cols"></div></div>
    <div class="rec" id="rec"></div>
    <p class="hint">Tinggi batang = seberapa ramai channel itu (sinyal kuat dan channel bertetangga ikut dihitung). Angka kecil = jumlah WiFi di channel tersebut.</p>
  </section>

  <footer>
    <div id="info">&nbsp;</div>
    <div>Offline penuh · tidak butuh internet · hanya membaca sinyal WiFi di sekitar</div>
  </footer>
</div>

<div class="sheet" id="sheet"><div class="panel" id="panel"></div></div>

<script>
"use strict";
const $=s=>document.querySelector(s);
const st={nets:[],hist:new Map(),scanning:false,id:-1,lastIngest:0,kick:0,err:0,
  interval:5000,sort:"rssi",min:-100,q:"",tab:"wifi",sel:null};
try{const o=JSON.parse(localStorage.getItem("nxdg")||"{}");
  if(o.interval!==undefined)st.interval=o.interval;if(o.sort)st.sort=o.sort;if(o.min)st.min=o.min;}catch(e){}
function save(){try{localStorage.setItem("nxdg",JSON.stringify({interval:st.interval,sort:st.sort,min:st.min}))}catch(e){}}

const esc=s=>String(s).replace(/[&<>"']/g,c=>({"&":"&amp;","<":"&lt;",">":"&gt;",'"':"&quot;","'":"&#39;"}[c]));
const clamp=(v,a,b)=>Math.max(a,Math.min(b,v));
const color=r=>r>=-50?"var(--g)":r>=-60?"var(--l)":r>=-70?"var(--y)":r>=-80?"var(--o)":"var(--r)";
const label=r=>r>=-50?"MANTAP":r>=-60?"BAGUS":r>=-70?"CUKUP":r>=-80?"LEMAH":"BURUK";
const qual=r=>clamp(Math.round((r+100)*100/60),0,100);
const nbars=r=>r>=-55?4:r>=-67?3:r>=-78?2:1;

async function api(path,ms){
  const c=new AbortController(),t=setTimeout(()=>c.abort(),ms||4000);
  try{const r=await fetch(path,{signal:c.signal,cache:"no-store"});if(!r.ok)throw 0;return await r.json()}
  finally{clearTimeout(t)}
}

function ingest(nets){
  const now=Date.now();
  nets.forEach(n=>{
    const h=st.hist.get(n.b)||[];
    n.d=h.length?n.r-h[h.length-1].r:0;
    h.push({t:now,r:n.r});if(h.length>90)h.shift();
    st.hist.set(n.b,h);
  });
  st.nets=nets;st.lastIngest=now;
}

async function tick(){
  try{
    const r=await api("/api/results");
    st.err=0;st.scanning=!!r.scanning;
    if(r.id!==st.id){st.id=r.id;if(r.nets&&r.id>0)ingest(r.nets)}
    if(!st.scanning&&st.interval>0&&Date.now()-st.kick>=st.interval)kick();
  }catch(e){st.err++}
  render();
}
async function kick(){
  if(st.scanning&&Date.now()-st.kick<9000)return;
  st.kick=Date.now();st.scanning=true;render();
  try{await api("/api/scan")}catch(e){}
}

function view(){
  let a=st.nets.filter(n=>n.r>=st.min);
  const q=st.q.trim().toLowerCase();
  if(q)a=a.filter(n=>n.s.toLowerCase().includes(q)||n.b.toLowerCase().includes(q));
  if(st.sort==="name")a.sort((x,y)=>(x.s||"~").localeCompare(y.s||"~"));
  else if(st.sort==="ch")a.sort((x,y)=>x.c-y.c||y.r-x.r);
  else a.sort((x,y)=>y.r-x.r);
  return a;
}

function chanStats(){
  const sc=Array(14).fill(0),cn=Array(14).fill(0),best=Array(14).fill(-200);
  st.nets.forEach(n=>{
    if(n.c<1||n.c>13)return;
    cn[n.c]++;if(n.r>best[n.c])best[n.c]=n.r;
    for(let k=1;k<=13;k++){const d=Math.abs(k-n.c);if(d<=4)sc[k]+=(1-d/5)*qual(n.r)/100}
  });
  return{sc,cn,best};
}

function render(){
  // status
  const pill=$("#pill");
  if(st.err>=2){pill.className="pill err";$("#pillt").textContent="Terputus…"}
  else if(st.scanning){pill.className="pill scan";$("#pillt").textContent="Scan…"}
  else{pill.className="pill";const s=st.lastIngest?Math.round((Date.now()-st.lastIngest)/1000):0;
    $("#pillt").textContent=st.lastIngest?(s<2?"Baru saja":s+" dtk lalu"):"Menunggu"}
  $("#bScan").disabled=st.scanning;

  // stats
  const n=st.nets,cs=chanStats();
  $("#sN").textContent=n.length;
  $("#sN2").textContent=n.filter(x=>!x.s).length+" tersembunyi";
  if(n.length){const b=n.reduce((a,x)=>x.r>a.r?x:a,n[0]);
    $("#sB").textContent=b.r;$("#sB").style.color=color(b.r);
    $("#sB2").textContent=b.s||"<tersembunyi>";
    let bestCh=1,bs=1e9;[1,6,11].forEach(k=>{if(cs.sc[k]<bs){bs=cs.sc[k];bestCh=k}});
    $("#sC").textContent="CH "+bestCh;$("#sC2").textContent="dari 1 / 6 / 11";
  }else{$("#sB").textContent="--";$("#sB2").innerHTML="&nbsp;";$("#sC").textContent="--";$("#sC2").innerHTML="&nbsp;"}

  if(st.tab==="wifi")renderList();else renderChan(cs);
  if(st.sel)renderSheet();
}

function renderList(){
  const a=view(),el=$("#list");
  if(!a.length){el.innerHTML='<div class="empty">'+(st.nets.length?"Tidak ada WiFi yang cocok dengan filter.":(st.id<1?"Sedang scan WiFi…":"Tidak ada WiFi terdeteksi."))+"</div>";return}
  el.innerHTML=a.map(n=>{
    const c=color(n.r),nb=nbars(n.r);
    const dl=n.d?'<span class="dl" style="color:'+(n.d>0?"var(--g)":"var(--r)")+'">'+(n.d>0?"▲":"▼")+Math.abs(n.d)+"</span>":"";
    return '<button class="row" data-b="'+esc(n.b)+'" style="--c:'+c+'">'
     +'<div class="sig">'+[1,2,3,4].map(i=>'<span class="'+(i<=nb?"on":"")+'"></span>').join("")+"</div>"
     +'<div class="main"><div class="ssid'+(n.s?"":" h")+'">'+(n.s?esc(n.s):"&lt;tersembunyi&gt;")+"</div>"
     +'<div class="meta"><span>CH '+n.c+'</span><span class="'+(n.e==="OPEN"?"open":"")+'">'+esc(n.e)+"</span></div></div>"
     +'<div class="db"><b style="color:'+c+'">'+n.r+dl+"</b><small>dBm</small></div></button>";
  }).join("");
}

function renderChan(cs){
  let mx=Math.max(1.2,...cs.sc.slice(1));
  let h="";
  for(let k=1;k<=13;k++){
    const v=cs.sc[k],p=Math.round(v/mx*100);
    const c=v<0.8?"var(--g)":v<2?"var(--y)":"var(--r)";
    h+='<div class="col" style="--c:'+c+'"><span class="n">'+(cs.cn[k]?cs.best[k]:"")+'</span><div class="b" style="height:'+Math.max(2,p)+'%"></div><span class="k">'+k+'</span><span class="n">'+(cs.cn[k]||"·")+"</span></div>";
  }
  $("#cols").innerHTML=h;
  if(!st.nets.length){$("#rec").textContent="Menunggu hasil scan…";return}
  let bestCh=1,bs=1e9;[1,6,11].forEach(k=>{if(cs.sc[k]<bs){bs=cs.sc[k];bestCh=k}});
  let any=1,as=1e9;for(let k=1;k<=13;k++)if(cs.sc[k]<as){as=cs.sc[k];any=k}
  $("#rec").innerHTML="Untuk router 2.4 GHz, pilih <b>channel "+bestCh+"</b> (paling sepi di antara 1, 6, 11)."
    +(any!==bestCh?" Channel paling kosong secara umum: "+any+".":"");
}

function spark(h){
  const W=300,H=90,lo=-100,hi=-30,X=i=>h.length<2?W/2:i*(W-8)/(h.length-1)+4,Y=r=>H-6-(clamp(r,lo,hi)-lo)/(hi-lo)*(H-12);
  let g="";[-50,-70,-90].forEach(v=>{g+='<line x1="0" x2="'+W+'" y1="'+Y(v)+'" y2="'+Y(v)+'" stroke="currentColor" stroke-opacity=".12"/><text x="3" y="'+(Y(v)-2)+'" font-size="8" fill="currentColor" fill-opacity=".5">'+v+"</text>"});
  const pts=h.map((p,i)=>X(i).toFixed(1)+","+Y(p.r).toFixed(1)).join(" ");
  const last=h[h.length-1];
  return '<svg viewBox="0 0 '+W+" "+H+'" preserveAspectRatio="none" style="color:var(--mu)">'+g
   +(h.length>1?'<polyline points="'+pts+'" fill="none" stroke="var(--c)" stroke-width="2.4" stroke-linejoin="round" stroke-linecap="round" vector-effect="non-scaling-stroke"/>':"")
   +'<circle cx="'+X(h.length-1)+'" cy="'+Y(last.r)+'" r="3.5" fill="var(--c)"/></svg>';
}

function renderSheet(){
  const n=st.nets.find(x=>x.b===st.sel),h=st.hist.get(st.sel)||[];
  const p=$("#panel");
  if(!n){p.innerHTML='<div class="ph"><h2>Tidak terlihat</h2><button class="x" id="xc">×</button></div><p style="color:var(--mu)">WiFi ini tidak muncul di scan terakhir.</p>';return}
  const rs=h.map(x=>x.r),mn=Math.min(...rs),mx=Math.max(...rs),av=Math.round(rs.reduce((a,b)=>a+b,0)/rs.length);
  const c=color(n.r);
  p.style.setProperty("--c",c);
  p.innerHTML='<div class="ph"><h2>'+(n.s?esc(n.s):"&lt;tersembunyi&gt;")+'</h2><button class="x" id="xc" aria-label="Tutup">×</button></div>'
   +'<div class="big"><b style="color:'+c+'">'+n.r+"</b><span>dBm</span><em>"+label(n.r)+"</em></div>"
   +'<div class="q"><i style="width:'+qual(n.r)+'%"></i></div>'
   +'<div class="spark">'+spark(h)+"</div>"
   +'<div class="mm"><div><small>MIN</small><b>'+mn+"</b></div><div><small>RATA-RATA</small><b>"+av+"</b></div><div><small>MAX</small><b>"+mx+"</b></div></div>"
   +'<dl class="kv"><dt>Kualitas</dt><dd>'+qual(n.r)+"%</dd><dt>Channel</dt><dd>"+n.c+"</dd><dt>Keamanan</dt><dd>"+esc(n.e)+"</dd><dt>BSSID</dt><dd>"+esc(n.b)+"</dd><dt>Sampel</dt><dd>"+h.length+" pembacaan</dd></dl>";
}

function openSheet(b){st.sel=b;$("#sheet").classList.add("on");renderSheet();if(!st.scanning)kick()}
function closeSheet(){st.sel=null;$("#sheet").classList.remove("on")}

function csv(){
  const rows=[["ssid","bssid","dbm","channel","keamanan"]].concat(view().map(n=>[n.s,n.b,n.r,n.c,n.e]));
  const t=rows.map(r=>r.map(v=>'"'+String(v).replace(/"/g,'""')+'"').join(",")).join("\n");
  const a=document.createElement("a");a.href=URL.createObjectURL(new Blob(["﻿"+t],{type:"text/csv"}));
  a.download="natxdg-wifi-"+new Date().toISOString().slice(0,19).replace(/[:T]/g,"-")+".csv";a.click();
}

async function info(){
  try{const i=await api("/api/info");
    const u=i.up,t=u>=3600?Math.floor(u/3600)+" j "+Math.floor(u%3600/60)+" m":Math.floor(u/60)+" m "+(u%60)+" d";
    $("#sub").textContent=i.chip+" · v"+i.ver;
    $("#info").textContent="Aktif "+t+" · RAM bebas "+Math.round(i.heap/1024)+" KB · "+i.cl+" perangkat tersambung";
  }catch(e){}
}

// events
$("#list").addEventListener("click",e=>{const r=e.target.closest(".row");if(r)openSheet(r.dataset.b)});
$("#sheet").addEventListener("click",e=>{if(e.target.id==="sheet"||e.target.closest("#xc"))closeSheet()});
document.addEventListener("keydown",e=>{if(e.key==="Escape")closeSheet()});
$("#bScan").onclick=kick;$("#bCsv").onclick=csv;
$("#auto").onchange=e=>{st.interval=+e.target.value;save();if(st.interval)kick()};
$("#sort").onchange=e=>{st.sort=e.target.value;save();render()};
$("#q").oninput=e=>{st.q=e.target.value;render()};
$("#min").oninput=e=>{st.min=+e.target.value;$("#minT").textContent=st.min<=-100?"semua":st.min+" dBm";save();render()};
function tab(t){st.tab=t;$("#tW").classList.toggle("on",t==="wifi");$("#tC").classList.toggle("on",t==="ch");
  $("#vW").hidden=t!=="wifi";$("#vC").hidden=t!=="ch";render()}
$("#tW").onclick=()=>tab("wifi");$("#tC").onclick=()=>tab("ch");

$("#auto").value=String(st.interval);$("#sort").value=st.sort;$("#min").value=st.min;
$("#minT").textContent=st.min<=-100?"semua":st.min+" dBm";
render();tick();setInterval(tick,1000);info();setInterval(info,10000);
</script>
</body>
</html>
)NXDG";
