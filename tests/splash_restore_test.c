/* SPDX-License-Identifier: GPL-3.0-only */
#define FELUCCA_SYNTH_PARTS 7
#include "editor_host.c"

int main(int argc,char **argv)
{
    /* Hashes of the approved, uncompressed arcade logo before this change. */
    assert(SLOOP_SPLASH_W==216 && SLOOP_SPLASH_H==156 && SLOOP_SPLASH_RLE_SIZE==2090);
    memset(main_scratch.st,0xA5,sizeof main_scratch.st);
    const uint8_t *rle=sloop_logo_unpack(); assert(rle);
    uint64_t hash=1469598103934665603ull;
    for(uint32_t i=0;i<SLOOP_SPLASH_RLE_SIZE;i++) hash=(hash^rle[i])*1099511628211ull;
    assert(hash==0x9c6f1e7e62dda20cull);
    for(uint32_t i=SLOOP_SPLASH_RLE_SIZE;i<sizeof main_scratch.st;i++) assert(main_scratch.st[i]==0xA5);
    for(uint32_t pass=0;pass<2;pass++) {
        memset(screen,0,sizeof screen); sloop_splash(); hash=1469598103934665603ull;
        for(uint32_t y=26;y<182;y++) for(uint32_t x=12;x<228;x++) {
            uint16_t p=swap16(screen[y*240+x]);
            hash=(hash^(p&255u))*1099511628211ull; hash=(hash^(p>>8))*1099511628211ull;
        }
        assert(hash==0x629ffd11f9ea534dull);
    }
    if(argc>1) { FILE *f=fopen(argv[1],"wb"); assert(f); fprintf(f,"P6\n240 240\n255\n");
        for(uint32_t i=0;i<240*240;i++) { uint16_t p=swap16(screen[i]);
            uint8_t c[3]={(uint8_t)(((p>>11)&31)*255/31),(uint8_t)(((p>>5)&63)*255/63),(uint8_t)((p&31)*255/31)}; fwrite(c,1,3,f); } fclose(f); }
    puts("arcade splash: lossless RLE and identical approved RGB565 pixels; scratch bounds and repeated boot: PASS");
    return 0;
}
