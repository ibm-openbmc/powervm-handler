#include "hmc_state_watch.hpp"

#include "dbus_util.hpp"

#include <fmt/format.h>

#include <phosphor-logging/log.hpp>

namespace openpower::dump
{
using ::openpower::dump::utility::DBusPropertiesMap;
using ::phosphor::logging::level;
using ::phosphor::logging::log;

HMCStateWatch::HMCStateWatch(sdbusplus::bus::bus& bus,
                             HostOffloaderQueue& dumpQueue) :
    _bus(bus), _dumpQueue(dumpQueue)
{
    _hmcStatePropWatch = std::make_unique<sdbusplus::bus::match_t>(
        _bus,
        sdbusplus::bus::match::rules::propertiesChanged(
            "/xyz/openbmc_project/bios_config/manager",
            "xyz.openbmc_project.BIOSConfig.Manager"),
        [this](auto& msg) { this->propertyChanged(msg); });
}

void HMCStateWatch::propertyChanged(sdbusplus::message::message& msg)
{
    if (msg.is_method_error())
    {
        log<level::ERR>("Error in reading BIOS attribute signal");
        return;
    }

    using BiosBaseTableMap =
        std::map<std::string, std::variant<BaseBIOSTableItemList>>;
    std::string object;
    BiosBaseTableMap propMap;
    msg.read(object, propMap);
    if (auto it = propMap.find("BaseBIOSTable"); it != propMap.end())
    {
        const auto& baseBiosTableItemList =
            std::get<BaseBIOSTableItemList>(it->second);
        auto attrIt = baseBiosTableItemList.find("pvm_hmc_managed");

        if (attrIt != baseBiosTableItemList.end())
        {
            const auto& attrValue = std::get<5>(attrIt->second);

            if (std::holds_alternative<std::string>(attrValue))
            {
                const std::string& strValue = std::get<std::string>(attrValue);
                if (strValue == "Enabled")
                {
                    // if it is hmc managed exit the service
                    log<level::INFO>("HMC managed system exit the application");
                    std::exit(0);
                }
            }
            else
            {
                log<level::ERR>("Unexpected value type for 'pvm_hmc_managed'");
                std::exit(0);
            }
        }
    }
}
} // namespace openpower::dump
