// Development captures of the web editor only. FM-1 device documentation uses render_ui_docs.c.
import fs from 'node:fs';
import path from 'node:path';
import {createHash} from 'node:crypto';
import {openBrowser} from '../promo/node_modules/@remotion/renderer/dist/index.js';
const out=path.resolve(process.argv[2]||'build/web-editor-captures');
fs.mkdirSync(out,{recursive:true});
console.log('Opening documentation browser');
let page;
const browser=await openBrowser('chrome',{logLevel:'error',browserExecutable:'/Applications/Google Chrome.app/Contents/MacOS/Google Chrome',chromeMode:'chrome-for-testing',chromiumOptions:{headless:true}});
try{
 page=await browser.newPage({context:()=>null,logLevel:'error',indent:false,pageIndex:0,onBrowserLog:l=>{if(l.type==='error')console.log('Browser:',l.text)},onLog:()=>{}});
 await page.setViewport({width:1440,height:1100,deviceScaleFactor:1});
 await page.evaluateOnNewDocument(()=>{window.confirm=()=>true;});
 console.log('Loading editor');
 await page.goto({url:'http://127.0.0.1:8766/webapp/editor/?mock=1',timeout:30000});
 console.log('Editor loaded',await page.evaluate(()=>({ready:document.readyState,mock:typeof window.__mockState,button:document.getElementById('connect').textContent})));
 await page.evaluate(()=>{document.getElementById('connect').click();return 'clicked';});
 console.log('Connect clicked');
 const wait=async(fn)=>{const end=Date.now()+30000;while(!await page.evaluate(fn)){if(Date.now()>end)throw Error('Mock UI did not settle: '+fn.toString());await new Promise(r=>setTimeout(r,100));}};
 await wait(()=>window.__mockState()?.tracks.length===8&&document.querySelectorAll('#slots .slot').length===24&&!document.getElementById('connect').disabled);
 console.log('Connected to eight-track mock');
 await page.evaluate(()=>document.querySelector('#tabs button[data-tab="projects"]').click());
 for(let k=0;k<24;k++){
  await page.evaluate(index=>document.querySelectorAll('#slots .slot')[index].querySelector('button:last-child').click(),k);
  await wait(()=>document.querySelectorAll('#slots .slot.used').length===24||!document.getElementById('connect').disabled);
  await page.evaluate(async index=>{const end=Date.now()+10000;while(!document.querySelectorAll('#slots .slot')[index].classList.contains('used')){if(Date.now()>end)throw Error('Pattern not stored');await new Promise(r=>setTimeout(r,50));}},k);
 }
 await page.evaluate(()=>document.querySelector('#tabs button[data-tab="song"]').click());
 await wait(()=>document.querySelectorAll('#songrows tr').length>0);
 await page.evaluate(()=>{while(!document.getElementById('songadd').disabled)document.getElementById('songadd').click();const rows=Array.from(document.querySelectorAll('#songrows tr'));if(rows.length!==128)throw Error('128 SONG positions unavailable');rows.forEach((row,i)=>{const input=row.querySelector('input');input.value=String([4,8,16][i%3]);input.dispatchEvent(new Event('change',{bubbles:true}));});document.getElementById('songsend').click();});
 await wait(()=>window.__mockState().settings[48]===128&&!document.getElementById('songsend').classList.contains('pri'));
 await page.evaluate(()=>document.getElementById('songread').click());
 await wait(()=>document.querySelectorAll('#songrows tr').length===128&&document.getElementById('songsum').textContent.includes('128'));
 // Reconnect to the same mock and reload its stored state before capture.
 await page.evaluate(()=>document.getElementById('connect').click());
 await page.evaluate(()=>document.getElementById('connect').click());
 await wait(()=>!document.getElementById('connect').disabled&&document.querySelectorAll('#slots .slot.used').length===24);
 await wait(()=>document.querySelectorAll('#songrows tr').length===128);
 const shot=async(name,height)=>{
  const r=await page._client().send('Page.captureScreenshot',{format:'png',captureBeyondViewport:true,clip:{x:0,y:0,width:1440,height,scale:1}});
  fs.writeFileSync(path.join(out,name+'.png'),Buffer.from(r.value.data,'base64'));
  console.log(name,height);
 };
 await page.evaluate(()=>{document.querySelector('#tabs button[data-tab="projects"]').click();window.scrollTo(0,0)});
 let height=await page.evaluate(()=>Math.ceil(document.getElementById('slots').getBoundingClientRect().bottom+window.scrollY+40));
 await shot('editor-24-patterns',height);
 await page.evaluate(async()=>{document.querySelector('#tabs button[data-tab="tracks"]').click();const end=Date.now()+10000;while(document.querySelectorAll('#mixer .strip').length<8){if(Date.now()>end)break;await new Promise(r=>setTimeout(r,100));}window.scrollTo(0,0)});
 height=await page.evaluate(()=>Math.ceil(document.getElementById('p-tracks').getBoundingClientRect().bottom+window.scrollY+30));
 await shot('editor-eight-tracks',height);
 await page.evaluate(()=>{document.querySelector('#tabs button[data-tab="song"]').click();window.scrollTo(0,0)});
 await shot('editor-song-start',1100);
 // The lower viewport is an unmodified application view: rows above 64 and the 128/128 summary.
 for(const [name,row] of [['editor-song-65',65],['editor-song-128',128]]){
  const y=await page.evaluate(index=>{const rows=document.querySelectorAll('#songrows tr');const first=Math.max(0,index-10);rows[first].scrollIntoView({block:'start'});return Math.round(window.scrollY)},row);
  const r=await page._client().send('Page.captureScreenshot',{format:'png',captureBeyondViewport:false});
  fs.writeFileSync(path.join(out,name+'.png'),Buffer.from(r.value.data,'base64'));console.log(name,'scroll',y);
 }
 fs.writeFileSync(path.join(out,'WEB-DOCS-FIXTURE.json'),JSON.stringify({source:'web/editor.html',sourceSHA256:createHash('sha256').update(fs.readFileSync(new URL('../web/editor.html',import.meta.url))).digest('hex'),mode:'built-in mock; no physical MIDI',patternSlots:24,tracks:8,songPositions:128,barsCycle:[4,8,16],savedAndReadBack:true,viewport:[1440,1100]},null,2)+'\n');
}finally{if(page&&!page.closed)await page.close();await browser.close({silent:true});}
