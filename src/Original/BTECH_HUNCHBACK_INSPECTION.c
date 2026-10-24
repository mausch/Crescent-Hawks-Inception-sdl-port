#include "game.h"
#include <string.h>

#include "BTECH_HUNCHBACK_INSPECTION_ART.inc"

static int Is_Hunchback_Record(const Mech *mech)
{
    return memcmp(mech->name,"HUNCHBACK      ",MechNameLength)==0;
}

/* Patch the decompressed packed-nibble image, never the cached CMP or on-disk
 * assets. The original labels, heat gauge, palette, and live armour/structure
 * gauges remain in charge. No new chassis ID or save-format field is needed. */
void Patch_Hunchback_Inspection(uint8_t *image,const Mech *mech)
{
    if(!Is_Hunchback_Record(mech)) return;
    const uint8_t *source=hunchbackInspectionPixels;
    for(unsigned region=0;region<sizeof hunchbackInspectionRegions/sizeof hunchbackInspectionRegions[0];++region) {
        const uint16_t *bounds=hunchbackInspectionRegions[region];
        size_t width=(bounds[2]-bounds[0])/2u;
        for(uint16_t y=bounds[1];y<bounds[3];++y) {
            memcpy(image+y*PackedGraphicsRowBytes+bounds[0]/2u,source,width);
            source+=width;
        }
    }
}

uint16_t Resolve_Mech_Status_Gauge_X(const Mech *mech,uint16_t location)
{
    if(location<MechArmourLocationCount && Is_Hunchback_Record(mech))
        return hunchbackInspectionGaugeX[location];
    return MechStatusGaugeX[location];
}

uint16_t Resolve_Mech_Status_Gauge_Bottom_Y(const Mech *mech,uint16_t location)
{
    if(location<MechArmourLocationCount && Is_Hunchback_Record(mech))
        return hunchbackInspectionGaugeBottomY[location];
    return MechStatusGaugeBottomY[location];
}
