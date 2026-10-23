#include "game.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static unsigned randomCalls,diceCalls;
static uint8_t randomValue;
static uint16_t diceValue;
static unsigned inspectPilot;
static unsigned crewRollStart;
static const uint8_t crewRolls[8]={0,1,1,0,0,0,1,1};
static void verify(int ok,unsigned line) { if(!ok) { fprintf(stderr,"Spawn mismatch line%u randomCalls%u\n",line,randomCalls); exit(1); } }
#define check(x) verify(!!(x),__LINE__)
static int sameTemplate(const Mech *actual,const Mech *stock) {
    Mech expected=*stock; expected.pilotId=actual->pilotId;
    check(actual->pilotId==(EnemyPilotSkillsTag | (randomValue&1) | ((randomValue&1)<<EnemyPilotPilotingShift)));
    return !memcmp(actual,&expected,sizeof expected);
}
/* Real whole generator, original templates/animation data. RNG/dice only
 * are controlled boundaries; no fake spawn/damage/rules implementation. */
uint8_t Rand_0x00_to_0xFF(void) {
    ++randomCalls;
    if(randomCalls==1) {
        for(unsigned actor=0;actor<24;++actor) check(CombatantCasualtyFlags[actor]==0 && CombatantPackedX[actor]==65535 && CombatantPackedY[actor]==65535);
        for(unsigned pilot=8;pilot<16;++pilot) check(Characters[pilot].name==255);
    }
    if(inspectPilot && randomCalls==3) check(Characters[8].name==0 && Characters[8].mechAssignment==4 && Mechs[4].name[0]=='L');
    if(crewRollStart && randomCalls>=crewRollStart) return crewRolls[randomCalls-crewRollStart];
    return randomValue;
}
uint16_t Roll2D6(void) { ++diceCalls; return diceValue; }
static void prepare(void) {
    randomCalls=diceCalls=inspectPilot=crewRollStart=0; randomValue=0; diceValue=7;
    KuritaAttackFlag=ArenaRentalMechMode=0; EnemyMechDamageLevel=0; PersistentState.bytes[0]=0;
    memset(Characters,0xA5,sizeof Characters); memset(Mechs,0xA5,sizeof WorldMapState.fields.mechs);
    memset(CombatantActionState,0xA5,Enemy_All_CombatantId_Range_First);
    memset(CombatantCasualtyFlags,0xA5,sizeof CombatantCasualtyFlags);
    memset(RoamingMapNpcs,0xA5,sizeof RoamingMapNpcs);
    for(unsigned actor=0;actor<24;++actor) { CombatantPackedX[actor]=(uint16_t)(0x0100+actor); CombatantPackedY[actor]=(uint16_t)(0x7000+actor); }
}
int main(void) {
    static const int offsetsX[16]={0,4,0,4,0,1,2,0,1,2,0,1,0,0,4,4};
    static const int offsetsY[16]={0,0,4,4,1,1,1,2,2,2,3,3,0,1,3,5};
    for(unsigned mechs=0;mechs<=4;++mechs) for(unsigned infantry=0;infantry<=10;++infantry) {
        prepare(); Mission_GenerateEnemies((uint16_t)mechs,(uint16_t)infantry);
        unsigned end=8+mechs+infantry; if(end>16) end=16;
        check(randomCalls==2+(end-8-mechs)*7+mechs*2 && diceCalls==end-8-mechs);
        check(CombatantActionState[0]==0); for(unsigned actor=1;actor<12;++actor) check(CombatantActionState[actor]==0xA5);
        for(unsigned npc=0;npc<8;++npc) {
            check(RoamingMapNpcs[npc].currentPositionX==0x0110+npc && RoamingMapNpcs[npc].currentPositionY==0x7010+npc);
            check(RoamingMapNpcs[npc].movementDelay==0xA5 && RoamingMapNpcs[npc].waypointPair==0xA5);
        }
        for(unsigned actor=0;actor<24;++actor) {
            int spawned=(actor>=12 && actor<12+mechs)||(actor>=16+mechs && actor<8+end);
            check(CombatantActive[actor]==(spawned || actor==0)); check(CombatantCasualtyFlags[actor]==0);
            if(!spawned) { check(CombatantPackedX[actor]==65535 && CombatantPackedY[actor]==65535); continue; }
            unsigned record=actor<16?actor-12:actor-8;
            check(CombatantPackedX[actor]==0x0C65+offsetsX[record] && CombatantPackedY[actor]==0xC059+offsetsY[record]);
            check(CombatantMovementDirection[actor]==6 && CombatantAnimationSelector[actor]==6);
        }
        for(unsigned record=8;record<16;++record) {
            Character *person=&Characters[record];
            if(record<8+mechs) check(person->name==0 && person->mechAssignment==record-4 && person->body==0xA5);
            else if(record<end) {
                check(person->name==0 && person->mechAssignment==8 && person->body==7 && person->health==70 && person->weapon==0);
                check(person->skillMedical==0xA5 && person->dexterity==0xA5 && person->armourValue==0xA5);
            } else check(person->name==255 && person->body==0xA5);
        }
        for(unsigned slot=0;slot<4;++slot) {
            if(slot<mechs) check(sameTemplate(&Mechs[4+slot],&MechRefs[0]));
            else { check(Mechs[4+slot].name[0]==255 && Mechs[4+slot].name[1]==0xA5); }
        }
    }
    static const uint8_t armourLoss[7]={0,1,3,5,10,14,20};
    for(unsigned level=0;level<128;++level) {
        prepare(); EnemyMechDamageLevel=(uint8_t)level; randomValue=1; inspectPilot=level>4; Mission_GenerateEnemies(1,0);
        unsigned effective=level>6?6:level; Mech expected=MechRefs[0];
        for(unsigned location=0;location<11;++location) expected.currentArmour[location]=expected.currentArmour[location]>armourLoss[effective]?(uint8_t)(expected.currentArmour[location]-armourLoss[effective]):0;
        if(effective>4 && ((uint8_t *)&expected)[0x35]!=0) ((uint8_t *)&expected)[0x35]|=0x80;
        if(effective>5) expected.engineHits=expected.sensorHits=expected.gyroHits=1;
        check(sameTemplate(&Mechs[4],&expected) && EnemyMechDamageLevel==effective);
        check(randomCalls==4+(effective>4?5u:0u)+(effective>5?3u:0u));
    }
    static const unsigned templateIndex[11]={5,7,3,0,1,0,2,1,3,5,MechRef_Hunchback};
    for(unsigned roll=2;roll<=12;++roll) {
        prepare(); diceValue=(uint16_t)roll; Mission_GenerateEnemies(0x81,0);
        check(sameTemplate(&Mechs[4],&MechRefs[templateIndex[roll-2]]) && diceCalls==1 && randomCalls==5);
        check(CombatantPackedX[12]==0x0A10 && CombatantPackedY[12]==0x806F);
    }
    for(unsigned band=0;band<4;++band) {
        prepare(); randomValue=(uint8_t)band; ArenaRentalMechMode=TRUE; StoredPartyMechNameInitial[2]=255;
        Mission_GenerateEnemies(0x81,0); check(diceCalls==1 && randomCalls==(band&1?9u:8u));
        check(CombatantActive[13]==TRUE && CombatantPackedX[13]==0x0A06 && CombatantPackedY[13]==0x8066);
        check(sameTemplate(&Mechs[5],&MechRefs[6]) && CombatantSpriteFrame[13]==16 && CombatantAnimationSelector[13]==2);
        check(CombatantAnimationCursors[13].data==EnemySpectatorAnimationStream && CombatantAnimationCursors[13].offset==0x4DD8);
        check(sameTemplate(&Mechs[4],&MechRefs[templateIndex[3+band]]));
    }
    prepare(); ArenaRentalMechMode=TRUE; memset(StoredPartyMechNameInitial,'L',LanceSize);
    Mission_GenerateEnemies(0x81,0); check(CombatantActive[12]==TRUE && CombatantActive[13]==FALSE && diceCalls==1 && randomCalls==6);
    for(unsigned roll=2;roll<=12;++roll) for(unsigned random=0;random<256;++random) {
        prepare(); diceValue=(uint16_t)roll; randomValue=(uint8_t)random; Mission_GenerateEnemies(0,1);
        check(Characters[8].body==roll && Characters[8].health==roll*10 && Characters[8].weapon==random%14);
        for(unsigned skill=0;skill<6;++skill) check(((uint8_t *)&Characters[8])[4+skill]==(random&1));
        check(Characters[8].skillMedical==0xA5 && randomCalls==9 && diceCalls==1);
    }
    prepare(); KuritaAttackFlag=TRUE; EnemyMechDamageLevel=6; Mission_GenerateEnemies(0,0);
    check(randomCalls==10 && diceCalls==0 && EnemyMechDamageLevel==0);
    for(unsigned slot=0;slot<4;++slot) check(sameTemplate(&Mechs[4+slot],&MechRefs[5]));
    prepare(); Mission_GenerateEnemies(0,0x88); check(CombatantActive[4]==TRUE && CombatantActive[0]==FALSE && CombatantPackedX[16]==0x0D11 && CombatantPackedY[16]==0x7026);
    prepare(); PersistentState.bytes[0]=2; Mission_GenerateEnemies(1,0); check(CombatantPackedX[12]==0x0C72);
    prepare(); Mission_GenerateEnemies(0,0x8000); check(CombatantActive[0]==TRUE && CombatantActive[4]==FALSE && randomCalls==2);
    prepare(); crewRollStart=3; Mission_GenerateEnemies(4,0);
    check(randomCalls==10);
    for(unsigned slot=0;slot<4;++slot) {
        check(EnemyPilotSkill(&Mechs[4+slot],0)==crewRolls[slot*2]);
        check(EnemyPilotSkill(&Mechs[4+slot],1)==crewRolls[slot*2+1]);
    }
    check(Characters[0].skillGunnery==0xA5 && Characters[0].skillPiloting==0xA5);
    return 0;
}
