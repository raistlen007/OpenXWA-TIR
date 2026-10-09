#include "xwa_remaster/frontend_font_aa.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int test(void) {
    unsigned char src[8*8*4] = {0};
    src[((size_t)3*8+3)*4+3] = 255;
    int w=0,h=0;
    unsigned char *dst=XwaFrontendFontAA_Upscale(src,8,8,4,&w,&h);
    if (!dst || w!=32 || h!=32) {free(dst);return 1;}
    int partial=0, fully=0, zeros=0;
    for(int i=0;i<w*h;i++){
        if(dst[i*4]!=255 || dst[i*4+1]!=255 || dst[i*4+2]!=255){free(dst);return 2;}
        if(dst[i*4+3] == 0) zeros++;
        else if(dst[i*4+3] == 255) fully++;
        else partial++;
    }
    free(dst);
    if(!partial || !zeros || fully){fprintf(stderr,"partial=%d zeros=%d fully=%d\n",partial,zeros,fully); return 3;}
    memset(src,0,sizeof src);
    for(int y=2;y<=5;y++)for(int x=2;x<=5;x++)src[((size_t)y*8+x)*4+3]=255;
    dst=XwaFrontendFontAA_Upscale(src,8,8,4,&w,&h);
    if(!dst) return 4;
    if(dst[((size_t)15*32+15)*4+3] != 255){free(dst);return 5;}
    if(dst[((size_t)0*32+0)*4+3] != 0){free(dst);return 6;}
    free(dst);
    puts("Font AA passed: softened subpixel edges, opaque interiors, transparent gutters, straight-white ink");
    return 0;
}
int main(void){return test();}
