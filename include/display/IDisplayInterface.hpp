#pragma once

namespace display {
    class IDisplayInterface {
        public:
            virtual ~IDisplayInterface() = default;

            virtual void init() = 0;
            virtual void tick() = 0;
    };
}
