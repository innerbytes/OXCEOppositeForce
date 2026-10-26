#include "Engine/Options.h"
#include "Engine/Yaml.h"
#include "catch_amalgamated.hpp"

namespace OppositeForce::Tests
{
    using namespace OpenXcom;

    class TestEnvironment : public Catch::EventListenerBase
    {
        bool _previousMute = false;

    public:
        using Catch::EventListenerBase::EventListenerBase;

        void testRunStarting(const Catch::TestRunInfo &) override
        {
            YAML::setGlobalErrorHandler();
            _previousMute = Options::mute;
            // Headless fixtures never play audio. Avoid mixer/Adlib shutdown work for every empty Mod.
            Options::mute = true;
        }

        void testRunEnded(const Catch::TestRunStats &) override
        {
            Options::mute = _previousMute;
        }
    };

    CATCH_REGISTER_LISTENER(TestEnvironment)
}  // namespace OppositeForce::Tests
