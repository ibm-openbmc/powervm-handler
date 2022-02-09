#include "config.h"

#include "dump_offload_mgr.hpp"

#include "dump_dbus_util.hpp"

namespace openpower::dump
{
DumpOffloadManager::DumpOffloadManager(sdbusplus::bus::bus& bus) : _bus(bus)
{
    // Changing a system from hmc-managed to non-hmc manged is a disruptive
    // process (Power off the system, do some clean ups and IPL).
    // Changing a system from non-hmc managed to hmc-manged can be done at
    // runtime.
    // Not creating offloader objects if system is HMC managed
    if (isSystemHMCManaged(_bus))
    {
        log<level::INFO>("DumpOffloadManager HMC managed system");
    }
    else
    {
        // add bmc dump offload handler to the list of dump types to offload
        std::unique_ptr<DumpOffloadHandler> bmcDump =
            std::make_unique<DumpOffloadHandler>(bus, bmcEntryIntf,
                                                 DumpType::bmc);
        _dumpOffloadList.push_back(std::move(bmcDump));

        // add host dump offload handler to the list of dump types to offload
        std::unique_ptr<DumpOffloadHandler> hostbootDump =
            std::make_unique<DumpOffloadHandler>(bus, hostbootEntryIntf,
                                                 DumpType::hostboot);
        _dumpOffloadList.push_back(std::move(hostbootDump));

        // add sbe dump offload handler to the list of dump types to offload
        std::unique_ptr<DumpOffloadHandler> sbeDump =
            std::make_unique<DumpOffloadHandler>(bus, sbeEntryIntf,
                                                 DumpType::sbe);
        _dumpOffloadList.push_back(std::move(sbeDump));

        // add hardware dump offload handler to the list of dump types to
        // offload
        std::unique_ptr<DumpOffloadHandler> hardwareDump =
            std::make_unique<DumpOffloadHandler>(bus, hardwareEntryIntf,
                                                 DumpType::hardware);
        _dumpOffloadList.push_back(std::move(hardwareDump));
    }
}

void DumpOffloadManager::offload()
{
    // we can query only on the dump service not on individual entry types,
    // so we get dumps of all types
    ManagedObjectType objects = openpower::dump::getDumpEntries(_bus);
    for (auto& dump : _dumpOffloadList)
    {
        dump->offload(objects);
    }
}
} // namespace openpower::dump
