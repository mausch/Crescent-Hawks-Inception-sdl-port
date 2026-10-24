#include "game.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static uint8_t image[PackedGraphicsOutputBytes+32];
static uint8_t expected[PackedGraphicsOutputBytes];
static void verify(int condition,unsigned line)
{
    if(!condition) { fprintf(stderr,"Hunchback inspection mismatch line %u\n",line); exit(1); }
}
#define check(condition) verify(!!(condition),__LINE__)
static int patched(unsigned x,unsigned y)
{
    return (x>=120 && x<232 && y<24) ||
        (x>=120 && x<264 && y>=24 && y<144) ||
        (x>=120 && x<252 && y>=144 && y<192) ||
        (x>=264 && x<320 && y>=96 && y<170);
}
static unsigned pixel(unsigned x,unsigned y)
{
    unsigned value=image[16+y*PackedGraphicsRowBytes+x/2];
    return x%2?value&15:value>>4;
}
int main(void)
{
    Mech mech={0},before;
    memset(image,0x5a,sizeof image);
    memcpy(mech.name,"COMMANDO       ",MechNameLength);
    Patch_Hunchback_Inspection(image+16,&mech);
    for(unsigned i=0;i<sizeof image;++i) check(image[i]==0x5a);
    for(unsigned loc=0;loc<MechArmourLocationCount;++loc) {
        check(Resolve_Mech_Status_Gauge_X(&mech,loc)==MechStatusGaugeX[loc]);
        check(Resolve_Mech_Status_Gauge_Bottom_Y(&mech,loc)==MechStatusGaugeBottomY[loc]);
    }

    memcpy(mech.name,"HUNCHBACK      ",MechNameLength);
    before=mech;
    Patch_Hunchback_Inspection(image+16,&mech);
    check(memcmp(&mech,&before,sizeof mech)==0);
    for(unsigned y=0;y<EgaScreenHeight;++y)
        for(unsigned x=0;x<EgaScreenWidth;x+=2)
            if(!patched(x,y)) check(image[16+y*PackedGraphicsRowBytes+x/2]==0x5a);
    for(unsigned i=0;i<16;++i) check(image[i]==0x5a && image[sizeof image-1-i]==0x5a);
    check(pixel(170,25)==0 && pixel(165,21)==15); /* AC/20 bore & shroud bevel, viewer left. */
    check(pixel(194,22)==0 && pixel(192,21)==15); /* Unobstructed tucked head visor & glint. */
    check(pixel(220,6)==15); /* Left-torso antenna on viewer right. */
    check(pixel(273,98)==15 && pixel(305,98)==7); /* Rear inset: antenna left, AC/20 hump right. */
    check(pixel(192,40)==0 && pixel(176,65)==0 && pixel(132,80)==0 && pixel(251,80)==0); /* Gauge wells. */
    check(Resolve_Mech_Status_Gauge_X(&mech,0)==251 && Resolve_Mech_Status_Gauge_X(&mech,5)==132);
    check(Resolve_Mech_Status_Gauge_Bottom_Y(&mech,3)==44 && Resolve_Mech_Status_Gauge_Bottom_Y(&mech,6)==73);
    check(pixel(127,120)==9 && pixel(128,120)==9); /* Aligned blue grid. */
    memcpy(expected,image+16,sizeof expected);
    Patch_Hunchback_Inspection(image+16,&mech);
    check(memcmp(expected,image+16,sizeof expected)==0);
    /* Freshly decoded shared screen restores every other chassis, even after
     * a Hunchback inspection. Art must not leak into the cached CMP buffer. */
    memset(image,0x5a,sizeof image);
    memcpy(mech.name,"JENNER         ",MechNameLength);
    Patch_Hunchback_Inspection(image+16,&mech);
    for(unsigned i=0;i<sizeof image;++i) check(image[i]==0x5a);
    puts("Hunchback inspection: selection, packed pixels, bounds, gauge wells and fallback passed.");
    return 0;
}
