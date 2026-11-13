/* SPDX-License-Identifier: GPL-3.0-only */
/* Same browser adapter on the host, for sample-by-sample WASM comparisons. */
#include <stdio.h>
#define __attribute__(x)
#include "../firmware/host/preview.c"
int main(int argc, char **argv)
{
    if (argc == 2 && !strcmp(argv[1], "--layout")) {
        printf("%u %u %u\n", (unsigned)P_COUNT, (unsigned)G_COUNT, (unsigned)P_E0);
        return 0;
    }
    int32_t config[4 + P_COUNT + G_COUNT];
    if (fread(config, sizeof config, 1, stdin) != 1) return 1;
    preview_init();
    preview_render(); /* the adapter obtains its initial output buffer before syncing */
    preview_track(0, config[0], config[1]);
    for (uint32_t i = 0; i < P_COUNT; i++) preview_param(0, i, config[4 + i]);
    for (uint32_t i = 0; i < G_COUNT; i++) preview_global(i, config[4 + P_COUNT + i]);
    if (fread(transfer, FM6_PACKED, 1, stdin) != 1) return 1;
    preview_fm6(0);
    if(argc==2 && !strcmp(argv[1],"--duck")) {
        int32_t p[P_COUNT]; if(fread(p,sizeof p,1,stdin)!=1) return 1;
        for(uint32_t i=0;i<P_COUNT;i++) preview_param(TRK_DRUM,i,p[i]);
    }
    preview_note(0, config[2], 1);
    if(argc==2 && !strcmp(argv[1],"--duck")) preview_note(TRK_DRUM,36,1);
    for (uint32_t i = 0; i < (uint32_t)config[3]; i += CTL) {
        if(argc==2 && !strcmp(argv[1],"--grain-bypass")) {
            if(i==(uint32_t)config[3]/4u) preview_param(0,P_IMIX,0);
            if(i==(uint32_t)config[3]/2u) { preview_param(0,P_GFREEZE,1); preview_param(0,P_IMIX,80); }
            if(i==(uint32_t)config[3]*3u/4u) preview_param(0,P_GFREEZE,0);
        }
        if (i == (uint32_t)config[3] / 2u) preview_note(0, config[2], 0);
        if (fwrite(preview_render(), sizeof output, 1, stdout) != 1) return 1;
    }
    return 0;
}
