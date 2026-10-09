#include "KnobMotion.h"
#include "UiSpec.h"

class KnobMotionTests final : public juce::UnitTest
{
public:
    KnobMotionTests() : juce::UnitTest ("KnobMotion", "BigCity") {}

    void runTest() override
    {
        using Change = KnobMotion::Change;
        constexpr float tolerance = 1.0e-3f;
        constexpr double glideSeconds = UiSpec::Knob::glideMs / 1000.0;
        constexpr double frameSeconds = 1.0 / 120.0;

        beginTest ("A host jump of more than 1.0 in one frame glides: 80 ms, cubic ease-out");
        {
            KnobMotion motion { 50.0f };
            expect (! motion.advance (60.0f, 0.0, Change::judgeBySize), "the first frame of a glide hasn't moved yet");
            expectEquals (motion.getShownPercent(), 50.0f);

            motion.advance (60.0f, glideSeconds * 0.5, Change::judgeBySize);
            expectWithinAbsoluteError (motion.getShownPercent(), 50.0f + 10.0f * 0.875f, tolerance);

            motion.advance (60.0f, glideSeconds, Change::judgeBySize);
            expectEquals (motion.getShownPercent(), 60.0f);
            expect (! motion.advance (60.0f, glideSeconds + frameSeconds, Change::judgeBySize), "idle once landed");
        }

        beginTest ("Host changes of 1.0 or less per frame follow 1:1, so automation curves never trail");
        {
            KnobMotion motion { 50.0f };
            auto percent = 50.0f;

            for (int frame = 1; frame <= 20; ++frame)
            {
                percent += frame % 2 == 0 ? 1.0f : 0.5f;
                expect (motion.advance (percent, frame * frameSeconds, Change::judgeBySize));
                expectEquals (motion.getShownPercent(), percent);
            }
        }

        beginTest ("The knob's own drags and swipes follow 1:1 however far they move in a frame");
        {
            KnobMotion motion { 50.0f };
            motion.advance (62.5f, 0.0, Change::follow);
            expectEquals (motion.getShownPercent(), 62.5f);
            motion.advance (20.0f, frameSeconds, Change::follow);
            expectEquals (motion.getShownPercent(), 20.0f);
        }

        beginTest ("The knob's own notches and resets glide, even a 1.0 notch");
        {
            KnobMotion motion { 50.0f };
            motion.advance (51.0f, 0.0, Change::glide);
            motion.advance (51.0f, glideSeconds * 0.5, Change::glide);
            expectWithinAbsoluteError (motion.getShownPercent(), 50.875f, tolerance);
        }

        beginTest ("A new jump mid-glide restarts from where the knob is, without a jump");
        {
            KnobMotion motion { 50.0f };
            motion.advance (60.0f, 0.0, Change::judgeBySize);
            motion.advance (60.0f, glideSeconds * 0.5, Change::judgeBySize);
            const auto midGlide = motion.getShownPercent();

            motion.advance (30.0f, glideSeconds * 0.5, Change::judgeBySize);
            expectEquals (motion.getShownPercent(), midGlide);

            motion.advance (30.0f, glideSeconds * 1.5, Change::judgeBySize);
            expectEquals (motion.getShownPercent(), 30.0f);
        }

        beginTest ("A small change mid-glide moves the landing point without a jump");
        {
            KnobMotion motion { 50.0f };
            motion.advance (60.0f, 0.0, Change::judgeBySize);
            motion.advance (60.0f, glideSeconds * 0.5, Change::judgeBySize);
            const auto midGlide = motion.getShownPercent();

            // Same instant, so only the retarget can move it: by 0.5 × 87.5%, well under a frame's follow step.
            motion.advance (60.5f, glideSeconds * 0.5, Change::follow);
            expectWithinAbsoluteError (motion.getShownPercent(), midGlide, 0.5f);

            motion.advance (60.5f, glideSeconds, Change::judgeBySize);
            expectEquals (motion.getShownPercent(), 60.5f);
        }
    }
};

static KnobMotionTests knobMotionTests;
