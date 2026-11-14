// SPDX-License-Identifier: GPL-3.0-only
import assert from 'node:assert/strict';
import {readFileSync} from 'node:fs';
import {spawn} from 'node:child_process';
import {createInterface} from 'node:readline';
import vm from 'node:vm';
const html=readFileSync('web/editor.html','utf8');
const source=html.slice(html.indexOf('/*PROTO-BEGIN*/'),html.indexOf('/*PROTO-END*/'));
const E=vm.runInNewContext(source+';({parse,req,CMD,HDR,unframe,deviceSongCapture,songObjects,songLoad,songProjectOk})',
 {setTimeout,clearTimeout,setInterval,clearInterval,console,TextEncoder,TextDecoder,Blob,Response,DecompressionStream});
const child=spawn(process.env.SONG_BRIDGE || 'build/host/song_library_bridge',[],{stdio:['pipe','pipe','inherit']});
const pending=[];
createInterface({input:child.stdout}).on('line',line=>{
 const bytes=Uint8Array.from(line.trim().split(/\s+/).map(x=>parseInt(x,16)));
 const frame=E.unframe(bytes); const next=pending.shift(); if(!next) throw new Error('Unexpected bridge output'); next.resolve(frame.a);
});
child.on('exit',code=>{ for(const p of pending) p.reject(new Error(`Bridge exited ${code}`)); });
const rq=([cmd,args])=>new Promise((resolve,reject)=>{ pending.push({resolve,reject}); child.stdin.write([...E.HDR,cmd,...args].map(x=>x.toString(16)).join(' ')+'\n'); });
try {
 const info=E.parse[E.CMD.INFO](await rq(E.req.info())); assert.equal(info.proto,19);
 assert.equal(E.parse[E.CMD.SMP_INFO](await rq(E.req.smpInfo())).nslots,0);
 let list=E.parse[E.CMD.SONG_LIBRARY](await rq(E.req.songLibrary(0)));
 assert.equal(list.totalKiB,352); assert.equal(list.songs.filter(s=>s.used).length,2);
 assert.equal(list.songs[0].name,'FIRST'); assert.equal(list.songs[1].name,'SECOND');
 const first=await E.deviceSongCapture(rq,info,0), second=await E.deviceSongCapture(rq,info,1);
 assert.equal(first.name,'FIRST'); assert.equal(second.name,'SECOND'); E.songObjects(first);
 assert.notEqual(first.objects.find(o=>o.id===2).data,second.objects.find(o=>o.id===2).data);
 assert.equal(first.objects.length,2,'export only working loop and nonempty pattern');
 assert.equal(E.parse[E.CMD.SONG_LIBRARY](await rq(E.req.songLibrary(2,0))).rc,0);
 const again=await E.deviceSongCapture(rq,info,0); assert.equal(again.objects[1].data,first.objects[1].data);
 await E.songLoad(rq,first);
 const afterImport=await E.deviceSongCapture(rq,info,1);
 assert.equal(afterImport.objects[1].data,second.objects[1].data,'import never overwrites a saved library song');
 // Exercise the browser controls with actual firmware requests and a small DOM double.
 const controls=new Map();
 const control=id=>{
  if(!controls.has(id)) controls.set(id,{value:'',textContent:'',hidden:false,disabled:false,
   replaceChildren(...items){this.items=items;this.value=items[0]?.value||'';}});
  return controls.get(id);
 };
 let refreshed=0,slotsRendered=0; const downloads=[];
 const uiState={setTimeout,clearTimeout,setInterval,clearInterval,console,TextEncoder,TextDecoder,Blob,Response,DecompressionStream,
  dev:{info},SG:{dirty:false},MOCK:false,patternCount:()=>24,rq,$:control,el:(tag,props)=>({...props}),confirm:()=>true,op:fn=>fn(),
  afterSoundChange:async()=>{},readSong:async()=>{refreshed++;},renderSlots:()=>{slotsRendered++;},
  download:(name,file)=>downloads.push({name,file}),fileSlug:s=>s.toLowerCase()};
 const gui=html.slice(html.indexOf('let deviceSongDirectory=null;'),html.indexOf('$('+'"device-song-slot").addEventListener'));
 const UI=vm.runInNewContext(source+gui+';({deviceSongsRead,deviceSongAction})',uiState);
 await UI.deviceSongsRead(); assert.equal(control('device-song-slot').items.length,8);
 control('device-song-slot').value='1'; await UI.deviceSongAction(2);
 assert.equal(refreshed,1,'load must refresh the displayed song order immediately');
 assert.equal(slotsRendered,1); assert.equal(uiState.dev.slotUsed[0],true); assert.equal(uiState.dev.slotUsed[4],false);
 uiState.SG.dirty=true; await UI.deviceSongAction(1);
 assert.match(control('device-song-status').textContent,/Send the edited song order/);
 uiState.SG.dirty=false; await UI.deviceSongAction(4);
 assert.equal(downloads.length,1); assert.equal(downloads[0].file.name,'SECOND');
 const stat=E.parse[E.CMD.SONG_LIBRARY](await rq(E.req.songLibrary(4,0,1)));
 assert.equal(E.parse[E.CMD.SONG_LIBRARY](await rq(E.req.songLibrary(3,0))).rc,0);
 const stale=E.parse[E.CMD.SONG_LIBRARY](await rq(E.req.songLibrary(5,0,1,...[0,1,2,3,4].map(k=>Math.floor(stat.seq/2**(7*k))&127),0,0,0,0,0,32,0)));
 assert.notEqual(stale.rc,0);
 list=E.parse[E.CMD.SONG_LIBRARY](await rq(E.req.songLibrary(0))); assert.equal(list.songs[0].used,false); assert.equal(list.songs[1].used,true);
 console.log('Real C firmware → USB protocol → browser song export/import: named list, sparse file, load, CRC and stale/delete protection PASS');
} finally { child.stdin.end(); }
