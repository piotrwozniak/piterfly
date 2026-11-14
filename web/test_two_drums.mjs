// SPDX-License-Identifier: GPL-3.0-only
import assert from 'node:assert/strict';
import {readFileSync} from 'node:fs';
import vm from 'node:vm';
const html=readFileSync(new URL('./editor.html',import.meta.url),'utf8');
const source=html.slice(html.indexOf('/*PROTO-BEGIN*/'),html.indexOf('/*PROTO-END*/'));
const E=vm.runInNewContext(source+';({makeMockDevice,parse,req,CMD,Link,songProjectOk,bkDevFormat,duckGlobal})',
 {setTimeout,clearTimeout,setInterval,clearInterval,console,TextEncoder,TextDecoder,Blob,Response,DecompressionStream});
const mock=E.makeMockDevice({auto:false,twoDrums:true});
const input=[...mock.access.inputs.values()][0], output=[...mock.access.outputs.values()][0];
const link=new E.Link(data=>output.send(data),{timeout:300});
input.onmidimessage=e=>link.receive(e.data);
const rq=r=>link.request(r);
try {
 const info=E.parse[E.CMD.INFO](await rq(E.req.info()));
 assert.equal(info.proto,19); assert.equal(info.ntrk,8); assert.equal(E.bkDevFormat(info),18);
 const samples=E.parse[E.CMD.SMP_INFO](await rq(E.req.smpInfo())); assert.equal(samples.nslots,0);
 await rq(E.req.track(6));
 const kit=E.parse[E.CMD.DESC](await rq(E.req.desc(0,info.pe0))); assert.equal(kit.max,40);
 await rq(E.req.track(0));
 const mix=E.parse[E.CMD.TRACK](await rq(E.req.track(0)));
 assert.deepEqual(Array.from(mix.tracks,t=>t.engine===info.nengines),[false,false,false,false,false,false,true,true]);
 assert.equal(E.duckGlobal(info,5),67); assert.equal(E.duckGlobal(info,6),null);
 const hit={on:1,lvl:[3,...new Array(15).fill(0)],rat:new Array(16).fill(0)};
 await rq(E.req.drumStep(63,hit,6));
 const a=E.parse[E.CMD.DRUM_STEP](await rq(E.req.drumStep(63,null,6)));
 const b=E.parse[E.CMD.DRUM_STEP](await rq(E.req.drumStep(63,null,7)));
 assert.equal(a.on,1); assert.equal(b.on,0); assert.equal(mock.state.sel,0);
 await rq(E.req.live(12,15,0,7,6));
 const ta=E.parse[E.CMD.LIVE](await rq(E.req.live(0,6)));
 const tb=E.parse[E.CMD.LIVE](await rq(E.req.live(0,7)));
 assert.equal(ta.dtw[15].tune,-8); assert.equal(ta.dtw[15].dec,3); assert.equal(tb.dtw[15].tune,0);
 const bytes=new Uint8Array(6160),view=new DataView(bytes.buffer);
 view.setUint32(0,0x46554e42,true); view.setUint32(4,bytes.length,true);
 let hash=0x811c9dc5; for(let i=0;i<bytes.length-4;i++) hash=Math.imul(hash^bytes[i],16777619)>>>0;
 view.setUint32(bytes.length-4,hash,true); assert(E.songProjectOk(bytes)); bytes[100]^=1; assert(!E.songProjectOk(bytes));
 console.log('6+2 editor: two drum engines, independent steps/tuning without selecting, FUNB, six DUCK targets: PASS');
} finally { link.close(); mock.stop(); }
