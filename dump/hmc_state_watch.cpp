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
    _bus(bus),
    _dumpQueue(dumpQueue)
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
    using BiosBaseTableItem = std::tuple<
        std::string, bool, std::string, std::string, std::string,
        std::variant<int64_t, std::string>, std::variant<int64_t, std::string>,
        std::vector<
            std::tuple<std::string, std::variant<int64_t, std::string>>>>;
    using BiosBaseTable =
        std::variant<std::map<std::string, BiosBaseTableItem>>;
    using BiosBaseTableType = std::map<std::string, BiosBaseTable>;

    std::string object;
    BiosBaseTableType propMap;
    msg.read(object, propMap);
    for (auto prop : propMap)
    {
        if (prop.first == "BaseBIOSTable")
        {
            auto list = std::get<0>(prop.second);
            for (const auto& item : list)
            {
                std::string attributeName = std::get<0>(item);
                if (attributeName == "pvm_hmc_managed")
                {
                    auto attrValue = std::get<5>(std::get<1>(item));
                    auto val = std::get_if<std::string>(&attrValue);
                    if (val != nullptr && *val == "Enabled")
                    {
                        _dumpQueue.hmcStateChange(true);
                    }
                    else
                    {
                        _dumpQueue.hmcStateChange(false);
                    }
                    break;
                }
            }
        }
    }
}

} // namespace openpower::dump
