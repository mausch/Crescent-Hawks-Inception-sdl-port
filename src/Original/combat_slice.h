#include <assert.h>
#include <string.h>

/* Private damage queue for BTECH_1AE8_MECHANICS.c. All attack eligibility and
 * rolls are committed before any damage, death, or ejection is applied. */
typedef struct CombatSliceAttack {
    uint16_t attacker, target, weapon, direction, record;
    uint16_t impact, applied, repeats, damage, location, personnel;
    uint16_t fire, fireX, fireY, mapImpact, impactX, impactY;
    uint16_t textColour, messageCount, messageBytes;
    uint16_t messageOffsets[32];
    uint8_t messageText[1024];
} CombatSliceAttack;

/* DynamicString is reused while later attacks are rolled. Own the bytes,
 * rather than retaining pointers into scratch storage or mutable records. */
static void Combat_Queue_Slice_Message(CombatSliceAttack *a, const uint8_t *text)
{
    size_t bytes = strlen((const char *)text) + 1;
    assert(a->messageCount < 32 && a->messageBytes + bytes <= sizeof a->messageText);
    if (a->messageCount >= 32 || a->messageBytes + bytes > sizeof a->messageText)
        return;
    a->messageOffsets[a->messageCount++] = a->messageBytes;
    memcpy(a->messageText + a->messageBytes, text, bytes);
    a->messageBytes += (uint16_t)bytes;
}

static void Combat_Queue_Slice_Location(CombatSliceAttack *a, uint16_t offset)
{
    for (uint16_t location = 0; location < CombatArmourLocationCount; ++location)
        if ((uint16_t)(int16_t)CombatArmourLocationOffsets[location] == offset) {
            Append_Large_Text_To_Memory(DynamicString, ArmourLocationText[location]);
            Append_Text_To_Memory(DynamicString, (uint8_t *)".");
            Combat_Queue_Slice_Message(a, DynamicString);
            break;
        }
}

static void Combat_Present_Slice_Attack(const CombatSliceAttack *a)
{
    if (CombatMessageVerbosity != CombatMessage_None) {
        Menu_Memory_Variables(4);
        Draw_Top_Graphic_Sidebar();
    }
    TextColour = a->textColour;
    for (uint16_t message = 0; message < a->messageCount; ++message)
        Combat_CombatMessageVerbosityFilter((uint8_t *)a->messageText + a->messageOffsets[message]);
    Combat_AudioVisual_Effects(a->attacker, a->target, a->weapon, a->direction,
        a->impact, FALSE, FALSE, a->record, a->applied, CombatSavedTileCache);
}

static void Combat_Apply_Slice_Attack(const CombatSliceAttack *a, uint16_t encounter)
{
    uint16_t dead = FALSE, destroyed = FALSE;
    uint16_t remaining = a->damage, location = a->location;
    TextColour = a->textColour;
    MechDestroyedFlag = CombatNotificationLatch = ShowArmShotOffAnimation = FALSE;
    if (a->fire) Combat_Random_CreateFire(a->target, a->fireX, a->fireY);
    if (a->mapImpact)
        Register_Persistent_Map_Effect(Sprite_Impact_Large, a->impactX, a->impactY);
    if (a->personnel) {
        int16_t damage = (int16_t)remaining;
        for (uint16_t repeat = 0; repeat < a->repeats; ++repeat) {
            int16_t armour = (int8_t)Characters[a->record].armourValue;
            if (armour != 0) {
                if (armour >= (damage >> 1)) {
                    damage >>= 1;
                    Characters[a->record].armourValue -= (uint8_t)damage;
                } else {
                    damage -= armour;
                    Characters[a->record].armourValue = 0;
                }
            }
            int16_t health = (int8_t)Characters[a->record].health;
            Characters[a->record].health = health <= damage ? 0 : health - damage;
        }
        dead = Characters[a->record].health == 0;
    } else {
        uint8_t *record = (uint8_t *)&Mechs[a->record];
        while (remaining != 0 && record[0] != 0xFF) {
            if (a->weapon == WeaponIndex_Inferno) {
                MechInfernoRoundsRemaining[a->record] = 3;
                break;
            }
            uint16_t available = record[location];
            if (remaining > available) {
                remaining -= available;
                record[location] = 0;
                if (available != 0 && (location == 0x1C || location == 0x21))
                    ShowArmShotOffAnimation = TRUE;
                if (location >= 0x1C && location <= 0x23)
                    Combat_Critical_Mech_Damage(a->record, location);
                location = Combat_StructureHit(location);
                if (MechDestroyedFlag) {
                    Combat_Mech_Eject(a->target);
                    CombatNotificationLatch = FALSE;
                    destroyed = TRUE;
                }
            } else {
                if (remaining == available && (location == 0x1C || location == 0x21))
                    ShowArmShotOffAnimation = TRUE;
                record[location] -= (uint8_t)remaining;
                remaining = 0;
                if (location >= 0x1C && location <= 0x23)
                    Combat_Critical_Mech_Damage(a->record, location);
                if ((location == 0x1F || location == 0x20) && record[location] == 0) {
                    Combat_Mech_Eject(a->target);
                    CombatNotificationLatch = FALSE;
                    destroyed = TRUE;
                    if (a->target == 0 && encounter != 0) Combat_Clear_Movement_Plans();
                }
            }
        }
    }
    Combat_Remove_Slice_Casualties(a->target, dead, destroyed, a->record, CombatSavedTileCache);
    if (ShowArmShotOffAnimation) {
        CombatNotificationLatch = FALSE;
        if (CombatDisplayGraphics != CombatGraphics_None &&
            (a->target != 13 || ArenaRentalMechMode == FALSE))
            Display_Animation_Scene(AnimationO16_WaspLostArm, AnimationPlayback_RestoreGameView);
    }
    if (CombatNotificationLatch &&
        (int16_t)(int8_t)Characters[Character_Jason].mechAssignment == a->record &&
        CombatDisplayGraphics != CombatGraphics_None)
        Display_Animation_Scene(AnimationO05_NeuroSmoking, AnimationPlayback_RestoreGameView);
    GameSpeed_RateControl();
    ShowArmShotOffAnimation = FALSE;
}
