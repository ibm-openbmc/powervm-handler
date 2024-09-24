#include "config.h"

#include "offload_handler.hpp"

#include "dbus_util.hpp"
#include "utility.hpp"

#include <fmt/format.h>

#include <phosphor-logging/log.hpp>

namespace openpower::dump
{
using ::openpower::dump::utility::DumpType;
using ::openpower::dump::utility::ManagedObjectType;
using ::phosphor::logging::level;
using ::phosphor::logging::log;

OffloadHandler::OffloadHandler(
    sdbusplus::bus::bus& bus, HostOffloaderQueue& dumpOffloader,
    const std::string& entryIntf, const std::string& entryObjPath,
    DumpType dumpType) :
    _bus(bus), _dumpOffloader(dumpOffloader), _entryIntf(entryIntf),
    _dumpType(dumpType), _dumpWatch(bus, dumpOffloader, entryObjPath, dumpType)
{}

void OffloadHandler::offload()
{
    try
    {
        auto objectPaths = getDumpEntryObjPaths(_bus, _entryIntf);
        std::vector<std::string> inProgressDumps;
        for (auto& path : objectPaths)
        {
            bool fcomplete = isDumpProgressCompleted(_bus, path);
            if (!fcomplete)
            {
                log<level::INFO>(
                    fmt::format("Offloader dump is not"
                                " completed, adding to watcher ({})",
                                path)
                        .c_str());
                inProgressDumps.emplace_back(path);
                continue;
            }
            log<level::INFO>(
                fmt::format("Offloader queue dump to offload ({})", path)
                    .c_str());
            // queue the dump for offloading
            _dumpOffloader.enqueue(path, _dumpType);

        } // end for

        // add any inprogress dumps to the watch list
        _dumpWatch.addInProgressDumpsToWatch(std::move(inProgressDumps));
    }
    catch (const std::exception& ex)
    {
        log<level::ERR>(
            fmt::format("Offloader failed to offload dump ex ({})", ex.what())
                .c_str());
        throw;
    }
}

} // namespace openpower::dump
