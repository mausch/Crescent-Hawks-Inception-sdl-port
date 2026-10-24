#include "game.h"
#include "combat_slice.h"

/* The roll phase records presentation; only the application phase displays it. */
#define Combat_CombatMessageVerbosityFilter(text) Combat_Queue_Slice_Message(&PendingAttack, (text))
#define Combat_DisplayText_ArmourHitLocation(offset) Combat_Queue_Slice_Location(&PendingAttack, (offset))

/* Sol: complete original1AE8:000C..12C6 template conversion. Native entry
 * Incidental first-use effects flag residue is normalized to no impact;
 * subsequent native same-call hit/miss carry-over remains (BUG-023). */
void Combat_Mechanics(uint16_t PrearrangedEncounter)
{
	// Sol: movement cursors, proficiency, narration, view restoration and missed
	// Sol: shot sentinel corrected against ASM2026-09-18.
	// Sol: 1AE8:000C executes twelve interleaved movement/fire slices, not
	// Sol: twelve turns. See docs/phase4/COMBAT_MOVEMENT_AND_WEAPON_LINKAGE.md.
	// Sol: BYTE cursor0078 counts consumed pairs; stack BYTE[24] at BP-78
	// Sol: counts successful nonzero moves for target penalty and final heat.
	uint16_t AttackingCharacterRecordId=0; /* Sol: valid actor branches assign before use. */
	// Sol: Native BP-56 attack-applied flag can remain unassigned when
	// Sol:09F1 jumps to effects for a high-bit weapon encoding versus a mech.
	// Sol: A host uninitialized local cannot reproduce native residual stack state.
	// Sol: After an ordinary hit/miss assigns BP-56, later bit80 personnel
	// Sol: attacks versus Mechs inherit that result for effects without damage.
	// Sol: Do not reset this flag per actor or weapon; see ORIGINAL_GAME_BUGS BUG-023.
	// Sol: Portable presentation policy: no invented hit sound/impact before an
	// Sol: actual attack assigns the flag. This entry initialization is not native.
	// Sol: The flag only controls effects; the skipped branch still does no damage.
	uint16_t AttackApplied=FALSE;

	// Sol: Actual stack BYTE[24] SS:BP-78, separate from global cursor0078.
	uint8_t SuccessfulMovementSteps[AllCombatantCount];

	uint16_t InitialCameraX = CrescentHawkMapPositionX; //A44B - Character Position X
	uint16_t InitialCameraY = CrescentHawkMapPositionY; //A44D - Character position Y

	uint8_t InitialCombatantId = 0x00;
	do
	{
		SuccessfulMovementSteps[InitialCombatantId] = 0;
		CombatMovementStepCursor[InitialCombatantId] = 0;
		InitialCombatantId++;
	} while (InitialCombatantId < AllCombatantCount);

	for (uint8_t RoundSlice = 0x00; RoundSlice < CombatMovementSlices; ++RoundSlice)
	{
		if (MainCharactersAlive != FALSE) //014A = Bool_MainCharactersAlive
		{
			uint8_t RedrawMovement = FALSE;

			if (RoundSlice == 0x00)
				RedrawMovement = TRUE;

			for(int16_t MovingCombatantId = 0x00; MovingCombatantId < AllCombatantCount; MovingCombatantId++)
			{
				if (CombatantActive[MovingCombatantId] != FALSE) //a406A = Visible / On Map?
				{
					// Sol:1037 CBWs cursor BYTE BEFORE forming the wrapped WORD address.
					uint16_t MovementPairOffset = (uint16_t)(
						(int16_t)(int8_t)CombatMovementStepCursor[MovingCombatantId] * 2 +
						MovingCombatantId * CombatMovementPlanBytesPerUnit); //24 BYTEs = twelve delta-X/delta-Y pairs per combatant

					int16_t MovementDeltaX = (int8_t)CombatMovementPlanBytes[MovementPairOffset]; //DS:40B4
					int16_t MovementDeltaY = (int8_t)CombatMovementPlanBytes[(uint16_t)(MovementPairOffset + 1)]; //DS:40B5 is +1 alias

					if (MovementDeltaX != CombatMovementPlanEnd && (MovementDeltaY != CombatMovementPlanEnd && (MovementDeltaX | MovementDeltaY) != 0x00))
					{
						uint16_t SavedAnchorX = CrescentHawkMapPositionX; //A44B - Character Position X
						uint16_t SavedAnchorY = CrescentHawkMapPositionY; //A44D - Character position Y

						Combat_Move_Position(CombatantPackedX[MovingCombatantId], CombatantPackedY[MovingCombatantId]); //DS:4004/4036 WORDs

						Offset_Packed_Position(MovementDeltaX, MovementDeltaY);

						if (Combat_Check_Occupancy_And_Crush(MovingCombatantId, MovementDeltaX, MovementDeltaY, TRUE) == FALSE) // Sol:1631:16AB four WORDs(id,dx,dy,crush=1), ADD SP8 verified at execution call. Candidate anchor already offset;0 result accepts movement even after crushing.
						{
							CombatMovementStepCursor[MovingCombatantId]++;

							CombatantPackedX[MovingCombatantId] = CrescentHawkMapPositionX;
							CombatantPackedY[MovingCombatantId] = CrescentHawkMapPositionY;

							// Sol:2ED1 is a prebiased base into the11-BYTE delta table2ECC.
							// Sol: Multiply signed delta by4; shifting a negative C value is undefined.
							uint8_t MovementDirection = MovementStepDirectionByDelta[MovementDeltaY * MovementDeltaDirectionRowStride + MovementDeltaX + MovementDeltaDirectionBias];
							RedrawMovement = TRUE;


							if (MovementDeltaX != 0x00 || MovementDeltaY != 0x00)
							{
								SuccessfulMovementSteps[MovingCombatantId] += 1;
								uint8_t PreviousAnimationDirection = CombatantAnimationSelector[MovingCombatantId];

								if (PreviousAnimationDirection != MovementDirection)
								{
									if (MovingCombatantId >= Friendly_Infantry_Combatant_Range_First
									 && MovingCombatantId < Enemy_All_CombatantId_Range_First
									 || MovingCombatantId >= Enemy_Infantry_CombatantId_Range_First)
										MovementDirection += CompassDirectionCount;

									// Sol:1167 stores the complete16:16 FAR cursor, not a BYTE
									// Sol: of its offset or the address of its segment cell.
									CombatantAnimationCursors[MovingCombatantId] =
										MovementDirection >= CompassDirectionCount ? FriendlyInfantryWalkAnimationByDirection[MovementDirection - CompassDirectionCount] :
										FriendlyMechWalkAnimationByDirection[MovementDirection];
								}

								// Sol:1191 stores returned AL, not the old facing/offset BYTE.
								CombatantSpriteFrame[MovingCombatantId] =
									Advance_Combatant_Animation_Stream(MovingCombatantId);
								CombatantMovementDirection[MovingCombatantId] = CombatantAnimationSelector[MovingCombatantId];
							}
						}
						CrescentHawkMapPositionX = SavedAnchorX; //A44B - Character Position X
						CrescentHawkMapPositionY = SavedAnchorY; //A44D - Character position Y
					}
				}
			}

			if (RedrawMovement != FALSE
		    && CombatDisplayGraphics != CombatGraphics_None) //w2E3A - See Combat Graphics [Menu Option]
			{
				int16_t ViewCombatantId = Friendly_Infantry_Combatant_Range_First;

				if ((int8_t)Characters[Character_Jason].mechAssignment < Character_OnFoot) // Sol:1208 CBW after signed BYTE comparison
					ViewCombatantId = (int8_t)Characters[Character_Jason].mechAssignment;

				Move_Map_View_To_Packed_Position(CombatantPackedX[ViewCombatantId], CombatantPackedY[ViewCombatantId]); //PosX, PosY args //t4004 - CharacterPosX_Mech //w4036 - CharacterPosY_Mech

				uint16_t SavedAnchorX = CrescentHawkMapPositionX; //Sol: 1AE8:1224/122E X/Y are not flipped
				uint16_t SavedAnchorY = CrescentHawkMapPositionY;

				PosXY_OffsetGrid(SavedAnchorX, SavedAnchorY);

				Combat_Copy_Map_Cache(CombatSavedTileCache, FALSE); // Sol:1224..1242 pushes FAR3092:4314, not cache contents.

				Update_Animated_Map_Tiles();
				Copy_Data_To_GraphicsMemory();
				Draw_Menu_MultiSelect();
				EGA_DrawBox_Wrapper();
			}
			CombatSliceAttack SliceAttacks[AllCombatantCount];
			uint16_t SliceAttackCount = 0;
			uint8_t CombatantId = 0x00;

			while (CombatantId < AllCombatantCount)
			{
				if (MainCharactersAlive != FALSE //014A = Bool_MainCharactersAlive
			    && CombatantActive[CombatantId] != FALSE) //a406A = Visible / On Map?
				{
					uint8_t WeaponSlot = 0x00;
					while (WeaponSlot < CombatWeaponTargetSlots)
					{
						MechDestroyedFlag = FALSE;
						CombatNotificationLatch = FALSE;

						CombatSliceAttack PendingAttack = {0};
						uint16_t PlayTargetImpactStream = 0x00; // Sol: native BP-6 WORD, impact stream flag.
						uint8_t TargetCombatantId = CombatWeaponTarget[(CombatantId * CombatWeaponTargetSlots) + WeaponSlot];

						if (CombatantId < Friendly_Infantry_Combatant_Range_First && (int8_t)MechHeatLevel[CombatantId] >= MechHeatShutdownLevel)
							TargetCombatantId = 0xFF;

						if (CombatantId >= Enemy_All_CombatantId_Range_First)
						{
							if (CombatantId < Enemy_Infantry_CombatantId_Range_First)
							{
								if ((int8_t)MechHeatLevel[CombatantId - Enemy_Infantry_Record_First] >= MechHeatShutdownLevel)
									TargetCombatantId = 0xFF;
							}
						}

						if ((TargetCombatantId & 0x80) == FALSE)
						{
							uint16_t WeaponIndex; //Sol: WORD table index/sentinel, not ammo amount
							uint16_t SavedAnchorX = CrescentHawkMapPositionX; //A44B - Character Position X
							uint16_t SavedAnchorY = CrescentHawkMapPositionY; //A44D - Character position Y

							CrescentHawkMapPositionX = CombatantPackedX[CombatantId];  //A44B - Character Position X  //t4004 - CharacterPosX_Mech
							CrescentHawkMapPositionY = CombatantPackedY[CombatantId]; //A44D - Character position Y //w4036 - CharacterPosY_Mech

							// Sol: Reconstructed weapon resolution 1AE8:0059..00E9/0EAB.
							// Sol: Preserve combatant ID; normalize only the record lookup.
							if ((CombatantId >= Friendly_Infantry_Combatant_Range_First && CombatantId < Enemy_All_CombatantId_Range_First) || CombatantId >= Enemy_Infantry_CombatantId_Range_First)
							{
								uint16_t InfantryRecordId = CombatantId >= Enemy_Infantry_CombatantId_Range_First ? CombatantId - Enemy_Infantry_Record_First : CombatantId - Friendly_Infantry_Combatant_Range_First;
								WeaponIndex = (int16_t)(int8_t)Characters[InfantryRecordId].weapon;
							}
							else if (WeaponSlot == CombatKickTargetSlot)
							{
								uint16_t MechRecordId = CombatantId >= Enemy_All_CombatantId_Range_First ? CombatantId - Enemy_Infantry_Record_First : CombatantId;
								if ((Mechs[MechRecordId].currentActuators[0] & MechLegPrimaryActuatorMask)
									&& (Mechs[MechRecordId].currentActuators[1] & MechLegPrimaryActuatorMask))
									WeaponIndex = WeaponIndex_Kick; //Sol: kick TABLE INDEX
								else
								{
									CombatWeaponTarget[CombatantId * CombatWeaponTargetSlots + CombatKickTargetSlot] = 0xFF;
									WeaponIndex = WeaponInfantryAttackFlag; //Sol: disabled sentinel
								}
							}
							else
							{
								uint16_t MechRecordId = CombatantId >= Enemy_All_CombatantId_Range_First ? CombatantId - Enemy_Infantry_Record_First : CombatantId;
								WeaponIndex = Combat_Get_Weapon_Index_For_Ordinal(MechRecordId, WeaponSlot);
								if (WeaponIndex == 0x00FF)
									CombatWeaponTarget[CombatantId * CombatWeaponTargetSlots + WeaponSlot] = 0xFF;
							}
							if ((WeaponIndex & WeaponInfantryAttackFlag) != FALSE)
							{
Restore_Attack_Anchor:
								CrescentHawkMapPositionX = SavedAnchorX; //A44B - Character Position X
								CrescentHawkMapPositionY = SavedAnchorY; //A44D - Character position Y
								goto Advance_Weapon_Slot;
							}

							if (CombatantActive[TargetCombatantId] == FALSE) //a406A = Visible / On Map?
							{
								CombatWeaponTarget[(CombatantId * CombatWeaponTargetSlots) + WeaponSlot] = 0xFF; //Sol: inactive target, 1AE8:0DAF
								goto Restore_Attack_Anchor;
							}

							uint8_t WeaponRangeBracket = (uint8_t)Combat_Calculate_RangeBracket(TargetCombatantId, WeaponIndex);

							if (WeaponRangeBracket >= RangeBracket_OutOfRange)
								goto Restore_Attack_Anchor;

							 //A44B - Character Position X  //A44D - Character position Y //a4036 = PosY
							//Compass takes X0, Y0, X1, Y1 and does comparison direction
							uint8_t AttackDirection = (uint8_t)Get_Target_Compass_Direction(CrescentHawkMapPositionX, CrescentHawkMapPositionY, CombatantPackedX[TargetCombatantId], CombatantPackedY[TargetCombatantId]); //t4004 - CharacterPosX_Mech //w4036 - CharacterPosY_Mech

							//If Compass invalid direction reset to zero?
							if (AttackDirection == 0xFF) //Why are you 0xFF?
								AttackDirection = 0x00;

							CrescentHawkMapPositionX = SavedAnchorX; //A44B - Character Position X
							CrescentHawkMapPositionY = SavedAnchorY; //A44D - Character position Y

							Move_Map_View_To_Packed_Position(CombatantPackedX[CombatantId], CombatantPackedY[CombatantId]); //PosX, PosY args //t4004 - CharacterPosX_Mech //w4036 - CharacterPosY_Mech

							if (Combat_Check_Terrain_Path(CombatantId, TargetCombatantId, CombatantPackedX[TargetCombatantId], CombatantPackedY[TargetCombatantId]) != 0) //Sol: four WORD arguments, 1AE8:01B5
							{
								if (CombatantId >= Friendly_Infantry_Combatant_Range_First && CombatantId < Enemy_All_CombatantId_Range_First)
								{
									uint16_t WeaponSkillCategory = WeaponStats[WeaponIndex].skillType; //01D0 zero-extended BYTE
									uint16_t PartyMemberId = CombatantId - Friendly_Infantry_Combatant_Range_First;

									if ((int16_t)(int8_t)LastWeaponProficiencyCategory[PartyMemberId] == WeaponSkillCategory)
									{
										WeaponProficiencyUseCount[PartyMemberId] += 1;

										if (WeaponProficiencyUseCount[PartyMemberId] == 0x00)
										{
											// Sol:01FF C5D4+actor*17+category = C614+(actor-4)*17+4+category.
											// Sol: This is the address of the skill BYTE, not its value as a pointer.
											uint8_t *SkillToImprove =
												(uint8_t *)&Characters[PartyMemberId] + offsetof(Character,skillBowsAndBlade) + WeaponSkillCategory;
											if ((int8_t)*SkillToImprove < 4) //excellent ceiling; native signed BYTE test
												++*SkillToImprove;
										}
									}
									else
									{
										LastWeaponProficiencyCategory[PartyMemberId] = (uint8_t)WeaponSkillCategory;
										WeaponProficiencyUseCount[PartyMemberId] = 0x00;
									}
								}

								// Sol: 1AE8:0239..0296 consumes ammo AFTER range and line
								// Sol: checks, BEFORE hit roll. FF means unlimited.
								if ((CombatantId < Friendly_Infantry_Combatant_Range_First || (CombatantId >= Enemy_All_CombatantId_Range_First && CombatantId < Enemy_Infantry_CombatantId_Range_First))
									&& WeaponSlot < CombatKickTargetSlot)
								{
									uint16_t MechRecordId = CombatantId >= Enemy_All_CombatantId_Range_First ? CombatantId - Enemy_Infantry_Record_First : CombatantId;
									uint8_t *Ammo = (uint8_t *)&Mechs[MechRecordId] + offsetof(Mech,currentAmmo) + WeaponSlot;
									// Sol: ASM allows ordinal10: reads/decrements WalkMove
									// Sol: at31. Preserve original BUG-012; do not call it ammo.
									if (*Ammo != 0xFF) --*Ammo;
								}

								// Sol: 0297..0428: signed WORD target number for 2D6, not a percentage.
								int16_t AttackTargetNumber = WeaponRangeBracket * 2 + 4; // Sol: base 4; each range bracket adds 2 (4/6/8).
								if (WeaponIndex == WeaponIndex_Kick)
								{
									// Sol: preserve BUG-014: both ASM calls pass combatant ID,
									// Sol: despite computing normalized mech record ID in BP-7A.
									AttackTargetNumber = 3; // Sol: kick's base target number, before actuator penalties.
									AttackTargetNumber += Mech_Count_Missing_Low_Actuator_Bits(CombatantId, MECH_Offset_CurrentActuators_Left);
									AttackTargetNumber += Mech_Count_Missing_Low_Actuator_Bits(CombatantId, MECH_Offset_CurrentActuators_Right);
								}
								// Sol: mode FF sign-extends to -1, so contributes zero.
								AttackTargetNumber += (int8_t)CombatMovementOrders[CombatantId * CombatMovementOrderBytes] + 1; // Sol: 48-byte order row; walk/run/jump modes 0/1/2 add 1/2/3.
								AttackTargetNumber += (int8_t)CombatTargetMovementPenalty[(int8_t)SuccessfulMovementSteps[TargetCombatantId]];
								/* Port add (tabletop-derived, no EXE counterpart):
								 * a target that moved this round in jump mode gains
								 * the tabletop jumping +1 on top of the movement
								 * penalty. Order rows carry one mode for the whole
								 * round (a planning mode switch wipes the row and
								 * the enemy planner writes a single order), so the
								 * first order byte identifies the mode, as the heat
								 * code already assumes at 1631:0C63. */
								if ((int8_t)CombatMovementOrders[TargetCombatantId * CombatMovementOrderBytes] == (int8_t)MovementMode_Jump)
									++AttackTargetNumber;
								if (CombatantId >= Friendly_Infantry_Combatant_Range_First && CombatantId < Enemy_All_CombatantId_Range_First) // Sol: friendly infantry IDs 4..11.
									AttackingCharacterRecordId = CombatantId - Friendly_Infantry_Combatant_Range_First; // Sol: character records 0..7.
								if (CombatantId >= Enemy_Infantry_CombatantId_Range_First) // Sol: enemy infantry IDs 16..23.
									AttackingCharacterRecordId = CombatantId - Enemy_Infantry_Record_First; // Sol: character records 8..15.
								if (CombatantId < Friendly_Infantry_Combatant_Range_First || (CombatantId >= Enemy_All_CombatantId_Range_First && CombatantId < Enemy_Infantry_CombatantId_Range_First)) // Sol: friendly mechs 0..3 or enemy mechs 12..15.
								{
									// Sol: unsigned PilotId BYTE and SensorHits BYTE values, not addresses.
									// Sol: enemy biased C3B5/C3B3, 008A/0066 normalize to record 4..7.
									uint16_t MechRecordId = CombatantId >= Enemy_All_CombatantId_Range_First ? CombatantId - Enemy_Infantry_Record_First : CombatantId; // Sol: enemy IDs 12..15 map to mech records 4..7; bias is 8.
									AttackingCharacterRecordId = Mechs[MechRecordId].pilotId;
									if (Mechs[MechRecordId].sensorHits != 0)
										AttackTargetNumber += 2; // Sol: damaged sensors impose a +2 target-number penalty.
									CombatWeaponHeat[MechRecordId] += WeaponStats[WeaponIndex].heat & 0x0F; // Sol: low nibble stores firing heat; upper bits encode effects.
									// Sol: signed current heat BYTE; weapon heat is accumulated separately.
									int16_t CurrentHeat = (int8_t)MechHeatLevel[MechRecordId];
									if (CurrentHeat >= MechHeatToHitPenaltyFirst) ++AttackTargetNumber; // Sol: heat 8: cumulative penalty +1.
									if (CurrentHeat >= MechHeatToHitPenaltySecond) ++AttackTargetNumber; // Sol: heat 13: cumulative penalty +2.
									if (CurrentHeat >= MechHeatToHitPenaltyThird) ++AttackTargetNumber; // Sol: heat 17: cumulative penalty +3.
									if (CurrentHeat >= MechHeatToHitPenaltyFourth) ++AttackTargetNumber; // Sol: heat 24: cumulative penalty +4.
								}
								PendingAttack.textColour = CombatantId >= Enemy_All_CombatantId_Range_First
									? EGA_BrightYellow : EGA_BrightWhite;

								// Sol: 1AE8:0460..071B. Native names are FAR string pointers,
								// not WORD values or addresses of pointer-table cells. Name IDs use CBW.
								if (CombatantId < Enemy_All_CombatantId_Range_First)
								{
								    if (CombatMessageVerbosity == CombatMessage_Verbose)
								    {
								        int16_t AttackerNameId = (int8_t)Characters[AttackingCharacterRecordId].name;
								        Combat_CombatMessageVerbosityFilter(CharacterNames[AttackerNameId]);
								    }
								    if (CombatantId < Friendly_Infantry_Combatant_Range_First)
								    {
								        if (CombatMessageVerbosity == CombatMessage_Verbose)
								            Combat_CombatMessageVerbosityFilter((uint8_t *)"'s Mech "); // DS3EDB:3E44
								        if (WeaponIndex == WeaponIndex_Kick)
								        {
								            // Sol: this historical helper copies (strcpy), despite its name.
								            Append_Large_Text_To_Memory(DynamicString, (uint8_t *)"kicks");
								            if (CombatMessageVerbosity == CombatMessage_Brief)
								                Append_Large_Text_To_Memory(DynamicString, (uint8_t *)"Kick");
								            Combat_CombatMessageVerbosityFilter(DynamicString);
								        }
								        else if (CombatMessageVerbosity == CombatMessage_Verbose)
								            Combat_CombatMessageVerbosityFilter((uint8_t *)"uses a ");
								    }
								    else if (CombatMessageVerbosity == CombatMessage_Verbose)
								        Combat_CombatMessageVerbosityFilter((uint8_t *)" uses a "); // DS3EDB:3E60, both spaces
								}
								else
								{
								    if (CombatMessageVerbosity == CombatMessage_Verbose)
								        Combat_CombatMessageVerbosityFilter((uint8_t *)"An enemy ");
								    if (CombatantId >= Enemy_Infantry_CombatantId_Range_First)
								    {
								        if (CombatMessageVerbosity == CombatMessage_Verbose)
								            Combat_CombatMessageVerbosityFilter((uint8_t *)"human uses a ");
								    }
								    else
								    {
								        if (CombatMessageVerbosity == CombatMessage_Verbose)
								            Combat_CombatMessageVerbosityFilter((uint8_t *)"Mech "); // DS3EDB:3E81, trailing space
								        if (WeaponIndex == WeaponIndex_Kick)
								        {
								            Append_Large_Text_To_Memory(DynamicString, (uint8_t *)"kicks");
								            if (CombatMessageVerbosity == CombatMessage_Brief)
								                Append_Large_Text_To_Memory(DynamicString, (uint8_t *)"Kick"); // DS3EDB:3E95
								            Combat_CombatMessageVerbosityFilter(DynamicString);
								        }
								        else if (CombatMessageVerbosity == CombatMessage_Verbose)
								            Combat_CombatMessageVerbosityFilter((uint8_t *)"uses a ");
								    }
								}
								// Sol: 05BF calls the filter even when brief/off; the filter decides visibility.
								if (WeaponIndex != WeaponIndex_Kick)
								    Combat_CombatMessageVerbosityFilter(&WeaponStats[WeaponIndex].name[0]);
								if (CombatMessageVerbosity == CombatMessage_Verbose)
								    Combat_CombatMessageVerbosityFilter((uint8_t *)" on ");

								if (TargetCombatantId < Enemy_All_CombatantId_Range_First)
								{
								    if (TargetCombatantId >= Friendly_Infantry_Combatant_Range_First)
								    {
								        int16_t TargetNameId = (int8_t)Characters[TargetCombatantId - Friendly_Infantry_Combatant_Range_First].name;
								        Append_Large_Text_To_Memory(DynamicString, CharacterNames[TargetNameId]);
								    }
								    else
								    {
								        uint16_t TargetPilotId = Mechs[TargetCombatantId].pilotId;
								        int16_t TargetPilotNameId = (int8_t)Characters[TargetPilotId].name;
								        Append_Large_Text_To_Memory(DynamicString, CharacterNames[TargetPilotNameId]);
								        Append_Text_To_Memory(DynamicString, (uint8_t *)"'s Mech"); // DS3EDB:3E9F
								    }
								    // Sol: native068C concatenates ONE period; scratch is built even when off.
								    Append_Text_To_Memory(DynamicString, (uint8_t *)".");
								    if (CombatMessageVerbosity == CombatMessage_Verbose)
								        Combat_CombatMessageVerbosityFilter(DynamicString);
								}
								else if (CombatMessageVerbosity == CombatMessage_Verbose)
								{
								    // Sol: arena actor13 is the spectator; native rereads rental mode after
								    // output calls. Keep these independent gates, not a cached if/else.
								    if (ArenaRentalMechMode == FALSE || TargetCombatantId != 13)
								    {
								        Combat_CombatMessageVerbosityFilter((uint8_t *)"an enemy ");
								        if (TargetCombatantId < Enemy_Infantry_CombatantId_Range_First)
								            Combat_CombatMessageVerbosityFilter((uint8_t *)"Mech.");
								        else
								            Combat_CombatMessageVerbosityFilter((uint8_t *)"human.");
								    }
								    if (ArenaRentalMechMode != FALSE && TargetCombatantId == 13)
								        Combat_CombatMessageVerbosityFilter((uint8_t *)"a spectator."); // DS3EDB:3EC0, native0712
								}
								uint16_t TargetRecordId; // Sol: native BP-0C WORD, normalized target record (Mech OR character).
								// Sol: 071B: unsigned skill index at 3EDB:2EE8; signed skill BYTE
								// Sol: at Infantry+04 (C618), seven contiguous skills. No original bounds check.
								uint8_t SkillIndex = WeaponStats[WeaponIndex].skillType;
								/* Port fix: enemy templates have pilotId=0 (Jason), and neither
								 * encounter generator supplies initialized Mech pilot skills.
								 * New encounters store independent crew skills in the enemy record;
								 * legacy saves use Average without consuming infantry slots. */
								if (CombatantId >= Enemy_All_CombatantId_Range_First && CombatantId < Enemy_Infantry_CombatantId_Range_First)
									AttackTargetNumber -= EnemyPilotSkill(&Mechs[CombatantId-Enemy_Infantry_Record_First], WeaponIndex == WeaponIndex_Kick);
								else
									AttackTargetNumber -= (int8_t)*((uint8_t *)&Characters[AttackingCharacterRecordId] + offsetof(Character,skillBowsAndBlade) + SkillIndex);
								//If Target is infantry
								if (TargetCombatantId >= Friendly_Infantry_Combatant_Range_First && TargetCombatantId < Enemy_All_CombatantId_Range_First || TargetCombatantId >= Enemy_Infantry_CombatantId_Range_First)
								{
									TargetRecordId = TargetCombatantId - Friendly_Infantry_Combatant_Range_First; // Sol: friendly infantry IDs 4..11 -> character records 0..7.
									if (TargetCombatantId >= Enemy_Infantry_CombatantId_Range_First) //If Target Enemy Infantry
										TargetRecordId = TargetCombatantId - Enemy_Infantry_Record_First; // Sol: enemy infantry IDs 16..23 -> character records 8..15.

									// Sol: 076A..0915: one hit roll governs all personnel attack repeats.
									int16_t signedCoverRows = (int8_t)TerrainOverlapRows[TargetCombatantId];
									// Sol: CBW/SAR halves signed cover, rounding DOWN even for
									// negative odd values. C division truncates toward zero;
									// subtract one for that remainder, without host signed >>.
									AttackTargetNumber += signedCoverRows / 2
										- (signedCoverRows < 0 && signedCoverRows % 2 != 0);
									uint8_t AttackEncoding = WeaponStats[WeaponIndex].attackCountOrClusterColumn;
									int16_t PersonnelDamage = 0x7F; // Sol: mech-category attacks use fixed 127 against personnel.
									if ((AttackEncoding & WeaponInfantryAttackFlag) != FALSE) // Sol: high bit selects personnel damage encoding.
									{
										uint8_t DamageEncoding = WeaponStats[WeaponIndex].damage;
										PersonnelDamage = DamageEncoding & 0x0F; // Sol: low nibble is fixed damage bonus.
										uint16_t DamageDiceRemaining = (DamageEncoding >> 4) & 0x0F; // Sol: high nibble is number of D6 rolls.
										// Sol: 07C2 tests OLD count before decrement; roll once per nonzero count.
										while (DamageDiceRemaining != 0)
										{
											--DamageDiceRemaining;
											PersonnelDamage += RollD6();
										}
									}
									uint16_t AttacksRemaining = AttackEncoding & CombatTargetIdMask; // Sol: legacy mask name; low seven bits are repeat count, not a weapon class.
									if (PersonnelDamage == 0)
										PersonnelDamage = 1; // Sol: minimum damage applied BEFORE armour, not after it.
									if (Roll2D6() < AttackTargetNumber) // Sol: signed comparison; equality hits.
									{
										Combat_CombatMessageVerbosityFilter((uint8_t *)"Missed!"); // Memory ref: [3EDB:3ECD]
										AttackApplied = FALSE;
										AttacksRemaining = 0; // Sol: one miss cancels ALL repeats.
										if (TerrainOverlapRows[TargetCombatantId] != 0
											&& ((WeaponIndex >= WeaponIndex_MediumLaser && WeaponIndex <= WeaponIndex_PPC) || WeaponIndex == WeaponIndex_Inferno))
										{
											if (MapTileUnderCombatant[TargetCombatantId] >= 0x10 && MapTileUnderCombatant[TargetCombatantId] <= 0x3F) // Sol: original fire-eligible tile interval; individual tile meanings not established here.
												{ PendingAttack.fire = TRUE; PendingAttack.fireX = 0xFFFF; PendingAttack.fireY = 0; } // Sol: ASM pushes WORD -1 sentinel (not BYTE FF), followed by zero.
										}
									}
									else
										Combat_CombatMessageVerbosityFilter((uint8_t *)"\r\006\rHit!"); // Sol: native control bytes DS3EDB:3ED6.

									// Commit repeats now; armour and health changes wait until
									// every unit has rolled its attack for this slice.
									PendingAttack.personnel = TRUE;
									PendingAttack.damage = PersonnelDamage;
									PendingAttack.repeats = AttacksRemaining;
									if (AttacksRemaining != 0) {
										AttackApplied = TRUE;
										PlayTargetImpactStream = 1;
									}
								}
								else
								{
									// Sol: 091D: only signed cover is shifted, not the whole target number.
									AttackTargetNumber += ((int16_t)(int8_t)TerrainOverlapRows[TargetCombatantId] >> 3);
									TargetRecordId = TargetCombatantId;

									if (TargetCombatantId >= Enemy_All_CombatantId_Range_First)
										TargetRecordId = TargetCombatantId - Enemy_Infantry_Record_First;

									// Sol: 093F/0951 read signed facing/animation BYTE; FF uses saved facing.
									int16_t TargetFacing = (int8_t)CombatantAnimationSelector[TargetCombatantId];
									if (TargetFacing == -1)
										TargetFacing = (int8_t)CombatSavedFacing[TargetCombatantId];
									// Sol: BP-60 still contains attacker-to-target compass direction here.
									// Sol: subtraction is NOT reduced modulo eight: biased table accepts -7..7.
									int16_t FacingDifference = TargetFacing - AttackDirection;
									int16_t HitLocationCategory = (int8_t)CombatHitCategoryByFacingDifference[FacingDifference + 7]; // Sol: original address 2D11+difference; seven-byte bias.
									uint16_t HitLocationRoll = Roll2D6(); // Sol: independent of the later to-hit roll, even for a kick/miss.
									uint16_t HitLocationOffset = CombatMechHitLocationOffsets[HitLocationCategory * CombatHitLocationRollOutcomes + HitLocationRoll - 2]; // Sol: four categories; eleven outcomes (2D6 = 2..12). Raw mech BYTE offset, not critical-slot ordinal.

									if (WeaponIndex == WeaponIndex_Kick)
									{
										uint16_t AttackingMechRecordId = CombatantId >= Enemy_All_CombatantId_Range_First ? CombatantId - Enemy_Infantry_Record_First : CombatantId; // Sol: enemy IDs 12..15 -> records 4..7 (eight-record bias).
										// Sol: 0996..09C0 reads unsigned tonnage; kick damage is floor(tonnage/5).
										// Sol: 3103 aliases kick record's Damage BYTE; the original mutates shared weapon data.
										WeaponStats[WeaponIndex_Kick].damage = Mechs[AttackingMechRecordId].tonnage / 5; // Sol: one damage point per five tons.
										uint16_t KickChoice = Rand_0x00_to_0xFF() & 8; // Sol: random bit3 selects table address 2E43 or 2E4B, NOT eight outcomes.
										HitLocationOffset = CombatMechHitLocationOffsets[1 + KickChoice]; // Sol: raw offsets 13h or 18h, overriding the ordinary location roll.
									}

									if ((WeaponStats[WeaponIndex].attackCountOrClusterColumn & WeaponInfantryAttackFlag) == 0) //Sol:1AE8:09F1 checks BYTE bit80
									{
								uint8_t BaseWeaponDamage = WeaponStats[WeaponIndex].damage; //Sol: DS:2EE3+index*11h
										uint16_t RemainingDamage = BaseWeaponDamage;

								if (WeaponStats[WeaponIndex].attackCountOrClusterColumn > 1)
									RemainingDamage = BaseWeaponDamage * CombatMissileClusterTable[Roll2D6() * CombatMissileClusterColumns + WeaponStats[WeaponIndex].attackCountOrClusterColumn - CombatMissileClusterIndexBias]; //Sol: 1AE8:0A1A loads BYTE at2E5E+roll*7+column

										if (Roll2D6() >= AttackTargetNumber)
										{
											Combat_CombatMessageVerbosityFilter((uint8_t *)"\r\006\rHit "); // Sol: native DS3EDB:3EE7, trailing space before location.
											Combat_DisplayText_ArmourHitLocation(HitLocationOffset);
											AttackApplied = TRUE;
										}
										else
										{
											Combat_CombatMessageVerbosityFilter((uint8_t *)"Missed!"); // Memory ref: [3EDB:3EDE]
											AttackApplied = FALSE;
											RemainingDamage = 0x00;
											if (TerrainOverlapRows[TargetCombatantId] != 0x00 && (WeaponIndex >= WeaponIndex_MediumLaser && WeaponIndex <= WeaponIndex_PPC || WeaponIndex == WeaponIndex_Inferno))
											{
												if (MapTileUnderCombatant[TargetCombatantId] >= 0x10 && MapTileUnderCombatant[TargetCombatantId] <= 0x3F)
												{
													{ PendingAttack.fire = TRUE; PendingAttack.fireX = 0; PendingAttack.fireY = 0xFFFF; } // Sol: native WORD Y offset-1, not BYTE255.
												}
											}
											if ((TargetCombatantId < Friendly_Infantry_Combatant_Range_First
											  || TargetCombatantId >= Enemy_All_CombatantId_Range_First
											  && TargetCombatantId < Enemy_Infantry_CombatantId_Range_First) && CombatMechPreviousMapRowTile[TargetCombatantId] >= BlockingTileCodeThreshold)
											{
												uint16_t ImpactYBeforeOffset = CombatantPackedY[TargetCombatantId] - CombatMechYParityAdjustment[TargetCombatantId]; // Sol: packed WORD minus unsigned BYTE, not WORD array at4554.
												uint16_t ImpactPackedY = ImpactYBeforeOffset - 0x01;

												if ((ImpactYBeforeOffset - 0x01 & 0x80) != FALSE)
													ImpactPackedY = ImpactYBeforeOffset - 0x01 & ~0x0F80;

												//0x7F - Impact Large
												PendingAttack.mapImpact = TRUE;
												PendingAttack.impactX = CombatantPackedX[TargetCombatantId];
												PendingAttack.impactY = ImpactPackedY; //t4004 - CharacterPosX_Mech
											}
										}
										PendingAttack.damage = RemainingDamage;
										PendingAttack.location = HitLocationOffset;
									}
								}

								PendingAttack.attacker = CombatantId;
								PendingAttack.target = TargetCombatantId;
								PendingAttack.weapon = WeaponIndex;
								PendingAttack.direction = AttackDirection;
								PendingAttack.record = TargetRecordId;
								PendingAttack.impact = PlayTargetImpactStream;
								PendingAttack.applied = AttackApplied;
								SliceAttacks[SliceAttackCount++] = PendingAttack;
								uint16_t WeaponTargetOffset = WeaponSlot + (CombatantId * CombatWeaponTargetSlots);

								CombatWeaponTarget[WeaponTargetOffset] |= 0x80;
							    ShowArmShotOffAnimation = FALSE;

								WeaponSlot = CombatWeaponTargetSlots;
							}

							Move_Map_View_To_Packed_Position(SavedAnchorX, SavedAnchorY); // Sol:0D9F saved anchor BP-2A/-36.

							goto Restore_Attack_Anchor;
						}
Advance_Weapon_Slot:
						WeaponSlot++;
					}
				}
				CombatantId++;
			}


			/* Present against the unchanged post-movement world. Casualty
			 * removal must not invalidate a later shooter's coordinates. */
			for (uint16_t attack = 0; attack < SliceAttackCount; ++attack)
				Combat_Present_Slice_Attack(&SliceAttacks[attack]);

			/* Apply every committed attack, even if an earlier result kills
			 * its shooter or ends the encounter. Only the next slice stops. */
			for (uint16_t attack = 0; attack < SliceAttackCount; ++attack)
				Combat_Apply_Slice_Attack(&SliceAttacks[attack], PrearrangedEncounter);

			// Sol: Reconstructed 1AE8:0F03..0FF5: retire completed FIRST
			// Sol: destination order for friendly actors, not generated step pairs.
			for (uint16_t Actor = 0; Actor < Enemy_All_CombatantId_Range_First; ++Actor)
			{
				uint8_t *Orders = &CombatMovementOrders[Actor * CombatMovementOrderBytes]; //DS:32C6
				if (Orders[0] == 0xFF) continue;
				uint16_t X = CombatantPackedX[Actor];
				uint16_t Y = CombatantPackedY[Actor];
				if (Orders[1] == ((X | Y) >> 8) && Orders[2] == (X & 0x7F) && Orders[3] == (Y & 0x7F))
				{
					// Sol: Original forward overlapping copy: 44 bytes from
					// Sol: row+4 to row. Mark BYTE44 FF; last three bytes remain.
					for (int16_t i = 0; i < 44; ++i) Orders[i] = Orders[i + 4];
					Orders[44] = 0xFF;
				}
			}
		}
	}
	if (MainCharactersAlive != FALSE) //014A = Bool_MainCharactersAlive
	{
		Move_Map_View_To_Packed_Position(InitialCameraX, InitialCameraY); //PosX, PosY args
	}

	//This deals with Mech HeatLevels
	if (MainCharactersAlive != FALSE) //014A = Bool_MainCharactersAlive
	{
		// Sol: Stack FAR pointer SS:BP-78 to the same 24 movement-count bytes.
		Combat_Mech_HeatLevels(SuccessfulMovementSteps);
	}
}

#undef Combat_CombatMessageVerbosityFilter
#undef Combat_DisplayText_ArmourHitLocation
