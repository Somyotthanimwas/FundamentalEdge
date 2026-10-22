#ifndef SWINGSIGNAL_H
#define SWINGSIGNAL_H

#include <filesystem>

class SwingSignal
{
public:
    static bool Run(
        const std::filesystem::path& projectDir,
        const std::filesystem::path& ohlcHistoryCsvPath
    );
};

#endif
