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
        return;
    }

    // Perform offload only if host is in running state, else add watch on
    // BootProgress property and offload when it moves to running state.
    //
    // At present we do not have a system-d target which indicates host is
    // in running target with which we could have added wait-for condition
    // for this service to start.
    if (isHostRunning(bus) == false)
    {
        log<level::ERR>("Host is not running do not offload dump, add watch"
                        " on host state");
        _hostStatePropWatch = std::make_unique<sdbusplus::bus::match_t>(
            _bus,
            sdbusplus::bus::match::rules::propertiesChanged(
                "/xyz/openbmc_project/state/host0",
                "xyz.openbmc_project.State.Host"),
            [this](auto& msg) { this->propertiesChanged(msg); });
    }
    else
    {
        addOffloadHandlers();
    }
}

void DumpOffloadManager::propertiesChanged(sdbusplus::message::message& msg)
{
    std::string intf;
    DBusPropertiesMap propMap;
    msg.read(intf, propMap);
    log<level::DEBUG>(
        fmt::format("propertiesChanged interface ({}) ", intf).c_str());
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
                    // we are checking for multiple values as part of
                    // BootProgress dump watch so we might get multiple
                    // notifications so adding check to create handlers
                    // only once.
                    if (_fOffloaded == false)
                    {
                        addOffloadHandlers();
                        offload();
                        _fOffloaded = true;
                    }
                }
            }
        }
    }
}

void DumpOffloadManager::addOffloadHandlers()
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

void DumpOffloadManager::offload()
{
    // system can change from hon-hmc to hmc managed system, do not offload
    // if system changed to hmc managed system
    if (!isSystemHMCManaged(_bus))
    {
        // we can query only on the dump service not on individual entry types,
        // so we get dumps of all types
        ManagedObjectType objects = openpower::dump::getDumpEntries(_bus);
        for (auto& dump : _dumpOffloadList)
        {
            dump->offload(objects);
        }
    }
}
} // namespace openpower::dump
