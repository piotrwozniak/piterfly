// SPDX-License-Identifier: GPL-3.0-only
import assert from 'node:assert/strict';
import {readFileSync} from 'node:fs';
import vm from 'node:vm';
const html=readFileSync(new URL('./editor.html',import.meta.url),'utf8');
const source=html.slice(html.indexOf('/*PROTO-BEGIN*/'),html.indexOf('/*PROTO-END*/'));
const E=vm.runInNewContext(source+';({makeMockDevice,frame,unframe,parse,req,CMD,Link,previewSnapshot,songProjectOk,bkDevFormat,duckGlobal})',
  {setTimeout,clearTimeout,setInterval,clearInterval,console,TextEncoder,TextDecoder,Blob,Response,DecompressionStream});
const mock=E.makeMockDevice({auto:false,seven:true});
const input=[...mock.access.inputs.values()][0], output=[...mock.access.outputs.values()][0];
const link=new E.Link(data=>output.send(data),{timeout:300});
input.onmidimessage=event=>link.receive(event.data);
const request=(r)=>link.request(r);
try {
  const info=E.parse[E.CMD.INFO](await request(E.req.info()));
  assert.equal(info.ntrk,8); assert.equal(info.proto,17); assert.equal(info.gcount,62); assert.equal(info.pcount,74); assert.equal(info.patterns,24);
  assert.equal(E.bkDevFormat(info),9); assert.equal(E.bkDevFormat({...info,proto:15}),7);
  const samples=E.parse[E.CMD.SMP_INFO](await request(E.req.smpInfo())); assert.equal(samples.nslots,1);
  for(let track=0;track<8;track++) {
    const state=E.parse[E.CMD.TRACK](await request(E.req.track(track)));
    assert.equal(state.sel,track); assert.equal(state.tracks.length,8);
    assert.equal(state.tracks[track].engine===info.nengines,track===7);
  }
  for(let track=0;track<8;track++) {
    const state=E.parse[E.CMD.LIVE](await request(E.req.live(15,track,1)));
    assert.equal(state.tracks.length,8); assert.equal(state.solo,(1<<(track+1))-1);
  }
  mock.state.rec=128; mock.state.tracks[7].p[40]=1;
  let live=E.parse[E.CMD.LIVE](await request(E.req.live(0)));
  assert.equal(live.rec,128); assert.equal(live.mutes,128); assert.equal(live.solo,255);
  const tracks=E.parse[E.CMD.TRACK](await request(E.req.track(7))); assert.equal(tracks.solo,255);
  mock.state.tracks[7].p[40]=0;
  live=E.parse[E.CMD.LIVE](await request(E.req.live(0))); assert.equal(live.mutes,0);
  for(let k=0;k<4;k++) {
    const d=E.parse[E.CMD.DESC](await request(E.req.desc(1,54+k))); assert.equal(d.label,'CH'+(k+4)); assert.equal(d.def,k+4);
  }
  assert.equal(E.previewSnapshot(mock.state).g.length,69);
  // A real envelope with the new size and checksum is accepted; corruption is rejected.
  const data=new Uint8Array(6016), view=new DataView(data.buffer);
  view.setUint32(0,0x46554e39,true); view.setUint32(4,data.length,true);
  let hash=0x811c9dc5; for(let i=0;i<data.length-4;i++) hash=Math.imul(hash^data[i],16777619)>>>0;
  view.setUint32(data.length-4,hash,true); assert(E.songProjectOk(data)); data[80]^=1; assert(!E.songProjectOk(data));
  console.log('eight-track editor: INFO, every track, MIDI channels, 8-bit masks, one sample bank, FUN9 checks: PASS');
} finally { link.close(); mock.stop(); }

// Experimental duck controls are exposed only when the appended descriptors exist.
const duck=E.makeMockDevice({auto:false,duck:true});
const di=[...duck.access.inputs.values()][0], dout=[...duck.access.outputs.values()][0];
const dl=new E.Link(data=>dout.send(data),{timeout:300}); di.onmidimessage=event=>dl.receive(event.data);
try {
  const info=E.parse[E.CMD.INFO](await dl.request(E.req.info())); assert.equal(info.gcount,69);
  assert.equal(E.duckGlobal({...info,gcount:62},0),null); assert.equal(E.duckGlobal(info,7),null);
  for(let track=0;track<7;track++) {
    const id=E.duckGlobal(info,track); assert.equal(id,62+track);
    const desc=E.parse[E.CMD.DESC](await dl.request(E.req.desc(1,id)));
    assert.equal(desc.label,'DUCK'); assert.equal(desc.def,1); assert.equal(desc.max,1);
    const set=E.parse[E.CMD.SET](await dl.request(E.req.set(1,id,0))); assert.equal(set.value,0);
    await dl.request(E.req.track((track+1)%8));
    const get=E.parse[E.CMD.GET](await dl.request(E.req.get(1,id))); assert.equal(get.value,0);
  }
  const snap=E.previewSnapshot(duck.state); assert.equal(snap.g.length,69); assert(snap.g.slice(62).every(v=>v===0));
  console.log('per-track duck: descriptors, defaults, independent flags and preview state: PASS');
} finally {dl.close();duck.stop();}
