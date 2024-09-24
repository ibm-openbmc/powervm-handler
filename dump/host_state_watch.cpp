#include "host_state_watch.hpp"

#include "dbus_util.hpp"
#include "utility.hpp"

#include <fmt/format.h>

#include <phosphor-logging/log.hpp>

namespace openpower::dump
{
using ::openpower::dump::utility::DBusPropertiesMap;
using ::phosphor::logging::level;
using ::phosphor::logging::log;

HostStateWatch::HostStateWatch(sdbusplus::bus::bus& bus,
                               HostOffloaderQueue& dumpQueue) :
    _bus(bus), _dumpQueue(dumpQueue)
{
    _hostStatePropWatch = std::make_unique<sdbusplus::bus::match_t>(
        _bus,
        sdbusplus::bus::match::rules::propertiesChanged(
            "/xyz/openbmc_project/state/host0",
            "xyz.openbmc_project.State.Boot.Progress"),
        [this](auto& msg) { this->propertyChanged(msg); });
}

void HostStateWatch::propertyChanged(sdbusplus::message::message& msg)
{
    std::string intf;
    DBusPropertiesMap propMap;
    msg.read(intf, propMap);
    for (auto prop : propMap)
    {
        if (prop.first == "BootProgress")
        {
            auto progress = std::get_if<ProgressStages>(&prop.second);
            if (progress != nullptr)
            {
                if (*progress == ProgressStages::OSRunning)
                {
                    log<level::INFO>("Host state is ProgressStages::OSRunning");
                    _dumpQueue.hostStateChange(true);
                }
                else
                {
                    _dumpQueue.hostStateChange(false);
                }
            }
            else
            {
                _dumpQueue.hostStateChange(false);
            }
        }
    }
}

} // namespace openpower::dump
