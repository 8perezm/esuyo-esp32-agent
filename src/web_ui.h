#pragma once
// web_ui.h — PROGMEM HTML pages. Agents: SKIP reading this file unless editing UI (large blobs, no logic).
#include <Arduino.h>

static const char CHAT_HTML[] PROGMEM = R"rawliteral(
<!doctype html><html><head><meta charset="utf-8"/>
<meta name="viewport" content="width=device-width,initial-scale=1"/>
<title>Agent — Chat</title>
<link rel="preconnect" href="https://fonts.googleapis.com">
<link rel="preconnect" href="https://fonts.gstatic.com" crossorigin>
<link href="https://fonts.googleapis.com/css2?family=Inter:wght@400;500;600;700&display=swap" rel="stylesheet">
<style>
:root{color-scheme:light dark}body{font-family:'Inter',system-ui,-apple-system,sans-serif;margin:0;display:flex;flex-direction:column;height:100vh}
a{text-decoration:none}a:hover{text-decoration:underline}
header{display:flex;justify-content:space-between;align-items:center;padding:10px 14px;border-bottom:1px solid #8884}
header h1{font-size:16px;margin:0;line-height:1.2}nav{white-space:nowrap}nav a{margin-left:10px;font-size:13px;text-decoration:none;line-height:1.2}
#warn{background:#7a2e00;color:#fff;padding:8px 14px;font-size:13px;display:none}
#warn a{color:#fff;text-decoration:none;font-weight:600}
#log{flex:1;overflow-y:auto;padding:14px;display:flex;flex-direction:column;gap:10px;max-width:800px;width:100%;margin:0 auto;box-sizing:border-box}
.msg{padding:10px 12px;border-radius:12px;max-width:85%;white-space:pre-wrap;word-break:break-word;font-size:14px;line-height:1.45}
.user{background:#0b6cff;color:#fff;align-self:flex-end}
.bot{background:none;border:0;padding:4px 2px;align-self:flex-start;max-width:100%}
.sys{font-size:12px;opacity:.7;align-self:center}
.wrap{align-self:flex-start;max-width:100%;width:100%;display:flex;flex-direction:column;gap:4px}
details.think{font-size:12px;opacity:.85;border:1px solid #8883;border-radius:8px;padding:6px 10px;max-width:85%}
details.think summary{cursor:pointer;font-weight:600;font-size:12px}
.thinkbody{white-space:pre-wrap;word-break:break-word;border-left:2px solid #8886;padding-left:8px;margin-top:6px;font-weight:400}
#composer{position:relative;max-width:832px;width:100%;margin:0 auto;padding:0 16px 16px;box-sizing:border-box}
#card{border:1px solid #8885;border-radius:14px;background:Canvas;box-shadow:0 1px 4px #0002;transition:box-shadow .15s}
#card:focus-within{box-shadow:0 0 0 2px #0b6cff44}
#in{width:100%;box-sizing:border-box;resize:none;background:transparent;border:0;outline:0;font:inherit;font-size:14px;line-height:1.5;min-height:44px;max-height:200px;padding:12px 16px 8px}
#toolbar{display:flex;align-items:center;justify-content:space-between;padding:0 12px 10px;gap:8px}
#pillwrap{position:relative;min-width:0}
#mpill{display:flex;align-items:center;gap:6px;border:0;background:#8882;border-radius:999px;padding:6px 12px;font:inherit;font-size:12px;font-weight:500;cursor:pointer;max-width:40vw;color:inherit}
@media(min-width:1024px){#mpill{max-width:280px}}
#mpill:hover{background:#8883}
#mpilltxt{overflow:hidden;text-overflow:ellipsis;white-space:nowrap}
#mpill svg{flex:none;transition:transform .15s}
#mpill.open svg{transform:rotate(180deg)}
#sendwrap{display:flex;align-items:center;gap:8px;flex:none}
#sendhint{font-size:11px;opacity:.55}
#send{width:32px;height:32px;border-radius:50%;border:0;display:flex;align-items:center;justify-content:center;background:#0b6cff;color:#fff;opacity:.55;cursor:not-allowed;padding:0}
#send.ready{background:#0b6cff;color:#fff;opacity:1;cursor:pointer}
#popwrap{position:relative}
#mpop{position:absolute;left:0;bottom:8px;width:min(430px,100%);max-height:350px;display:none;flex-direction:column;background:Canvas;border:1px solid #8885;border-radius:14px;box-shadow:0 12px 40px #0007;z-index:20;overflow:hidden}
#mpop.open{display:flex}
#msearchwrap{padding:10px;border-bottom:1px solid #8883}
#msearch{width:100%;box-sizing:border-box;border:1.5px solid #9dbcf3;border-radius:10px;padding:8px 12px;font:inherit;font-size:13px;background:#8881;outline:none;color:inherit}
#mgroups{overflow-y:auto;padding:4px 4px 8px}
.msec{font-size:11px;font-weight:700;opacity:.5;padding:10px 12px 4px;letter-spacing:.05em}
.mrow{display:flex;align-items:center;gap:8px;padding:8px 12px;border-radius:8px;cursor:pointer}
.mrow:hover{background:#0b6cff1c}
.mrow .mt{flex:1;min-width:0}
.mrow .mn{font-size:13px;font-weight:500;overflow:hidden;text-overflow:ellipsis;white-space:nowrap}
.mrow .mp{font-size:11px;opacity:.6;margin-top:1px}
.mstar{flex:none;border:0;background:none;cursor:pointer;font-size:15px;opacity:.45;padding:2px 6px;color:inherit;font-family:inherit}
.mstar.on{opacity:1;color:#f5a623}
.mstar:before{content:'☆'}
.mstar.on:before{content:'★'}
#status{font-size:12px;opacity:.7;padding:0 14px 8px;text-align:center}
.cursor::after{content:'▍';animation:blink 0.8s infinite}
@keyframes blink{50%{opacity:0}}
header nav{display:flex;align-items:center}
#themebtn{display:inline-flex;align-items:center;justify-content:center;width:30px;height:30px;margin-left:10px;border:1px solid #8884;border-radius:8px;background:transparent;color:inherit;cursor:pointer;padding:0;flex:none}
#themebtn:hover{background:#8882}
#themebtn svg{width:16px;height:16px}
#themebtn .sun{display:none}
html[data-theme="dark"] #themebtn .sun{display:block}
html[data-theme="dark"] #themebtn .moon{display:none}
html[data-theme="light"]{color-scheme:light}
html[data-theme="dark"]{color-scheme:dark}
html[data-theme="light"] body{background:#fff;color:#111}
html[data-theme="dark"] body{background:#0f141b;color:#e6e8ee}
html[data-theme="dark"] a{color:#8ab4ff}
html[data-theme="dark"] #card,html[data-theme="dark"] #mpop{background:#161c26;border-color:#ffffff22}
html[data-theme="dark"] #msearch{background:#1a2230;border-color:#ffffff2a;color:#e6e8ee}
</style>
<script>try{var t=localStorage.getItem('agent_theme');if(!t){t=matchMedia('(prefers-color-scheme:dark)').matches?'dark':'light'}document.documentElement.dataset.theme=t}catch(e){}</script></head><body>
<header><h1><a href="/" style="color:inherit">Agent</a></h1><nav><a href="/">Chat</a><a href="/functions">Functions</a><a href="/docs">Docs</a><a href="/setup">Setup</a><a href="/update">Firmware</a><button id="themebtn" type="button" aria-label="Toggle dark mode" title="Toggle dark mode"><svg class="moon" xmlns="http://www.w3.org/2000/svg" width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M12 3a6 6 0 0 0 9 9 9 9 0 1 1-9-9Z"/></svg><svg class="sun" xmlns="http://www.w3.org/2000/svg" width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><circle cx="12" cy="12" r="4"/><path d="M12 2v2"/><path d="M12 20v2"/><path d="m4.93 4.93 1.41 1.41"/><path d="m17.66 17.66 1.41 1.41"/><path d="M2 12h2"/><path d="M20 12h2"/><path d="m6.34 17.66-1.41 1.41"/><path d="m19.07 4.93-1.41 1.41"/></svg></button></nav></header>
<div id="warn"></div>
<div id="log"><div class="sys">Connected. Ask anything.</div></div>
<div id="status"></div>
<div id="composer">
  <div id="popwrap"><div id="mpop">
    <div id="msearchwrap"><input id="msearch" placeholder="Search models…" autocomplete="off" spellcheck="false"/></div>
    <div id="mgroups"></div>
  </div></div>
  <div id="card">
    <textarea id="in" rows="1" placeholder="Message…"></textarea>
    <div id="toolbar">
      <div id="pillwrap"><button id="mpill" type="button"><span id="mpilltxt">Select model</span><svg xmlns="http://www.w3.org/2000/svg" width="12" height="12" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="m6 9 6 6 6-6"></path></svg></button></div>
      <div id="sendwrap"><span id="sendhint">⏎ to send</span><button id="send" type="button" disabled title="Send message"><svg xmlns="http://www.w3.org/2000/svg" width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M14.536 21.686a.5.5 0 0 0 .937-.024l6.5-19a.496.496 0 0 0-.635-.635l-19 6.5a.5.5 0 0 0-.024.937l7.93 3.18a2 2 0 0 1 1.112 1.11z"></path><path d="m21.854 2.147-10.94 10.939"></path></svg></button></div>
    </div>
  </div>
</div>
<script>
const log=document.getElementById('log'),inp=document.getElementById('in'),
send=document.getElementById('send'),warn=document.getElementById('warn'),st=document.getElementById('status');
let history=[];
fetch('/api/status').then(r=>r.json()).then(s=>{
  let m=[];
  if(!s.wifi_connected)m.push('WiFi not connected — <a href="/wifi">open WiFi setup</a>.');
  if(!s.endpoint_configured)m.push('Agent endpoint/API key missing — <a href="/agent">open Setup</a>.');
  if(m.length){warn.style.display='block';warn.innerHTML=m.join('<br/>');}
  if(s.model)setPill(s.model,true);
  window._model=s.model||'';
  st.textContent=(s.ip||'')+' • '+(s.model||'')+' • '+(s.free_heap?Math.round(s.free_heap/1024)+'KB heap':'');
}).catch(()=>{});
function add(text,cls){const d=document.createElement('div');d.className='msg '+cls;d.textContent=text;log.appendChild(d);log.scrollTop=log.scrollHeight;return d;}
function typeInto(el,full){el.classList.add('cursor');let i=0;const step=Math.max(1,Math.round(full.length/120));
  return new Promise(done=>{const t=setInterval(()=>{i+=step;el.textContent=full.slice(0,i);log.scrollTop=log.scrollHeight;
    if(i>=full.length){clearInterval(t);el.classList.remove('cursor');done();}},18);});}
function addAssistant(){
  const wrap=document.createElement('div');wrap.className='wrap';
  const det=document.createElement('details');det.className='think';det.open=true;det.style.display='none';
  const sum=document.createElement('summary');sum.textContent='Thinking';
  const tb=document.createElement('div');tb.className='thinkbody';
  det.appendChild(sum);det.appendChild(tb);
  const b=document.createElement('div');b.className='msg bot';
  wrap.appendChild(det);wrap.appendChild(b);log.appendChild(wrap);log.scrollTop=log.scrollHeight;
  return {wrap:b,det:det,think:tb,sum:sum};
}
async function streamReply(t,ui){
  const el=ui.wrap,det=ui.det,think=ui.think;
  const r=await fetch('/api/chat-stream',{method:'POST',headers:{'Content-Type':'application/json'},
    body:JSON.stringify({message:t,history:history.slice(-10)})});
  const ct=(r.headers.get('content-type')||'');
  if(!r.ok||!r.body||ct.indexOf('text/event-stream')<0){
    let err='stream failed';
    try{const j=await r.json();err=j.error||err;}catch(e){}
    throw new Error(err);
  }
  const reader=r.body.getReader(),dec=new TextDecoder();
  let buf='',full='',th='',needTools=false;
  el.classList.add('cursor');
  for(;;){
    const {done,value}=await reader.read();
    if(done)break;
    buf+=dec.decode(value,{stream:true});
    let idx;
    while((idx=buf.indexOf('\n\n'))>=0){
      const evt=buf.slice(0,idx);buf=buf.slice(idx+2);
      for(const raw of evt.split('\n')){
        const line=raw.trim();
        if(line.slice(0,5)!=='data:')continue;
        const p=line.slice(5).trim();
        if(p==='[DONE]'){el.classList.remove('cursor');return {reply:full,reasoning:th,needTools:needTools};}
        try{const o=JSON.parse(p);
          if(o.need_tools)needTools=true;
          if(o.delta){full+=o.delta;el.textContent=full;log.scrollTop=log.scrollHeight;}
          if(o.reasoning){th+=o.reasoning;det.style.display='';think.textContent=th;log.scrollTop=log.scrollHeight;}
        }catch(e){}
      }
    }
  }
  el.classList.remove('cursor');
  return {reply:full,reasoning:th,needTools:needTools};
}
async function go(){
  const t=inp.value.trim();if(!t)return;inp.value='';inp.style.height='auto';send.disabled=true;send.classList.remove('ready');
  add(t,'user');const ui=addAssistant();
  async function agentFallback(note){
    const r=await fetch('/api/chat',{method:'POST',headers:{'Content-Type':'application/json'},
      body:JSON.stringify({message:t,history:history.slice(-10)})});
    const j=await r.json();
    if(!r.ok){ui.wrap.textContent='Error: '+(j.error||r.status);}
    else{
      if(j.reasoning){
        ui.det.style.display='';
        ui.think.textContent=j.reasoning;
        ui.sum.textContent='Thinking ('+j.reasoning.length+' chars)';
        // ui.det is already in place (first child of the assistant container) — no DOM move needed.
      } else ui.det.remove();
      let txt=j.reply||'(empty reply)';
      if(j.tools_used)txt='⚙️ '+j.tools_used.trim()+'\n'+txt;
      if(note&&!j.tools_used)txt='⚙️ tools\n'+txt;
      await typeInto(ui.wrap,txt);
      history.push({role:'user',content:t},{role:'assistant',content:j.reply||''});}
  }
  try{
    try{
      const res=await streamReply(t,ui);
      if(res.needTools||(!res.reply&&!res.reasoning)){ui.wrap.textContent='⚙️ running tools…';await agentFallback(true);return;}
      if(!res.reasoning)ui.det.remove();
      else ui.sum.textContent='Thinking ('+res.reasoning.length+' chars)';
      if(res.reply)history.push({role:'user',content:t},{role:'assistant',content:res.reply});
    }catch(se){
      // Fallback: classic non-streaming request + typewriter (full agent loop with tools).
      ui.det.remove();
      await agentFallback(false);
    }
  }catch(e){ui.wrap.textContent='Fetch failed: '+e;}
  syncSend();inp.focus();log.scrollTop=log.scrollHeight;
}
send.onclick=go;
function syncSend(){const has=inp.value.trim().length>0;send.disabled=!has;send.classList.toggle('ready',has);}
inp.addEventListener('input',()=>{inp.style.height='auto';inp.style.height=Math.min(inp.scrollHeight,200)+'px';syncSend();});
inp.onkeydown=e=>{if(e.key==='Enter'&&!e.shiftKey){e.preventDefault();go();}};
syncSend();
// --- model picker popup: search + STARRED + MODELS (list proxied from your endpoint via /api/models; ESP stores your pick) ---
const mpill=document.getElementById('mpill'),mpilltxt=document.getElementById('mpilltxt'),
mpop=document.getElementById('mpop'),msearch=document.getElementById('msearch'),mgroups=document.getElementById('mgroups');
let MODELS=[];
let modelsLoaded=false;
let STARRED=[];
try{STARRED=JSON.parse(localStorage.getItem('agent_starred')||localStorage.getItem('or_starred')||'[]');if(!Array.isArray(STARRED))STARRED=[];}catch(e){STARRED=[];}
function saveStarred(){try{localStorage.setItem('agent_starred',JSON.stringify(STARRED));}catch(e){}}
function prettyId(id){
  const p=id.split('/');
  const cap=s=>s.replace(/[-_]+/g,' ').replace(/\b\w/g,c=>c.toUpperCase());
  return p.length>1?cap(p[0])+': '+cap(p.slice(1).join('/')):cap(id);
}
function friendlyName(id){
  const f=MODELS.find(m=>m[0]===id);
  if(f&&f[1]&&f[1]!==id)return f[1];
  return prettyId(id);
}
function setPill(id,silent){
  mpilltxt.textContent=friendlyName(id||'Select model');
  if(!silent){window._model=id||'';}
}
async function loadModels(){
  if(modelsLoaded)return;modelsLoaded=true;
  try{const r=await fetch('/api/models');const j=await r.json();
    const list=j.models||[];
    if(list.length){MODELS=list.map(m=>[(m.id||m),'']);setPill(window._model,true);}
    else if(window._model)MODELS=[[window._model,'']];
  }catch(e){if(window._model)MODELS=[[window._model,'']];}
}
function matchQ(m,toks){const h=(m[0]+' '+(m[1]||'')).toLowerCase();return toks.every(t=>h.includes(t));}
function rowEl(m){
  const d=document.createElement('div');d.className='mrow';
  const t=document.createElement('div');t.className='mt';
  const n=document.createElement('div');n.className='mn';n.textContent=(m[1]&&m[1]!==m[0])?m[1]:m[0];
  t.appendChild(n);
  if(m[1]&&m[1]!==m[0]){const p=document.createElement('div');p.className='mp';p.textContent=m[0];t.appendChild(p);}
  d.appendChild(t);
  const on=STARRED.includes(m[0]);
  const s=document.createElement('button');s.className='mstar'+(on?' on':'');s.type='button';s.title='Star';
  s.onmousedown=e=>{e.preventDefault();e.stopPropagation();
    const i=STARRED.indexOf(m[0]);
    if(i>=0)STARRED.splice(i,1);else STARRED.push(m[0]);
    saveStarred();renderPopup();};
  d.appendChild(s);
  d.onmousedown=e=>{e.preventDefault();pickModel(m[0]);};
  return d;
}
function section(label,rows){
  if(!rows.length)return;
  const h=document.createElement('div');h.className='msec';h.textContent=label;mgroups.appendChild(h);
  for(const m of rows)mgroups.appendChild(rowEl(m));
}
function renderPopup(){
  const toks=msearch.value.toLowerCase().trim().split(/\s+/).filter(Boolean);
  mgroups.innerHTML='';
  const star=MODELS.filter(m=>STARRED.includes(m[0])&&(toks.length?matchQ(m,toks):true)).slice(0,25);
  const main=MODELS.filter(m=>!STARRED.includes(m[0])&&(!toks.length||matchQ(m,toks))).slice(0,60);
  section('STARRED',star);
  section('MODELS',main);
  if(!star.length&&!main.length){
    const e=document.createElement('div');e.className='msec';e.textContent=toks.length?'No matches':'Loading models…';
    mgroups.appendChild(e);
  }
}
function openPopup(){
  mpop.classList.add('open');mpill.classList.add('open');
  renderPopup();
  loadModels().then(()=>{if(mpop.classList.contains('open'))renderPopup();});
  setTimeout(()=>msearch.focus(),30);
}
function closePopup(){mpop.classList.remove('open');mpill.classList.remove('open');}
async function pickModel(id){
  closePopup();setPill(id);
  st.textContent='Saving model…';
  try{const r=await fetch('/api/agent',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({model:id})});
    const j=await r.json();
    if(j.ok)st.textContent='Model saved (no reboot)';
    else st.textContent='Save failed: '+(j.error||r.status);
  }catch(e){st.textContent='Save failed: '+e;}
}
mpill.addEventListener('click',e=>{e.stopPropagation();mpop.classList.contains('open')?closePopup():openPopup();});
msearch.addEventListener('input',renderPopup);
msearch.addEventListener('keydown',e=>{
  if(e.key==='Enter'){e.preventDefault();const f=mgroups.querySelector('.mrow');if(f)f.dispatchEvent(new Event('mousedown'));}
  if(e.key==='Escape')closePopup();});
document.addEventListener('click',e=>{if(!document.getElementById('composer').contains(e.target))closePopup();});
document.addEventListener('keydown',e=>{if(e.key==='Escape')closePopup();});
</script><script>(function(){var b=document.getElementById('themebtn');if(!b)return;b.addEventListener('click',function(){var h=document.documentElement;var n=h.dataset.theme==='dark'?'light':'dark';h.dataset.theme=n;try{localStorage.setItem('agent_theme',n)}catch(e){}});})();</script></body></html>
)rawliteral";

static const char SETUP_HTML[] PROGMEM = R"rawliteral(
<!doctype html><html><head><meta charset="utf-8"/>
<meta name="viewport" content="width=device-width,initial-scale=1"/>
<title>Agent — Setup</title>
<link rel="preconnect" href="https://fonts.googleapis.com">
<link rel="preconnect" href="https://fonts.gstatic.com" crossorigin>
<link href="https://fonts.googleapis.com/css2?family=Inter:wght@400;500;600;700&display=swap" rel="stylesheet">
<style>:root{color-scheme:light dark}body{font-family:'Inter',system-ui,-apple-system,sans-serif;margin:0}
a{text-decoration:none}
.card{border:1px solid #8884;border-radius:12px;padding:14px;margin-top:14px}
a.btn{display:block;text-align:center;margin-top:8px;padding:12px;border-radius:8px;background:#0b6cff;color:#fff;font-weight:700;text-decoration:none}
a.ghost{background:#444}
header{display:flex;justify-content:space-between;align-items:center;padding:10px 14px;border-bottom:1px solid #8884}
header h1{font-size:16px;margin:0;line-height:1.2}nav{white-space:nowrap}nav a{margin-left:10px;font-size:13px;line-height:1.2}
.wrap{max-width:800px;margin:0 auto;padding:0 16px 24px}
header nav{display:flex;align-items:center}
#themebtn{display:inline-flex;align-items:center;justify-content:center;width:30px;height:30px;margin-left:10px;border:1px solid #8884;border-radius:8px;background:transparent;color:inherit;cursor:pointer;padding:0;flex:none}
#themebtn:hover{background:#8882}
#themebtn svg{width:16px;height:16px}
#themebtn .sun{display:none}
html[data-theme="dark"] #themebtn .sun{display:block}
html[data-theme="dark"] #themebtn .moon{display:none}
html[data-theme="light"]{color-scheme:light}
html[data-theme="dark"]{color-scheme:dark}
html[data-theme="light"] body{background:#fff;color:#111}
html[data-theme="dark"] body{background:#0f141b;color:#e6e8ee}
html[data-theme="dark"] a{color:#8ab4ff}
html[data-theme="dark"] .card{background:#161c26;border-color:#ffffff22}</style>
<script>try{var t=localStorage.getItem('agent_theme');if(!t){t=matchMedia('(prefers-color-scheme:dark)').matches?'dark':'light'}document.documentElement.dataset.theme=t}catch(e){}</script></head><body>
<header><h1><a href="/" style="color:inherit">Agent</a></h1><nav><a href="/">Chat</a><a href="/functions">Functions</a><a href="/docs">Docs</a><a href="/setup">Setup</a><a href="/update">Firmware</a><button id="themebtn" type="button" aria-label="Toggle dark mode" title="Toggle dark mode"><svg class="moon" xmlns="http://www.w3.org/2000/svg" width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M12 3a6 6 0 0 0 9 9 9 9 0 1 1-9-9Z"/></svg><svg class="sun" xmlns="http://www.w3.org/2000/svg" width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><circle cx="12" cy="12" r="4"/><path d="M12 2v2"/><path d="M12 20v2"/><path d="m4.93 4.93 1.41 1.41"/><path d="m17.66 17.66 1.41 1.41"/><path d="M2 12h2"/><path d="M20 12h2"/><path d="m6.34 17.66-1.41 1.41"/><path d="m19.07 4.93-1.41 1.41"/></svg></button></nav></header>
<div class="wrap">
<h2>Agent — Setup</h2>
<p><a href="/wifi">WiFi</a> · <a href="/agent">Endpoint</a> · <a href="/api/status" target="_blank">Status</a></p>
<div class="card"><h3>Step 1 — WiFi</h3><p>Pick hotspot / router + password.</p><a class="btn" href="/wifi">Open WiFi settings</a></div>
<div class="card"><h3>Step 2 — Agent endpoint</h3><p>OpenAI-compatible base URL + API key + model. Saving here never touches WiFi.</p><a class="btn" href="/agent">Open Agent settings</a></div>
</div><script>(function(){var b=document.getElementById('themebtn');if(!b)return;b.addEventListener('click',function(){var h=document.documentElement;var n=h.dataset.theme==='dark'?'light':'dark';h.dataset.theme=n;try{localStorage.setItem('agent_theme',n)}catch(e){}});})();</script></body></html>
)rawliteral";

static const char WIFI_HTML[] PROGMEM = R"rawliteral(
<!doctype html><html><head><meta charset="utf-8"/>
<meta name="viewport" content="width=device-width,initial-scale=1"/>
<title>Agent — WiFi</title>
<link rel="preconnect" href="https://fonts.googleapis.com">
<link rel="preconnect" href="https://fonts.gstatic.com" crossorigin>
<link href="https://fonts.googleapis.com/css2?family=Inter:wght@400;500;600;700&display=swap" rel="stylesheet">
<style>:root{color-scheme:light dark}body{font-family:'Inter',system-ui,-apple-system,sans-serif;margin:0}
a{text-decoration:none}
label{display:block;margin:12px 0 4px;font-weight:600;font-size:13px}
input,select{width:100%;padding:10px;border-radius:8px;border:1px solid #8888;font-size:14px;box-sizing:border-box;font-family:inherit}
button{margin-top:8px;padding:10px 14px;border-radius:8px;border:0;background:#0b6cff;color:#fff;font-weight:700;cursor:pointer;font-family:inherit}
.row{display:flex;gap:8px}.row button{flex:1}.card{border:1px solid #8884;border-radius:12px;padding:14px;margin-top:14px}
small{opacity:.7}#msg{margin-top:10px;font-size:13px}
header{display:flex;justify-content:space-between;align-items:center;padding:10px 14px;border-bottom:1px solid #8884}
header h1{font-size:16px;margin:0;line-height:1.2}nav{white-space:nowrap}nav a{margin-left:10px;font-size:13px;line-height:1.2}
.wrap{max-width:800px;margin:0 auto;padding:0 16px 24px}
header nav{display:flex;align-items:center}
#themebtn{display:inline-flex;align-items:center;justify-content:center;width:30px;height:30px;margin-left:10px;border:1px solid #8884;border-radius:8px;background:transparent;color:inherit;cursor:pointer;padding:0;flex:none;margin-top:0}
#themebtn:hover{background:#8882}
#themebtn svg{width:16px;height:16px}
#themebtn .sun{display:none}
html[data-theme="dark"] #themebtn .sun{display:block}
html[data-theme="dark"] #themebtn .moon{display:none}
html[data-theme="light"]{color-scheme:light}
html[data-theme="dark"]{color-scheme:dark}
html[data-theme="light"] body{background:#fff;color:#111}
html[data-theme="dark"] body{background:#0f141b;color:#e6e8ee}
html[data-theme="dark"] a{color:#8ab4ff}
html[data-theme="dark"] .card{background:#161c26;border-color:#ffffff22}
html[data-theme="dark"] input,html[data-theme="dark"] select{background:#1a2230;border-color:#ffffff2a;color:#e6e8ee}
html[data-theme="dark"] code{background:#ffffff1a}
html[data-theme="dark"] pre{background:#ffffff0d!important;border-color:#ffffff22}</style>
<script>try{var t=localStorage.getItem('agent_theme');if(!t){t=matchMedia('(prefers-color-scheme:dark)').matches?'dark':'light'}document.documentElement.dataset.theme=t}catch(e){}</script></head><body>
<header><h1><a href="/" style="color:inherit">Agent</a></h1><nav><a href="/">Chat</a><a href="/functions">Functions</a><a href="/docs">Docs</a><a href="/setup">Setup</a><a href="/update">Firmware</a><button id="themebtn" type="button" aria-label="Toggle dark mode" title="Toggle dark mode"><svg class="moon" xmlns="http://www.w3.org/2000/svg" width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M12 3a6 6 0 0 0 9 9 9 9 0 1 1-9-9Z"/></svg><svg class="sun" xmlns="http://www.w3.org/2000/svg" width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><circle cx="12" cy="12" r="4"/><path d="M12 2v2"/><path d="M12 20v2"/><path d="m4.93 4.93 1.41 1.41"/><path d="m17.66 17.66 1.41 1.41"/><path d="M2 12h2"/><path d="M20 12h2"/><path d="m6.34 17.66-1.41 1.41"/><path d="m19.07 4.93-1.41 1.41"/></svg></button></nav></header>
<div class="wrap">
<h2>WiFi setup</h2>
<p><a href="/setup">Setup hub</a> · <a href="/agent">Endpoint →</a></p>
<div class="card">
<label>Networks</label><div class="row"><select id="ssid"></select><button id="scan" type="button">Scan</button></div>
<label>Password</label><input id="pass" type="password" placeholder="WiFi password (blank = keep saved)" autocomplete="current-password"/>
<small>Device reboots and joins on Save. Password left blank keeps the saved one.</small></div>
<button id="save">Save WiFi &amp; Reboot</button>
<div id="msg"></div>
<script>
const $=id=>document.getElementById(id),msg=$('msg');
let savedSsid='';
async function load(){try{const s=await(await fetch('/api/wifi')).json();savedSsid=s.wifi_ssid||'';
  if(savedSsid){msg.textContent='Saved network: '+savedSsid+' (leave password blank to keep it)';}}catch(e){}scan();}
async function scan(){$('scan').disabled=true;const sel=$('ssid');sel.innerHTML='<option>Scanning…</option>';
  try{const j=await(await fetch('/api/scan')).json();sel.innerHTML='';
    if(savedSsid){const o=document.createElement('option');o.value=savedSsid;o.text=savedSsid+' (saved)';sel.appendChild(o);}
    (j.networks||[]).forEach(n=>{if(n.ssid===savedSsid)return;const o=document.createElement('option');o.value=n.ssid;o.text=n.ssid+' ('+n.rssi+'dBm'+(n.secure?', secured':'')+')';sel.appendChild(o);});
    if(!sel.options.length)sel.innerHTML='<option value="">(no networks found)</option>';
  }catch(e){sel.innerHTML='<option>scan failed</option>';}$('scan').disabled=false;}
$('scan').onclick=scan;
$('save').onclick=async()=>{msg.textContent='Saving…';
  const b={ssid:$('ssid').value,password:$('pass').value};
  const r=await fetch('/api/wifi',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify(b)});
  const j=await r.json();msg.textContent=j.ok?'Saved "'+j.ssid+'". Rebooting — rejoin your normal WiFi, then open http://esp32-agent.local or the new IP.':'Error: '+(j.error||r.status);};
load();
</script><script>(function(){var b=document.getElementById('themebtn');if(!b)return;b.addEventListener('click',function(){var h=document.documentElement;var n=h.dataset.theme==='dark'?'light':'dark';h.dataset.theme=n;try{localStorage.setItem('agent_theme',n)}catch(e){}});})();</script></div></body></html>
)rawliteral";

static const char AGENT_HTML[] PROGMEM = R"rawliteral(
<!doctype html><html><head><meta charset="utf-8"/>
<meta name="viewport" content="width=device-width,initial-scale=1"/>
<title>Agent — Setup</title>
<link rel="preconnect" href="https://fonts.googleapis.com">
<link rel="preconnect" href="https://fonts.gstatic.com" crossorigin>
<link href="https://fonts.googleapis.com/css2?family=Inter:wght@400;500;600;700&display=swap" rel="stylesheet">
<style>:root{color-scheme:light dark}body{font-family:'Inter',system-ui,-apple-system,sans-serif;margin:0}
a{text-decoration:none}
label{display:block;margin:12px 0 4px;font-weight:600;font-size:13px}
input{width:100%;padding:10px;border-radius:8px;border:1px solid #8888;font-size:14px;box-sizing:border-box;font-family:inherit}
button{margin-top:12px;padding:10px 14px;border-radius:8px;border:0;background:#0b6cff;color:#fff;font-weight:700;cursor:pointer;font-family:inherit}
.card{border:1px solid #8884;border-radius:12px;padding:14px;margin-top:14px}
small{opacity:.7}#msg{margin-top:10px;font-size:13px}
header{display:flex;justify-content:space-between;align-items:center;padding:10px 14px;border-bottom:1px solid #8884}
header h1{font-size:16px;margin:0;line-height:1.2}nav{white-space:nowrap}nav a{margin-left:10px;font-size:13px;line-height:1.2}
.wrap{max-width:800px;margin:0 auto;padding:0 16px 24px}
header nav{display:flex;align-items:center}
#themebtn{display:inline-flex;align-items:center;justify-content:center;width:30px;height:30px;margin-left:10px;border:1px solid #8884;border-radius:8px;background:transparent;color:inherit;cursor:pointer;padding:0;flex:none;margin-top:0}
#themebtn:hover{background:#8882}
#themebtn svg{width:16px;height:16px}
#themebtn .sun{display:none}
html[data-theme="dark"] #themebtn .sun{display:block}
html[data-theme="dark"] #themebtn .moon{display:none}
html[data-theme="light"]{color-scheme:light}
html[data-theme="dark"]{color-scheme:dark}
html[data-theme="light"] body{background:#fff;color:#111}
html[data-theme="dark"] body{background:#0f141b;color:#e6e8ee}
html[data-theme="dark"] a{color:#8ab4ff}
html[data-theme="dark"] .card{background:#161c26;border-color:#ffffff22}
html[data-theme="dark"] input,html[data-theme="dark"] select{background:#1a2230;border-color:#ffffff2a;color:#e6e8ee}
html[data-theme="dark"] code{background:#ffffff1a}
html[data-theme="dark"] pre{background:#ffffff0d!important;border-color:#ffffff22}</style>
<script>try{var t=localStorage.getItem('agent_theme');if(!t){t=matchMedia('(prefers-color-scheme:dark)').matches?'dark':'light'}document.documentElement.dataset.theme=t}catch(e){}</script></head><body>
<header><h1><a href="/" style="color:inherit">Agent</a></h1><nav><a href="/">Chat</a><a href="/functions">Functions</a><a href="/docs">Docs</a><a href="/setup">Setup</a><a href="/update">Firmware</a><button id="themebtn" type="button" aria-label="Toggle dark mode" title="Toggle dark mode"><svg class="moon" xmlns="http://www.w3.org/2000/svg" width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M12 3a6 6 0 0 0 9 9 9 9 0 1 1-9-9Z"/></svg><svg class="sun" xmlns="http://www.w3.org/2000/svg" width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><circle cx="12" cy="12" r="4"/><path d="M12 2v2"/><path d="M12 20v2"/><path d="m4.93 4.93 1.41 1.41"/><path d="m17.66 17.66 1.41 1.41"/><path d="M2 12h2"/><path d="M20 12h2"/><path d="m6.34 17.66-1.41 1.41"/><path d="m19.07 4.93-1.41 1.41"/></svg></button></nav></header>
<div class="wrap">
<h2>Agent endpoint</h2>
<p><a href="/setup">Setup hub</a> · <a href="/wifi">WiFi</a></p>
<div class="card">
<label>Base URL (OpenAI-compatible) <span style="color:#c00">*</span></label><input id="endpoint" placeholder="https://api.openai.com/v1" autocomplete="url"/>
<small>Base URL only — <code>/chat/completions</code> is appended automatically. Works with OpenAI, Ollama, LM Studio, vLLM, etc. Saving here never touches WiFi, no reboot. <b>Required on first setup.</b></small>
<label>API key (optional)</label><input id="key" type="password" placeholder="sk-… (blank = keep saved)" autocomplete="current-password"/>
<small>Not needed for keyless local endpoints. Blank keeps the saved key.</small>
<label>Model (exact model ID, optional)</label><input id="model" placeholder="e.g. llama3.1, gpt-4o-mini" autocomplete="off" spellcheck="false"/>
<small>Exact ID your endpoint serves. You can also switch models from the list in the chat composer.</small>
</div>
<button id="save">Save agent settings (no reboot)</button>
<div id="msg"></div>
<script>
const $=id=>document.getElementById(id),msg=$('msg');
async function load(){try{const s=await(await fetch('/api/agent')).json();
  $('endpoint').value=s.endpoint||'';$('model').value=s.model||'deepseek/deepseek-v4.1-flash';
  if(s.api_key_masked)msg.textContent='Saved key: '+s.api_key_masked+' (leave blank to keep)';
}catch(e){}
load();}
$('save').onclick=async()=>{msg.textContent='Saving…';
  const b={endpoint:$('endpoint').value,apiKey:$('key').value,model:$('model').value};
  const r=await fetch('/api/agent',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify(b)});
  const j=await r.json();msg.textContent=j.ok?'Saved. No reboot — go to <a href="/">Chat</a>.':'Error: '+(j.error||r.status);};
load();
</script><script>(function(){var b=document.getElementById('themebtn');if(!b)return;b.addEventListener('click',function(){var h=document.documentElement;var n=h.dataset.theme==='dark'?'light':'dark';h.dataset.theme=n;try{localStorage.setItem('agent_theme',n)}catch(e){}});})();</script></div></body></html>
)rawliteral";

static const char UPDATE_HTML[] PROGMEM = R"rawliteral(
<!doctype html><html><head><meta charset="utf-8"/><meta name="viewport" content="width=device-width,initial-scale=1"/>
<title>Agent — Firmware update</title>
<link rel="preconnect" href="https://fonts.googleapis.com">
<link rel="preconnect" href="https://fonts.gstatic.com" crossorigin>
<link href="https://fonts.googleapis.com/css2?family=Inter:wght@400;500;600;700&display=swap" rel="stylesheet">
<style>:root{color-scheme:light dark}body{font-family:'Inter',system-ui,-apple-system,sans-serif;margin:0}
a{text-decoration:none}
header{display:flex;justify-content:space-between;align-items:center;padding:10px 14px;border-bottom:1px solid #8884}
header h1{font-size:16px;margin:0;line-height:1.2}nav{white-space:nowrap}nav a{margin-left:10px;font-size:13px;line-height:1.2}
.wrap{max-width:800px;margin:0 auto;padding:0 16px 24px}
header nav{display:flex;align-items:center}
#themebtn{display:inline-flex;align-items:center;justify-content:center;width:30px;height:30px;margin-left:10px;border:1px solid #8884;border-radius:8px;background:transparent;color:inherit;cursor:pointer;padding:0;flex:none;margin-top:0}
#themebtn:hover{background:#8882}
#themebtn svg{width:16px;height:16px}
#themebtn .sun{display:none}
html[data-theme="dark"] #themebtn .sun{display:block}
html[data-theme="dark"] #themebtn .moon{display:none}
html[data-theme="light"]{color-scheme:light}
html[data-theme="dark"]{color-scheme:dark}
html[data-theme="light"] body{background:#fff;color:#111}
html[data-theme="dark"] body{background:#0f141b;color:#e6e8ee}
html[data-theme="dark"] a{color:#8ab4ff}
html[data-theme="dark"] .card{background:#161c26;border-color:#ffffff22}
html[data-theme="dark"] input,html[data-theme="dark"] select{background:#1a2230;border-color:#ffffff2a;color:#e6e8ee}
html[data-theme="dark"] code{background:#ffffff1a}
html[data-theme="dark"] pre{background:#ffffff0d!important;border-color:#ffffff22}</style>
<script>try{var t=localStorage.getItem('agent_theme');if(!t){t=matchMedia('(prefers-color-scheme:dark)').matches?'dark':'light'}document.documentElement.dataset.theme=t}catch(e){}</script>
</head><body>
<header><h1><a href="/" style="color:inherit">Agent</a></h1><nav><a href="/">Chat</a><a href="/functions">Functions</a><a href="/docs">Docs</a><a href="/setup">Setup</a><a href="/update">Firmware</a><button id="themebtn" type="button" aria-label="Toggle dark mode" title="Toggle dark mode"><svg class="moon" xmlns="http://www.w3.org/2000/svg" width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M12 3a6 6 0 0 0 9 9 9 9 0 1 1-9-9Z"/></svg><svg class="sun" xmlns="http://www.w3.org/2000/svg" width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><circle cx="12" cy="12" r="4"/><path d="M12 2v2"/><path d="M12 20v2"/><path d="m4.93 4.93 1.41 1.41"/><path d="m17.66 17.66 1.41 1.41"/><path d="M2 12h2"/><path d="M20 12h2"/><path d="m6.34 17.66-1.41 1.41"/><path d="m19.07 4.93-1.41 1.41"/></svg></button></nav></header>
<div class="wrap">
<h2>Firmware update (OTA, no cable)</h2>
<form method="POST" action="/update" enctype="multipart/form-data">
<input type="file" name="firmware" accept=".bin" required/>
<button type="submit">Upload .bin</button></form>
<p><small>Build with <code>pio run</code> then upload <code>.pio/build/esp32dev/firmware.bin</code> here.
Or use ArduinoOTA: <code>pio run --target upload --upload-port &lt;ESP_IP&gt;</code></small></p>
</div><script>(function(){var b=document.getElementById('themebtn');if(!b)return;b.addEventListener('click',function(){var h=document.documentElement;var n=h.dataset.theme==='dark'?'light':'dark';h.dataset.theme=n;try{localStorage.setItem('agent_theme',n)}catch(e){}});})();</script></body></html>
)rawliteral";

static const char FUNCS_HTML[] PROGMEM = R"rawliteral(
<!doctype html><html><head><meta charset="utf-8"/><meta name="viewport" content="width=device-width,initial-scale=1"/>
<title>Agent — Functions</title>
<link rel="preconnect" href="https://fonts.googleapis.com">
<link rel="preconnect" href="https://fonts.gstatic.com" crossorigin>
<link href="https://fonts.googleapis.com/css2?family=Inter:wght@400;500;600;700&display=swap" rel="stylesheet">
<style>:root{color-scheme:light dark}body{font-family:'Inter',system-ui,-apple-system,sans-serif;margin:0}
a{text-decoration:none}.card{border:1px solid #8884;border-radius:12px;padding:14px;margin-top:12px}
label{display:flex;gap:10px;align-items:flex-start;padding:8px 0;border-top:1px solid #8882;font-size:14px}
small{opacity:.7}button{margin-top:12px;padding:10px 14px;border-radius:8px;border:0;background:#0b6cff;color:#fff;font-weight:700;cursor:pointer;font-family:inherit}
#msg{margin-top:10px;font-size:13px}
header{display:flex;justify-content:space-between;align-items:center;padding:10px 14px;border-bottom:1px solid #8884}
header h1{font-size:16px;margin:0;line-height:1.2}nav{white-space:nowrap}nav a{margin-left:10px;font-size:13px;line-height:1.2}
.wrap{max-width:800px;margin:0 auto;padding:0 16px 24px}
header nav{display:flex;align-items:center}
#themebtn{display:inline-flex;align-items:center;justify-content:center;width:30px;height:30px;margin-left:10px;border:1px solid #8884;border-radius:8px;background:transparent;color:inherit;cursor:pointer;padding:0;flex:none;margin-top:0}
#themebtn:hover{background:#8882}
#themebtn svg{width:16px;height:16px}
#themebtn .sun{display:none}
html[data-theme="dark"] #themebtn .sun{display:block}
html[data-theme="dark"] #themebtn .moon{display:none}
html[data-theme="light"]{color-scheme:light}
html[data-theme="dark"]{color-scheme:dark}
html[data-theme="light"] body{background:#fff;color:#111}
html[data-theme="dark"] body{background:#0f141b;color:#e6e8ee}
html[data-theme="dark"] a{color:#8ab4ff}
html[data-theme="dark"] .card{background:#161c26;border-color:#ffffff22}
html[data-theme="dark"] input,html[data-theme="dark"] select{background:#1a2230;border-color:#ffffff2a;color:#e6e8ee}
html[data-theme="dark"] code{background:#ffffff1a}
html[data-theme="dark"] pre{background:#ffffff0d!important;border-color:#ffffff22}</style>
<script>try{var t=localStorage.getItem('agent_theme');if(!t){t=matchMedia('(prefers-color-scheme:dark)').matches?'dark':'light'}document.documentElement.dataset.theme=t}catch(e){}</script></head><body>
<header><h1><a href="/" style="color:inherit">Agent</a></h1><nav><a href="/">Chat</a><a href="/functions">Functions</a><a href="/docs">Docs</a><a href="/setup">Setup</a><a href="/update">Firmware</a><button id="themebtn" type="button" aria-label="Toggle dark mode" title="Toggle dark mode"><svg class="moon" xmlns="http://www.w3.org/2000/svg" width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M12 3a6 6 0 0 0 9 9 9 9 0 1 1-9-9Z"/></svg><svg class="sun" xmlns="http://www.w3.org/2000/svg" width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><circle cx="12" cy="12" r="4"/><path d="M12 2v2"/><path d="M12 20v2"/><path d="m4.93 4.93 1.41 1.41"/><path d="m17.66 17.66 1.41 1.41"/><path d="M2 12h2"/><path d="M20 12h2"/><path d="m6.34 17.66-1.41 1.41"/><path d="m19.07 4.93-1.41 1.41"/></svg></button></nav></header>
<div class="wrap">
<h2>Device functions</h2>
<p><a href="/docs">How to use →</a> · <a href="/api/tools" target="_blank">JSON</a> · <a href="/api/memory" target="_blank">Memory</a></p>
<p><small>Only enabled functions are sent to the model — fewer tokens, cheaper + faster. Untick what you don't use (e.g. restart).</small></p>
<div class="card" id="list">Loading…</div>
<button id="save">Save selection</button> <button id="all" type="button">All</button> <button id="none" type="button">None</button>
<div id="msg"></div>
<div class="card"><h3 style="margin:0 0 6px">Memory</h3>
<p style="margin:0 0 8px"><small>Durable facts (<code>mem_save</code>) + recent chat log. Clearing takes effect immediately, no reboot.</small></p>
<pre id="mem" style="white-space:pre-wrap;word-break:break-word;font-size:12px;opacity:.85">(loading…)</pre>
<button id="clrMem" type="button">Clear memory</button> <button id="clrLog" type="button">Clear chat log</button>
<div id="memmsg" style="margin-top:8px;font-size:13px"></div></div>
<script>
const DESCR={get_status:'Heap, uptime, WiFi, IP',gpio_write:'Digital out (safe pins)',gpio_read:'Digital in',adc_read:'Analog (32,33,34,35,36,39)',wifi_scan:'Scan networks (~2s)',mem_save:'Save a memory fact',mem_read:'Read memory facts',restart_device:'Restart (needs confirm)',blink:'Blink a pin N times',web_fetch:'Fetch a web page as text'};
async function load(){const j=await(await fetch('/api/tools')).json();
  const box=document.getElementById('list');box.innerHTML='';
  (j.tools||[]).forEach(t=>{const l=document.createElement('label');
    l.innerHTML='<input type="checkbox" '+(t.enabled?'checked':'')+' data-n="'+t.name+'"/><span><b>'+t.name+'</b><br/><small>'+(DESCR[t.name]||'')+'</small></span>';
    box.appendChild(l);});}
document.getElementById('all').onclick=()=>document.querySelectorAll('input[data-n]').forEach(c=>c.checked=true);
document.getElementById('none').onclick=()=>document.querySelectorAll('input[data-n]').forEach(c=>c.checked=false);
document.getElementById('save').onclick=async()=>{const en=[...document.querySelectorAll('input[data-n]:checked')].map(c=>c.dataset.n);
  const r=await fetch('/api/tools',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({enabled:en})});
  const j=await r.json();document.getElementById('msg').textContent=j.ok?'Saved '+en.length+' enabled. System prompt now lists only these.':'Error '+(j.error||r.status);};
async function loadMem(){try{const j=await(await fetch('/api/memory')).json();
  document.getElementById('mem').textContent=(j.memory||'(empty)')+'\n— '+(j.log_bytes||0)+'B chat log';}catch(e){document.getElementById('mem').textContent='(load failed)';}};
async function clearMem(kind){const mm=document.getElementById('memmsg');
  const what=kind==='clear_memory'?'saved memory facts':'chat log';
  if(!confirm('Clear '+what+'? This cannot be undone.'))return;
  mm.textContent='Clearing…';
  const b={};b[kind]=true;
  try{const r=await fetch('/api/memory',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify(b)});
  const j=await r.json();mm.textContent=j.ok?('Cleared '+what+'.'):('Error '+(j.error||r.status));loadMem();}catch(e){mm.textContent='Failed: '+e;}};
document.getElementById('clrMem').onclick=()=>clearMem('clear_memory');
document.getElementById('clrLog').onclick=()=>clearMem('clear_log');
load();
loadMem();
</script><script>(function(){var b=document.getElementById('themebtn');if(!b)return;b.addEventListener('click',function(){var h=document.documentElement;var n=h.dataset.theme==='dark'?'light':'dark';h.dataset.theme=n;try{localStorage.setItem('agent_theme',n)}catch(e){}});})();</script></div></body></html>
)rawliteral";

static const char DOCS_HTML[] PROGMEM = R"rawliteral(
<!doctype html><html><head><meta charset="utf-8"/><meta name="viewport" content="width=device-width,initial-scale=1"/>
<title>Agent — Docs</title>
<link rel="preconnect" href="https://fonts.googleapis.com">
<link rel="preconnect" href="https://fonts.gstatic.com" crossorigin>
<link href="https://fonts.googleapis.com/css2?family=Inter:wght@400;500;600;700&display=swap" rel="stylesheet">
<style>:root{color-scheme:light dark}body{font-family:'Inter',system-ui,-apple-system,sans-serif;margin:0}
a{text-decoration:none}.card{border:1px solid #8884;border-radius:12px;padding:14px;margin-top:12px}
small{opacity:.7}code{background:#8882;padding:1px 6px;border-radius:6px;font-size:12px}
pre{background:#8881;border:1px solid #8883;border-radius:10px;padding:10px;overflow-x:auto;font-size:12px;white-space:pre-wrap;word-break:break-word}
header{display:flex;justify-content:space-between;align-items:center;padding:10px 14px;border-bottom:1px solid #8884}
header h1{font-size:16px;margin:0;line-height:1.2}nav{white-space:nowrap}nav a{margin-left:10px;font-size:13px;line-height:1.2}
.wrap{max-width:800px;margin:0 auto;padding:0 16px 24px}ul,ol{line-height:1.7;font-size:14px}h3{margin:22px 0 6px}p{font-size:14px;line-height:1.6}table{border-collapse:collapse;width:100%;font-size:13px}td,th{border:1px solid #8884;padding:6px 8px;text-align:left;vertical-align:top}
header nav{display:flex;align-items:center}
#themebtn{display:inline-flex;align-items:center;justify-content:center;width:30px;height:30px;margin-left:10px;border:1px solid #8884;border-radius:8px;background:transparent;color:inherit;cursor:pointer;padding:0;flex:none;margin-top:0}
#themebtn:hover{background:#8882}
#themebtn svg{width:16px;height:16px}
#themebtn .sun{display:none}
html[data-theme="dark"] #themebtn .sun{display:block}
html[data-theme="dark"] #themebtn .moon{display:none}
html[data-theme="light"]{color-scheme:light}
html[data-theme="dark"]{color-scheme:dark}
html[data-theme="light"] body{background:#fff;color:#111}
html[data-theme="dark"] body{background:#0f141b;color:#e6e8ee}
html[data-theme="dark"] a{color:#8ab4ff}
html[data-theme="dark"] .card{background:#161c26;border-color:#ffffff22}
html[data-theme="dark"] input,html[data-theme="dark"] select{background:#1a2230;border-color:#ffffff2a;color:#e6e8ee}
html[data-theme="dark"] code{background:#ffffff1a}
html[data-theme="dark"] pre{background:#ffffff0d!important;border-color:#ffffff22}
html[data-theme="dark"] td,html[data-theme="dark"] th{border-color:#ffffff22}</style>
<script>try{var t=localStorage.getItem('agent_theme');if(!t){t=matchMedia('(prefers-color-scheme:dark)').matches?'dark':'light'}document.documentElement.dataset.theme=t}catch(e){}</script></head><body>
<header><h1><a href="/" style="color:inherit">Agent</a></h1><nav><a href="/">Chat</a><a href="/functions">Functions</a><a href="/docs">Docs</a><a href="/setup">Setup</a><a href="/update">Firmware</a><button id="themebtn" type="button" aria-label="Toggle dark mode" title="Toggle dark mode"><svg class="moon" xmlns="http://www.w3.org/2000/svg" width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M12 3a6 6 0 0 0 9 9 9 9 0 1 1-9-9Z"/></svg><svg class="sun" xmlns="http://www.w3.org/2000/svg" width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><circle cx="12" cy="12" r="4"/><path d="M12 2v2"/><path d="M12 20v2"/><path d="m4.93 4.93 1.41 1.41"/><path d="m17.66 17.66 1.41 1.41"/><path d="M2 12h2"/><path d="M20 12h2"/><path d="m6.34 17.66-1.41 1.41"/><path d="m19.07 4.93-1.41 1.41"/></svg></button></nav></header>
<div class="wrap">
<h2>ESP32 Agent — what it is</h2>
<p>A tiny on-device assistant. The chat page at <a href="/">/</a> forwards your message to any OpenAI-compatible <code>/chat/completions</code> endpoint (OpenAI, Ollama, LM Studio, vLLM…), and the model can call back into this ESP32 to <b>read pins, blink LEDs, scan WiFi, remember facts, fetch web pages, or restart</b>. Replies stream live; anything starting with <code>⚙️</code> means tools ran first.</p>
<div class="card"><h3>Start in 30 seconds</h3>
<ol><li>Open <a href="/">Chat</a> and ask <code>are you online? what is your heap and wifi?</code></li>
<li>Open <a href="/functions">Functions</a> — tick only what you need (fewer tools = cheaper + faster).</li>
<li>Read the cookbook below and steal a prompt.</li></ol></div>
<h3>What the app can do</h3>
<ul>
<li><b>Chat with tools</b> (<a href="/">/</a>) — streaming + non-streaming fallback, last turns kept as context.</li>
<li><b>10 device functions</b> — status, GPIO, ADC, blink, WiFi scan, memory save/read, restart, web fetch. Enable/disable at <a href="/functions">Functions</a>.</li>
<li><b>Memory</b> — durable facts (<code>mem_save</code>) survive reboots; recent chat turns persist on LittleFS and are re-injected as context.</li>
<li><b>Web research</b> — <code>web_fetch</code> pulls an http(s) page as text so the model can summarize fresh info.</li>
<li><b>Setup &amp; OTA</b> — <a href="/setup">Setup</a>, <a href="/wifi">WiFi</a>, <a href="/agent">Endpoint</a>, <a href="/update">Firmware</a>, <a href="/api/status" target="_blank">Status JSON</a>.</li>
</ul>
<h3>How to ask (so tools fire)</h3>
<ul>
<li>Be concrete: pin numbers, counts, URLs. <code>blink pin 2 five times</code> beats <code>blink a bit</code>.</li>
<li>Name the thing: <code>read analog pin 34</code> → <code>adc_read</code>; <code>is pin 13 high?</code> → <code>gpio_read</code>.</li>
<li>Paste full URLs with <code>https://</code> for research: <code>summarize https://example.com/specs</code>.</li>
<li>One job per message works best; the agent can still chain up to 3 tool rounds.</li>
<li>If the reply starts <code>⚙️ blink</code>, that lists the tools that ran — then comes the summary.</li>
</ul>
<h3>Functions reference</h3>
<table>
<tr><th>Function</th><th>Does</th><th>Try saying</th></tr>
<tr><td><code>get_status</code></td><td>Heap, uptime, WiFi SSID/RSSI, IP, model.</td><td><code>are you online? heap and wifi?</code><br/><code>what model are you running?</code></td></tr>
<tr><td><code>gpio_write</code></td><td>Set an output pin HIGH/LOW. Safe pins: 2,4,5,12-19,21-23,25-27,32,33.</td><td><code>set pin 2 high</code><br/><code>turn pin 13 off</code></td></tr>
<tr><td><code>gpio_read</code></td><td>Read a digital pin (never guess — it measures).</td><td><code>is pin 13 high?</code><br/><code>read pins 12 and 14</code></td></tr>
<tr><td><code>adc_read</code></td><td>Analog read. Best on 32,33,34,35,36,39.</td><td><code>read analog pin 34</code><br/><code>what voltage on pin 32?</code></td></tr>
<tr><td><code>blink</code></td><td>Blink a pin N times with real timing, ends LOW (max 20 blinks, ≤15s). Prefer over repeated <code>gpio_write</code> — raw toggles are too fast to see.</td><td><code>blink pin 2 five times</code><br/><code>blink pin 13 ten times slowly</code></td></tr>
<tr><td><code>wifi_scan</code></td><td>Scan nearby networks (~2s, top 10).</td><td><code>what wifi networks do you see?</code><br/><code>is my hotspot nearby?</code></td></tr>
<tr><td><code>mem_save</code></td><td>Save one durable fact (≤500 chars).</td><td><code>remember I like slow blinks</code><br/><code>remember the lab sensor is on pin 34</code></td></tr>
<tr><td><code>mem_read</code></td><td>Read saved facts.</td><td><code>what do you remember about me?</code></td></tr>
<tr><td><code>restart_device</code></td><td>Reboot. Only on explicit ask; needs confirmation.</td><td><code>restart yourself (confirm)</code></td></tr>
<tr><td><code>web_fetch</code></td><td>Fetch an http(s) URL as text (~2000 chars, 15s timeout, follows redirects, no JS/login).</td><td><code>summarize https://example.com</code><br/><code>fetch https://…/price and tell me the number</code></td></tr>
</table>
<h3>Web research guide</h3>
<div class="card">
<p><code>web_fetch</code> lets the agent look things up instead of guessing: prices, docs, headlines, changelogs. It strips pages to plain text and hands ~2000 characters to the model.</p>
<ul>
<li>Always include the scheme: <code>https://site.com/page</code>, not <code>site.com/page</code>.</li>
<li>Prefer stable, mostly-text pages. Heavy JS apps and login walls return little.</li>
<li>Ask for the extraction: <code>fetch … and list the specs</code>, <code>fetch … and summarize in 3 bullets</code>.</li>
<li>Compare: <code>fetch A and B, which board has more RAM?</code> (two fetches, one summary).</li>
</ul>
<pre>You: fetch https://example.com/esp32-boards and summarize the RAM options
ESP: ⚙️ web_fetch
3 boards listed: 320KB / 520KB / 8MB PSRAM — cheapest with 520KB is…</pre>
</div>
<h3>Prompt cookbook</h3>
<div class="card">
<pre>You: what is your heap, wifi and IP?
ESP: ⚙️ get_status
Heap 218KB, up 2h, WiFi LAB (-61dBm), 192.168.5.44.

You: blink pin 2 five times
ESP: ⚙️ blink
Done — blinked 5 times, ending LOW.

You: remember I like slow blinks
ESP: ⚙️ mem_save
Saved. Next time just say "blink my LEDs".

You: what do you remember?
ESP: ⚙️ mem_read
- like slow blinks
- lab sensor is on pin 34

You: read analog pin 34 and digital pin 13
ESP: ⚙️ adc_read gpio_read
A34=1842, D13=LOW.</pre>
</div>
<h3>Memory vs history</h3>
<ul>
<li><code>mem_save</code> facts persist across reboots (4KB total, 500 chars each). Inspect at <a href="/api/memory" target="_blank">/api/memory</a>.</li>
<li>Recent chat turns auto-persist (32KB log) and are re-injected as context — no need to re-paste.</li>
<li>To forget: open <a href="/functions">Functions</a> → Memory → <b>Clear memory</b> or <b>Clear chat log</b> (or POST <code>{"clear_memory":true}</code> / <code>{"clear_log":true}</code> to <code>/api/memory</code>).</li>
</ul>
<h3>Setup &amp; upkeep</h3>
<ul>
<li><a href="/setup">Setup hub</a> → <a href="/wifi">WiFi</a> (reboots) · <a href="/agent">Endpoint</a> (no reboot, never touches WiFi).</li>
<li><a href="/functions">Functions</a> — enable/disable tools (stored as a bitmask, survives reboot).</li>
<li><a href="/update">Firmware</a> — upload <code>.pio/build/esp32dev/firmware.bin</code>, or ArduinoOTA.</li>
<li><a href="/api/status" target="_blank">Status</a> — heap, uptime, model, log sizes. Watch <code>free_heap</code> if replies get flaky.</li>
</ul>
<h3>Limits (honest)</h3>
<ul>
<li>Low RAM: big pages truncate (~2000 chars from web, 1500 chars per chat message, 800 reply tokens). Why: the ESP32 has only ~225KB of free heap, shared by the page download, the encrypted connection (~35KB for HTTPS), and the chat request itself. Fetching whole pages would crash it on large sites, so it reads with a cap and hands the model the first useful 2000 characters. Bigger chunks would also mean slower replies and higher token costs, while the reply itself is capped at 800 tokens anyway — so 2000 is the sweet spot.</li>
<li>Blocking tools cap out: <code>wifi_scan</code> ~2s, <code>blink</code> ≤15s, <code>web_fetch</code> ≤15s — chat waits.</li>
<li>No JS rendering, no login pages, no file downloads via <code>web_fetch</code>.</li>
<li>Upstream errors (bad key/model, HTTP 4xx/5xx) come back as <code>ERROR: …</code> — check <a href="/agent">Endpoint</a> first.</li>
</ul>
<p><small><a href="/">Chat</a> · <a href="/functions">Functions</a> · <a href="/api/tools" target="_blank">tools JSON</a> · <a href="/api/memory" target="_blank">memory JSON</a></small></p>
</div><script>(function(){var b=document.getElementById('themebtn');if(!b)return;b.addEventListener('click',function(){var h=document.documentElement;var n=h.dataset.theme==='dark'?'light':'dark';h.dataset.theme=n;try{localStorage.setItem('agent_theme',n)}catch(e){}});})();</script></body></html>
)rawliteral";
