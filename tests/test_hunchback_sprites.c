#include "game.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static uint8_t originals[CombatSpriteCount][4];
static struct { uint8_t *sprite,height; int16_t x,y; } draws[16];
static unsigned drawCount;
static void checkAt(int condition,unsigned line)
{
    if(!condition) { fprintf(stderr,"Hunchback sprite check failed at line %u\n",line); exit(1); }
}
#define check(c) checkAt((c),__LINE__)
void Draw_Persistent_Map_Effects(void) {}
void DrawCall_EGA_CharacterPos(EgaMemoryAddress destination,uint8_t *sprite,int16_t x,int16_t y)
{
    check(destination.segment==MapViewportSegment && drawCount<16);
    draws[drawCount].sprite=sprite;
    draws[drawCount].height=sprite[SpriteHeightMinusOneByte];
    draws[drawCount].x=x; draws[drawCount++].y=y;
}
void Map_Move_North(void) { check(0); }
void Map_Move_South(void) { check(0); }
void Map_Move_West(void) { check(0); }
void Map_Move_East(void) { check(0); }

static unsigned pixel(const uint8_t *sprite,unsigned x,unsigned y)
{
    unsigned offset=4+(y*3+x/8)*4,colour=0;
    for(unsigned plane=0;plane<4;++plane)
        colour|=((sprite[offset+plane]>>(7-x%8))&1u)<<plane;
    return colour;
}
static void reset(void)
{
    memset(Mechs,255,sizeof Mechs); memset(Characters,255,sizeof Characters);
    memset(CombatantPackedX,255,sizeof CombatantPackedX);
    memset(CombatantPackedY,255,sizeof CombatantPackedY);
    memset(CombatantSpriteFrame,0,sizeof CombatantSpriteFrame);
    memset(CombatantSpriteFamilyOffset,MECH_Sprite_COMMANDO,sizeof CombatantSpriteFamilyOffset);
    for(unsigned id=0;id<CombatSpriteCount;++id) {
        originals[id][1]=23; originals[id][2]=3; CombatSpritePointers[id]=originals[id];
    }
    for(unsigned i=0;i<MapCharacterCount;++i) RoamingMapNpcs[i].movementDelay=1;
    memset(CombatMap,0,sizeof CombatMap);
    CrescentHawkMapPositionX=0x0220; CrescentHawkMapPositionY=0x3020;
    InsideStarLeagueCache=FALSE; CachedMapOriginIndex=0; DrawJailMissionParkedMechs=FALSE;
    drawCount=0;
}
static void unit(unsigned id,int dy)
{
    CombatantPackedX[id]=CrescentHawkMapPositionX;
    CombatantPackedY[id]=(uint16_t)(CrescentHawkMapPositionY+dy);
}
int main(void)
{
    reset(); Mechs[0]=MechRefs[MechRef_Hunchback]; Mechs[4]=MechRefs[MechRef_Hunchback];
    uint8_t *bank[HunchbackSpriteCount];
    for(unsigned f=0;f<HunchbackSpriteCount;++f) {
        uint16_t sprite=(uint16_t)(MECH_Sprite_COMMANDO+f);
        bank[f]=Resolve_Mech_Combat_Sprite(0,sprite);
        check(bank[f]!=originals[sprite] && bank[f]==Resolve_Mech_Combat_Sprite(12,sprite));
        check(bank[f][0]==0 && bank[f][1]==23 && bank[f][2]==3 && bank[f][3]==0);
        unsigned occupied=0,black=0,green=0;
        for(unsigned y=0;y<24;++y) for(unsigned x=0;x<24;++x) {
            unsigned c=pixel(bank[f],x,y); occupied+=(c!=0); black+=(c==1); green+=(c==2);
        }
        check(occupied>100 && occupied<500 && black>10 && green>0);
        for(unsigned previous=0;previous<f;++previous)
            check(memcmp(bank[f],bank[previous],HunchbackSpriteBytes)!=0);
    }
    /* Native frame contract: rear 0..3/17, east 4..7/18, front 8..11/16,
     * west 12..15/19. Head has no rear-facing cockpit; cannon is not mirrored. */
    unsigned frontCockpit=0,backCockpit=0;
    for(unsigned y=0;y<6;++y) for(unsigned x=0;x<24;++x) {
        frontCockpit+=(pixel(bank[16],x,y)==14);
        backCockpit+=(pixel(bank[17],x,y)==14);
    }
    check(frontCockpit==2 && backCockpit==0);
    check(pixel(bank[16],6,2)==1 && pixel(bank[17],16,2)!=0);
    check(pixel(bank[16],18,2)==0 && pixel(bank[17],5,2)==0);
    /* Compact shoulders (12px), a filled 8px waist and cannon against head. */
    check(pixel(bank[16],5,6)==0 && pixel(bank[16],6,6)!=0);
    check(pixel(bank[16],17,6)!=0 && pixel(bank[16],18,6)==0);
    for(unsigned x=8;x<16;++x) check(pixel(bank[16],x,10)!=0);
    check(pixel(bank[16],10,2)==1 && pixel(bank[16],11,2)!=0);
    /* Never change the original table, infantry, other chassis or shared effects. */
    for(unsigned id=0;id<CombatSpriteCount;++id) check(CombatSpritePointers[id]==originals[id]);
    for(unsigned id=0;id<AllCombatantCount;++id) {
        if(id==0 || id==12) continue;
        check(Resolve_Mech_Combat_Sprite((uint16_t)id,162)==originals[162]);
    }
    check(Resolve_Mech_Combat_Sprite(24,162)==originals[162]);
    for(unsigned sprite=0;sprite<CombatSpriteCount;++sprite)
        if(sprite<146 || sprite>=166) check(Resolve_Mech_Combat_Sprite(0,(uint16_t)sprite)==originals[sprite]);
    CombatantSpriteFamilyOffset[0]=MECH_Sprite_LOCUST;
    check(Resolve_Mech_Combat_Sprite(0,162)==originals[162]);
    CombatantSpriteFamilyOffset[0]=MECH_Sprite_COMMANDO;
    Mechs[0].name[0]=MECH_Destroyed;
    check(Resolve_Mech_Combat_Sprite(0,162)==originals[162]);

    /* Identity must follow the ascending-Y sort, not the input array slot. */
    reset(); memset(CombatMap,0x2F,sizeof CombatMap);
    Mechs[0]=MechRefs[MechRef_Commando]; Mechs[3]=MechRefs[MechRef_Hunchback];
    Mechs[4]=MechRefs[MechRef_Hunchback]; Mechs[5]=MechRefs[MechRef_Commando];
    unit(0,4); unit(3,-2); unit(12,2); unit(13,-4);
    CombatantSpriteFrame[0]=CombatantSpriteFrame[12]=16;
    CombatantSpriteFrame[3]=CombatantSpriteFrame[13]=17;
    Draw_Menu_MultiSelect(); check(drawCount==4);
    check(draws[0].sprite==originals[163] && draws[1].sprite==bank[17]);
    check(draws[2].sprite==bank[16] && draws[3].sprite==originals[162]);
    for(unsigned i=0;i<4;++i) check(draws[i].height==7 && draws[i].sprite[1]==23);
    for(unsigned i=0;i<HunchbackSpriteCount;++i) check(bank[i][1]==23);

    /* Exploration compacts formation slots but must select art by record ID. */
    reset(); Mechs[1]=MechRefs[MechRef_Hunchback]; Mechs[2]=MechRefs[MechRef_Commando];
    CombatantSpriteFrame[1]=CombatantSpriteFrame[2]=16;
    Draw_Infantry_And_Mechs(); check(drawCount==2);
    check(draws[0].sprite==bank[16] && draws[1].sprite==originals[162]);
    check(bank[16][1]==23 && draws[0].height==23);
    puts("Hunchback: all native frames, per-record selection, depth sort and clipping passed");
    return 0;
}
