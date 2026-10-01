#pragma once

// What one City Size position means for each sound element.
struct CitySettings
{
    float airGainDb;
    float detuneCents;  // magnitude; the left voice goes down by this much and the right voice up
    float wetLevelDb;   // minus infinity means off
};

namespace CityMacro
{
    // Maps citySize (0–100, clamped) to the PRD §6 curves.
    CitySettings map (float citySizePercent);
}
