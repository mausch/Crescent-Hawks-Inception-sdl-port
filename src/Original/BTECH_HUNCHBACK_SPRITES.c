#include "game.h"
#include <string.h>

/* Port-owned art, not a new native family number. Family/frame BYTEs and the
 * 125-byte Mech record stay save-compatible. Native animation streams, muzzle
 * offsets, shared impacts/projectiles/wrecks and all other chassis stay intact.
 * Headers are mutable because both original compositors temporarily clip them. */
#include "BTECH_HUNCHBACK_ART.inc"

uint8_t *Resolve_Mech_Combat_Sprite(uint16_t combatantId,uint16_t spriteId)
{
    uint16_t record;
    if(combatantId<LanceSize) record=combatantId;
    else if(combatantId>=Enemy_All_CombatantId_Range_First &&
        combatantId<Enemy_Infantry_CombatantId_Range_First)
        record=(uint16_t)(combatantId-Enemy_All_CombatantId_Range_First+Enemy_Mech_Record_First);
    else return CombatSpritePointers[spriteId];

    if(CombatantSpriteFamilyOffset[combatantId]==MECH_Sprite_COMMANDO &&
        spriteId>=MECH_Sprite_COMMANDO && spriteId<MECH_Sprite_COMMANDO+HunchbackSpriteCount &&
        memcmp(Mechs[record].name,"HUNCHBACK      ",MechNameLength)==0)
        return hunchbackSprites[spriteId-MECH_Sprite_COMMANDO];
    return CombatSpritePointers[spriteId];
}
