// The Wii has no GPS, but the position features (PositionModule: storing other
// nodes' positions, broadcasting the fixed position) need the GPS interface.
// This is what the firmware sees on a board with no GPS fitted: createGps()
// finds nothing, gps stays null, and every caller already checks for that.
// gps/GPS.cpp itself (serial/gpsd drivers) is left out of the build.
#include "configuration.h"
#include "gps/GPS.h"

std::unique_ptr<GPS> gps;

std::unique_ptr<GPS> GPS::createGps()
{
    return nullptr;
}

void GPS::enable() {}

bool GPS::isEnabled()
{
    return false;
}
