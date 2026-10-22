#include "SwingSignal.h"

#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <string>
#include <windows.h>

int main()
{
    std::cout << "============================================\n";
    std::cout << " FundamentalUpdater_rev12 - Supertrend Pre-Buy Scanner\n";
    std::cout << "============================================\n";

    char exePath[MAX_PATH]{};

    GetModuleFileNameA(nullptr, exePath, MAX_PATH);

    std::filesystem::path exeDir =
        std::filesystem::path(exePath).parent_path();

    std::filesystem::path projectDir =
        exeDir.parent_path();

    std::filesystem::path dataDir =
        projectDir / "Data";

    const auto ohlcHistoryCsvPath =
        dataDir / "ohlc_history.csv";

    const auto exporterPath =
        projectDir / "tools" / "export_amibroker_ohlc.js";

    const auto cscriptPath =
        std::filesystem::path(
            std::getenv("WINDIR")
                ? std::getenv("WINDIR")
                : "C:\\Windows"
        )
        / "SysWOW64"
        / "CScript.exe";

    std::cout
        << "OHLC       : \""
        << ohlcHistoryCsvPath.string()
        << "\"\n";

    std::cout
        << "AmiBroker  : \""
        << exporterPath.string()
        << "\"\n";

    std::cout
        << "CScript    : \""
        << cscriptPath.string()
        << "\"\n";

    if (!std::filesystem::exists(exporterPath))
    {
        std::cerr
            << "ERROR: AmiBroker exporter not found:\n"
            << exporterPath
            << '\n';

        return 1;
    }

    if (!std::filesystem::exists(cscriptPath))
    {
        std::cerr
            << "ERROR: 32-bit CScript.exe not found:\n"
            << cscriptPath
            << '\n';

        return 1;
    }

    std::error_code ec;

    std::filesystem::create_directories(dataDir, ec);

    if (ec)
    {
        std::cerr
            << "ERROR: Cannot create Data directory: "
            << ec.message()
            << '\\n';

        return 1;
    }

    ec.clear();

    std::filesystem::remove(
        ohlcHistoryCsvPath,
        ec
    );

    if (ec)
    {
        std::cerr
            << "ERROR: Cannot remove old ohlc_history.csv: "
            << ec.message()
            << '\n';

        return 1;
    }

    /*
        Export OHLC directly from AmiBroker.

        No swing_candidates.csv is used.
        The exporter is responsible for selecting
        only SET stocks and excluding DW / W / other
        non-stock symbols.
    */

    std::string command =
        "cmd.exe /d /c \"\"" +
        cscriptPath.string() +
        "\" //nologo \"" +
        exporterPath.string() +
        "\" \"" +
        ohlcHistoryCsvPath.string() +
        "\"\"";

    std::cout
        << "\nUpdating SET stock OHLC from AmiBroker...\n";

    std::cout
        << command
        << "\n\n";

    const int exportResult =
        std::system(command.c_str());

    if (exportResult != 0)
    {
        std::cerr
            << "\nERROR: AmiBroker OHLC export failed.\n"
            << "Exit code: "
            << exportResult
            << '\n';

        return 1;
    }

    if (!std::filesystem::exists(ohlcHistoryCsvPath))
    {
        std::cerr
            << "ERROR: AmiBroker export completed but "
               "ohlc_history.csv was not created.\n";

        return 1;
    }

    const auto fileSize =
        std::filesystem::file_size(
            ohlcHistoryCsvPath,
            ec
        );

    if (ec || fileSize == 0)
    {
        std::cerr
            << "ERROR: ohlc_history.csv is empty.\n";

        return 1;
    }

    std::cout
        << "\nAmiBroker SET stock OHLC update completed.\n";

    std::cout
        << "OHLC file size: "
        << fileSize
        << " bytes\n";

    std::cout
        << "\nRunning Supertrend Pre-Buy Scanner...\n";

    /*
        SwingSignal now scans the exported OHLC data
        directly. No candidate list is required.
    */

    if (!SwingSignal::Run(
            projectDir,
            ohlcHistoryCsvPath))
    {
        std::cerr
            << "\nERROR: Near-Buy Scanner failed.\n";

        return 1;
    }

    std::cout
        << "\n============================================\n";
    std::cout
        << " Rev12 Supertrend Pre-Buy Scanner completed successfully\n";
    std::cout
        << "============================================\n";

    return 0;
}