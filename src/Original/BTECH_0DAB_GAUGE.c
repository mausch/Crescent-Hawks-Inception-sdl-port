#include "game.h"

/* Extra: 1.5 points per pixel for every chassis/location, independent of current
 * or maximum health. Round each nonzero segment up so one point is visible.
 * Keep the original heat renderer below unchanged, including its flash timing.
 * Draw inclusive segment bounds explicitly, avoiding native BUG-006 at one pixel. */
void Draw_Mech_Protection_Gauge(uint16_t x,uint16_t bottomY,
    uint16_t armour,uint16_t structure)
{
    uint16_t heights[2] = {
        (uint16_t)((2u*(uint32_t)structure+2u)/3u),
        (uint16_t)((2u*(uint32_t)armour+2u)/3u)
    };
    const uint16_t colours[2] = {EGA_Red,EGA_Green};
    for (unsigned segment=0; segment<2; ++segment) {
        if (heights[segment]==0) continue;
        uint16_t topY=(uint16_t)(bottomY-heights[segment]+1);
        Draw_Horizontal_EGA_Line(x,topY,(uint16_t)(x+MechStatusGaugeWidth-1),
            bottomY,colours[segment]);
        bottomY=(uint16_t)(topY-1);
    }
}

/* Sol: original 0DAB:174C..1857, retained EGA path. Native signed JLE1
 * tests and IDIV6 are intentional even for WORDs with the high bit set.
 * BUG-006 is retained: after a red segment, a one-unit green segment reuses
 * the red top and passes reversed Y bounds to the rectangle primitive. */
void Draw_Vertical_Mech_Status_Gauge(uint16_t x,uint16_t bottomY,
    uint16_t greenHeight,uint16_t redHeight)
{
    uint16_t topY=bottomY;
    if(redHeight!=0) {
        uint16_t colour=EGA_Red;
        if((int16_t)redHeight>1) topY=(uint16_t)(bottomY-redHeight+1);
        if(bottomY==MechHeatGaugeBottomY) {
            MechHeatGaugeFlashResetDelay=(int16_t)(MechHeatGaugeFlashBaseDelay-
                (int16_t)redHeight/MechHeatGaugeHeatPerDelayStep);
            if(MechHeatGaugeFlashColour==EGA_Black) MechHeatGaugeFlashColour=EGA_Red;
            MechHeatGaugeFlashCountdown=(int16_t)(uint16_t)(
                (uint16_t)MechHeatGaugeFlashCountdown-1);
            if(MechHeatGaugeFlashCountdown<0) {
                MechHeatGaugeFlashCountdown=MechHeatGaugeFlashResetDelay;
                /* Native XOR BYTE121C: high byte remains unchanged. */
                MechHeatGaugeFlashColour^=MechHeatGaugeEgaColourToggle;
            }
            colour=MechHeatGaugeFlashColour;
        }
        Draw_Horizontal_EGA_Line(x,topY,(uint16_t)(x+MechStatusGaugeWidth-1),bottomY,colour);
        bottomY=(uint16_t)(topY-1);
    }
    if(greenHeight!=0) {
        if((int16_t)greenHeight>1) topY=(uint16_t)(bottomY-greenHeight+1);
        Draw_Horizontal_EGA_Line(x,topY,(uint16_t)(x+MechStatusGaugeWidth-1),bottomY,EGA_Green);
    }
}
