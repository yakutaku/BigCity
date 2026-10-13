#include "ReduceMotion.h"

#import <AppKit/AppKit.h>

namespace ReduceMotion
{
    bool isOn()
    {
        // AppKit keeps this current as the setting changes, so reading it is cheap enough to poll.
        return [[NSWorkspace sharedWorkspace] accessibilityDisplayShouldReduceMotion];
    }
}
