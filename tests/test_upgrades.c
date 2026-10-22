/* Sol: Complete-record witnesses for original upgrade packages. Test-only UI
 * adapters; selection/purchase/Yes-No are the actual preservation methods. */
#include "game.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned checks, balanceUpdates, rejected;
static uint16_t answer;
static void check(int condition)
{
    ++checks;
    if (!condition) { fprintf(stderr,"Upgrade check %u failed\n",checks); exit(1); }
}
void Draw_Top_Graphic_Sidebar(void) { TextRow = TextColumn = 0; }
uint16_t Display_Text_Mech_Names(void) { return SelectedMechId; }
void Display_Text_From_Memory(uint8_t *text) { (void)text; ++TextRow; }
void Display_Text_Dynamic_Value(uint16_t value) { (void)value; }
void Display_Text_CBill_Balance(void) { ++balanceUpdates; }
void Display_Text_4FA0_Value(void) { ++TextRow; }
void Display_Text_Shop_Cannot_Afford_Text(void) { ++rejected; }
void Drain_Pending_Keyboard_Input(void) { }
uint16_t Keyboard_Get_ASCII_Hex_Input(void) { return answer; }
void Citadel_Building_Dialogs(uint16_t action) { check(action == CitadelDialog_DisplayCBillBalance); }
static Mech *reset(uint16_t package, uint32_t funds)
{
    memset(Mechs,0,sizeof Mechs);
    SelectedMechId = 1;
    Mechs[1] = MechRefs[package % MechUpgrade_ChassisCount];
    Mechs[1].upgradePackageBase = (uint8_t)(package % MechUpgrade_ChassisCount);
    Mechs[1].upgradeLevelFlags = package >= MechUpgrade_ChassisCount ? MechUpgrade_StageOneInstalled : 0;
    CBills = funds;
    answer = 'Y';
    balanceUpdates = rejected = 0;
    return &Mechs[1];
}
static int heatAfterRound(const Mech *mech)
{
    uint8_t steps[AllCombatantCount] = {0};
    memset(Mechs,0,sizeof Mechs);
    Mechs[0] = *mech;
    memset(CombatMovementOrders,0,sizeof CombatMovementOrders);
    memset(CombatWeaponHeat,0,sizeof CombatWeaponHeat);
    memset(MechInfernoRoundsRemaining,0,sizeof MechInfernoRoundsRemaining);
    memset(TerrainOverlapRows,0,sizeof TerrainOverlapRows);
    memset(CombatWeaponTarget,255,sizeof CombatWeaponTarget);
    MechHeatLevel[0] = 30;
    Combat_Mech_HeatLevels(steps);
    return MechHeatLevel[0];
}
int main(void)
{
    static const uint16_t prices[8] = {11000,10400,12200,13800,15800,14000,13200,17600};
    Mech expected, before, *mech;
    check(sizeof MechRefs == 1000 && !strcmp((char *)MechRefs[0].name,"LOCUST         "));
    check(!memcmp(prices,MechUpgradeCostByPackage,sizeof prices));
    for (uint16_t package = 0; package < MechUpgrade_PackageCount; ++package) {
        mech = reset(package,50000);
        expected = *mech;
        switch (package) {
        case 0:
            expected.criticalSlots[14] = expected.criticalSlots[0] = Mech_Med_Laser;
            expected.currentAmmo[0] = expected.currentAmmo[1] = 255;
            expected.maxAmmo[0] = expected.maxAmmo[1] = 255;
            break;
        case 1:
            expected.criticalSlots[28] = Mech_Med_Laser;
            expected.currentAmmo[1] = expected.maxAmmo[1] = 255;
            memcpy(expected.currentArmour,MechRefs[0].currentArmour,11);
            memcpy(expected.maxArmour,MechRefs[0].maxArmour,11);
            break;
        case 2:
            expected.criticalSlots[16] = expected.criticalSlots[15] = Mech_Small_Laser;
            expected.criticalSlots[1] = expected.criticalSlots[0] = Mech_Small_Laser;
            memset(expected.currentAmmo,255,5); memset(expected.maxAmmo,255,5);
            break;
        case 3:
            expected.criticalSlots[23] = expected.criticalSlots[14] = Mech_Med_Laser;
            expected.currentAmmo[3] = expected.currentAmmo[2]; expected.maxAmmo[3] = expected.maxAmmo[2];
            expected.currentAmmo[1] = expected.currentAmmo[2] = 255;
            expected.maxAmmo[1] = expected.maxAmmo[2] = 255;
            memcpy(expected.currentArmour,CommandoStageOneArmour,11);
            memcpy(expected.maxArmour,CommandoStageOneArmour,11);
            break;
        case 4:
            expected.walkMove = 7; expected.engineHits = 0;
            expected.criticalSlots[22] = expected.criticalSlots[8] = Mech_Small_Laser;
            expected.currentAmmo[3] = expected.currentAmmo[4] = 255;
            expected.maxAmmo[3] = expected.maxAmmo[4] = 255;
            break;
        case 5:
            expected.jumpMove = 0;
            expected.criticalSlots[32] = expected.criticalSlots[0] = expected.criticalSlots[34] = Mech_Med_Laser;
            memset(expected.currentAmmo + 2,255,3); memset(expected.maxAmmo + 2,255,3);
            break;
        case 6:
            expected.jumpMove = 0;
            memcpy(expected.currentArmour,MechRefs[0].currentArmour,11);
            memcpy(expected.maxArmour,MechRefs[0].currentArmour,11);
            expected.criticalSlots[33] = expected.criticalSlots[32] = Mech_Med_Laser;
            expected.currentAmmo[5] = expected.currentAmmo[6] = 255;
            expected.maxAmmo[5] = expected.maxAmmo[6] = 255;
            break;
        case 7:
            memset(expected.criticalSlots + 28,Mech_Small_Laser,6);
            expected.criticalSlots[34] = Mech_Med_Laser;
            memset(expected.currentAmmo,255,10); memset(expected.maxAmmo,255,10);
            break;
        }
        expected.upgradeLevelFlags = package < 4 ? 1 : 3;
        Mechlube_Modify_Mech();
        check(MechModificationWorkflowEnabled && SelectedMechUpgradePackage == package && MechUpgradeCost == prices[package]);
        Mechlube_Upgrade_Mech();
        check(CBills == 50000U - prices[package] && balanceUpdates == 1 && rejected == 0);
        check(!memcmp(mech,&expected,sizeof expected));
        check(Mechs[0].tonnage == 0);
    }
    mech = reset(0,10999); before = *mech;
    Mechlube_Modify_Mech(); Mechlube_Upgrade_Mech();
    check(rejected == 1 && CBills == 10999 && !memcmp(mech,&before,sizeof before));
    mech = reset(0,50000); before = *mech; answer = 'N';
    Mechlube_Modify_Mech(); Mechlube_Upgrade_Mech();
    check(CBills == 50000 && balanceUpdates == 0 && !memcmp(mech,&before,sizeof before));
    mech = reset(0,50000); mech->upgradePackageBase = 201; before = *mech;
    Mechlube_Modify_Mech(); check(SelectedMechUpgradePackage == 8 && MechUpgradeCost == 3341);
    Mechlube_Upgrade_Mech();
    check(CBills == 46659 && !memcmp(mech,&before,sizeof before)); /* BUG-010. */
    mech = reset(0,50000); mech->upgradeLevelFlags = 3; SelectedMechUpgradePackage = 7; MechUpgradeCost = 123;
    Mechlube_Modify_Mech();
    check(!MechModificationWorkflowEnabled && SelectedMechUpgradePackage == 7 && MechUpgradeCost == 123);
    mech = reset(0,UINT32_MAX); before = *mech; SelectedMechUpgradePackage = 8; MechUpgradeCost = -1;
    Mechlube_Upgrade_Mech(); check(CBills == 0 && !memcmp(mech,&before,sizeof before));
    mech = reset(0,66000); *mech = MechRefs[MechRef_Chameleon];
    mech->currentArmour[0] = 3; mech->engineHits = 1;
    mech->criticalSlots[0] |= Component_Destroyed;
    mech->criticalSlots[32] |= Component_Destroyed;
    mech->currentAmmo[0] = 0; /* Preserve even unusual laser-state bytes. */
    expected = *mech;
    const uint8_t restoredStructure[8] = {8,12,12,3,16,8,12,12};
    memcpy(expected.currentStructure,restoredStructure,8);
    memcpy(expected.maxStructure,restoredStructure,8);
    expected.upgradeLevelFlags = MechUpgrade_ChameleonStructureInstalled;
    Mechlube_Modify_Mech();
    check(SelectedMechUpgradePackage == MECH_CHAMELEON_STRUCTURE_RESTORATION && MechUpgradeCost == 16000);
    Mechlube_Upgrade_Mech();
    check(CBills == 50000 && !memcmp(mech,&expected,sizeof expected));
    Mechlube_Upgrade_Mech();
    check(CBills == 50000 && !memcmp(mech,&expected,sizeof expected));
    expected.criticalSlots[32] = expected.criticalSlots[33] = Heat_Sink;
    expected.currentAmmo[7] = expected.currentAmmo[8] = 0;
    expected.maxAmmo[7] = expected.maxAmmo[8] = 0;
    expected.upgradeLevelFlags = MechUpgrade_ChameleonCoolingInstalled;
    Mechlube_Modify_Mech();
    check(SelectedMechUpgradePackage == MECH_CHAMELEON_FIELD_CONVERSION && MechUpgradeCost == 8000);
    Mechlube_Upgrade_Mech();
    check(CBills == 42000 && !memcmp(mech,&expected,sizeof expected));
    Mechlube_Upgrade_Mech(); /* Stale first-stage selector cannot charge twice. */
    check(CBills == 42000 && !memcmp(mech,&expected,sizeof expected));
    before = expected;
    expected.criticalSlots[14] = WeaponIndex_PPC + MechComponentToWeaponRecordBias;
    expected.criticalSlots[7] = expected.criticalSlots[8] = 0;
    expected.criticalSlots[21] = expected.criticalSlots[22] = 0;
    expected.currentAmmo[2] = before.currentAmmo[6];
    expected.maxAmmo[2] = before.maxAmmo[6];
    expected.currentAmmo[1] = expected.maxAmmo[1] = 255;
    memset(expected.currentAmmo + 3,0,7); memset(expected.maxAmmo + 3,0,7);
    expected.upgradeLevelFlags = MechUpgrade_ChameleonComplete;
    Mechlube_Modify_Mech();
    check(MechModificationWorkflowEnabled && SelectedMechUpgradePackage == MECH_CHAMELEON_PPC_CONVERSION && MechUpgradeCost == 28000);
    Mechlube_Upgrade_Mech();
    check(CBills == 14000 && !memcmp(mech,&expected,sizeof expected));
    Mechlube_Modify_Mech(); Mechlube_Upgrade_Mech();
    check(!MechModificationWorkflowEnabled && CBills == 14000 && !memcmp(mech,&expected,sizeof expected));
    {
        *mech = before;
        /* Damage to retained sinks/lasers must survive the PPC installation. */
        mech->criticalSlots[14] |= Component_Destroyed;
        mech->criticalSlots[32] |= Component_Destroyed;
        mech->currentAmmo[6] = 0;
        Mech damaged = *mech;
        CBills = 27999; answer = 'Y';
        Mechlube_Modify_Mech(); Mechlube_Upgrade_Mech();
        check(CBills == 27999 && !memcmp(mech,&damaged,sizeof damaged));
        CBills = 50000; answer = 'N';
        Mechlube_Modify_Mech(); Mechlube_Upgrade_Mech();
        check(CBills == 50000 && !memcmp(mech,&damaged,sizeof damaged));
        answer = 'Y'; Mechlube_Upgrade_Mech();
        Mech damagedExpected = expected;
        damagedExpected.criticalSlots[32] |= Component_Destroyed;
        damagedExpected.currentAmmo[2] = 0;
        check(CBills == 22000 && !memcmp(mech,&damagedExpected,sizeof damagedExpected));
    }
    *mech = MechRefs[MechRef_Chameleon]; before = *mech;
    SelectedMechUpgradePackage = MECH_CHAMELEON_PPC_CONVERSION;
    Mechlube_Upgrade_Mech();
    check(CBills == 22000 && !memcmp(mech,&before,sizeof before));
    /* Every custom package requires its own chassis and exact preceding stage. */
    const unsigned packages[] = {MECH_CHAMELEON_STRUCTURE_RESTORATION,
        MECH_CHAMELEON_FIELD_CONVERSION, MECH_CHAMELEON_PPC_CONVERSION};
    const unsigned prerequisites[] = {0,MechUpgrade_ChameleonStructureInstalled,
        MechUpgrade_ChameleonCoolingInstalled};
    const unsigned chameleonPrices[] = {16000,8000,28000};
    for (unsigned package = 0; package < 3; ++package) {
        for (unsigned stage = 0; stage < 8; ++stage) {
            if (stage == prerequisites[package]) continue;
            mech = reset(0,50000); *mech = MechRefs[MechRef_Chameleon];
            mech->upgradeLevelFlags = stage; before = *mech;
            SelectedMechUpgradePackage = packages[package];
            MechUpgradeCost = chameleonPrices[package];
            Mechlube_Upgrade_Mech();
            check(CBills == 50000 && !memcmp(mech,&before,sizeof before));
        }
        mech = reset(0,50000); mech->upgradeLevelFlags = prerequisites[package];
        before = *mech; SelectedMechUpgradePackage = packages[package];
        MechUpgradeCost = chameleonPrices[package]; Mechlube_Upgrade_Mech();
        check(CBills == 50000 && !memcmp(mech,&before,sizeof before));
        for (unsigned scenario = 0; scenario < 2; ++scenario) {
            mech = reset(0,scenario ? chameleonPrices[package]-1 : 50000);
            *mech = MechRefs[MechRef_Chameleon];
            mech->upgradeLevelFlags = prerequisites[package]; before = *mech;
            if (!scenario) answer = 'N';
            uint32_t balance = CBills;
            Mechlube_Modify_Mech(); Mechlube_Upgrade_Mech();
            check(CBills == balance && !memcmp(mech,&before,sizeof before));
            check(rejected == scenario);
        }
    }
    for (unsigned scenario = 0; scenario < 2; ++scenario) {
        mech = reset(0,scenario ? 15999 : 50000);
        *mech = MechRefs[MechRef_Chameleon]; before = *mech;
        if (!scenario) answer = 'N';
        Mechlube_Modify_Mech(); Mechlube_Upgrade_Mech();
        check(CBills == (scenario ? 15999U : 50000U) && !memcmp(mech,&before,sizeof before));
        check(rejected == scenario);
    }
    before = MechRefs[MechRef_Chameleon];
    expected = before;
    expected.criticalSlots[32] = expected.criticalSlots[33] = Heat_Sink;
    int stockHeat = heatAfterRound(&before);
    check(heatAfterRound(&expected) == stockHeat - 2);
    expected.criticalSlots[32] |= Component_Destroyed;
    check(heatAfterRound(&expected) == stockHeat - 1);
    printf("%u upgrade assertions passed\n",checks);
    return 0;
}
