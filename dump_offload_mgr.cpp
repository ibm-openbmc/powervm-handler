#include "config.h"

#include "dump_offload_mgr.hpp"

#include "dump_dbus_util.hpp"

namespace openpower::dump
{
DumpOffloadManager::DumpOffloadManager(sdbusplus::bus::bus& bus) : _bus(bus)
{
    // add bmc dump offload handler to the list of dump types to offload
    std::unique_ptr<DumpOffloadHandler> bmcDump =
        std::make_unique<DumpOffloadHandler>(_bus, bmcEntryIntf, DumpType::bmc);
    _dumpOffloadList.push_back(std::move(bmcDump));

    // add host dump offload handler to the list of dump types to offload
    std::unique_ptr<DumpOffloadHandler> hostbootDump =
        std::make_unique<DumpOffloadHandler>(_bus, hostbootEntryIntf,
                                             DumpType::hostboot);
    _dumpOffloadList.push_back(std::move(hostbootDump));

    // add sbe dump offload handler to the list of dump types to offload
    std::unique_ptr<DumpOffloadHandler> sbeDump =
        std::make_unique<DumpOffloadHandler>(_bus, sbeEntryIntf, DumpType::sbe);
    _dumpOffloadList.push_back(std::move(sbeDump));

    // add hardware dump offload handler to the list of dump types to
    // offload
    std::unique_ptr<DumpOffloadHandler> hardwareDump =
        std::make_unique<DumpOffloadHandler>(_bus, hardwareEntryIntf,
                                             DumpType::hardware);
    _dumpOffloadList.push_back(std::move(hardwareDump));
}

void DumpOffloadManager::propertiesChanged(sdbusplus::message::message& msg)
{
    std::string intf;
    DBusPropertiesMap propMap;
    msg.read(intf, propMap);
    log<level::INFO>(
        fmt::format("Host state propertiesChanged interface ({}) ", intf)
            .c_str());
    for (auto prop : propMap)
    {
        if (prop.first == "BootProgress")
        {
            auto progress = std::get_if<std::string>(&prop.second);
            if (progress != nullptr)
            {
                ProgressStages bootProgress =
                    sdbusplus::xyz::openbmc_project::State::Boot::server::
                        Progress::convertProgressStagesFromString(*progress);
                if ((bootProgress == ProgressStages::SystemInitComplete) ||
                    (bootProgress == ProgressStages::OSStart) ||
                    (bootProgress == ProgressStages::OSRunning))
                {
                    log<level::INFO>(
                        fmt::format("Offloading dumps host is now in  ({})"
                                    " state ",
                                    *progress)
                            .c_str());
                    offloadHelper();
                }
            }
        }
    }
}

void DumpOffloadManager::offloadHelper()
{
    // ensure dumps are offloaded only once, also hmc might change from
    // non hmc to hmc by the time host goes to runtime so add HMC state check
    if (!_fOffloaded && !isSystemHMCManaged(_bus))
    {
        // we can query only on the dump service not on individual entry types,
        // so we get dumps of all types
        ManagedObjectType objects = openpower::dump::getDumpEntries(_bus);
        for (auto& dump : _dumpOffloadList)
        {
            dump->offload(objects);
        }
        _fOffloaded = true;
    }
}

void DumpOffloadManager::offload()
{
    if (isHostRunning(_bus))
    {
        offloadHelper();
    }
    else
    {
        // Perform offload only if host is in running state, else add watch on
        // BootProgress property and offload when it moves to running state.
        //
        // At present we do not have a system-d target which indicates host is
        // in running target with which we could have added wait-for condition
        // for this service to start.
        log<level::ERR>("Host is not in running state adding watch"
                        " on boot progress property");
        _hostStatePropWatch = std::make_unique<sdbusplus::bus::match_t>(
            _bus,
            sdbusplus::bus::match::rules::propertiesChanged(
                "/xyz/openbmc_project/state/host0",
                "xyz.openbmc_project.State.Boot.Progress"),
            [this](auto& msg) { this->propertiesChanged(msg); });
    }
}
} // namespace openpower::dump
