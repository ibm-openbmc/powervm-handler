#include "config.h"

#include "dbus_util.hpp"

namespace openpower::dump
{
using ::openpower::dump::utility::DbusVariantType;

bool isDumpProgressCompleted(sdbusplus::bus::bus& bus,
                             const std::string& objectPath)
{
    try
    {
        auto retVal = readDBusProperty<DbusVariantType>(
            bus, dumpService, objectPath, progressIntf, "Status");
        auto status = std::get_if<std::string>(&retVal);
        if (status != nullptr)
        {
            if (*status == progressComplete)
            {
                return true;
            }
        }
    }
    catch (const std::exception& ex)
    {
        log<level::ERR>(
            fmt::format("Util failed to get dump ({}) progress property ({})",
                        objectPath, ex.what())
                .c_str());
        throw;
    }
    return false;
}

bool isDumpProgressCompleted(const DBusPropertiesMap& propMap)
{
    for (auto prop : propMap)
    {
        if (prop.first == "Status")
        {
            auto status = std::get_if<std::string>(&prop.second);
            if (status != nullptr)
            {
                if (*status == progressComplete)
                {
                    return true;
                }
            }
        }
    }
    return false;
}

uint64_t getDumpSize(sdbusplus::bus::bus& bus, const std::string& objectPath)
{
    uint64_t size = 0;
    try
    {
        auto retVal = readDBusProperty<DbusVariantType>(
            bus, dumpService, objectPath, entryIntf, "Size");
        const uint64_t* sizePtr = std::get_if<uint64_t>(&retVal);
        if (sizePtr == nullptr)
        {
            std::string err = fmt::format(
                "Util size value not set for dump object ({})", objectPath);
            log<level::ERR>(err.c_str());
            throw std::runtime_error(err);
        }
        size = *sizePtr;
    }
    catch (const std::exception& ex)
    {
        log<level::INFO>(
            fmt::format(
                "Util failed to get dump size property object ({}) ex({})",
                objectPath, ex.what())
                .c_str());
        throw;
    }
    return size;
}

bool isSystemHMCManaged(sdbusplus::bus::bus& bus)
{
    using BiosBaseTableItem = std::pair<
        std::string,
        std::tuple<std::string, bool, std::string, std::string, std::string,
                   std::variant<int64_t, std::string>,
                   std::variant<int64_t, std::string>,
                   std::vector<std::tuple<
                       std::string, std::variant<int64_t, std::string>>>>>;
    using BiosBaseTable = std::vector<BiosBaseTableItem>;

    try
    {
        std::string hmcManaged{};
        auto retVal = readDBusProperty<std::variant<BiosBaseTable>>(
            bus, "xyz.openbmc_project.BIOSConfigManager",
            "/xyz/openbmc_project/bios_config/manager",
            "xyz.openbmc_project.BIOSConfig.Manager", "BaseBIOSTable");
        const auto baseBiosTable = std::get_if<BiosBaseTable>(&retVal);
        if (baseBiosTable == nullptr)
        {
            log<level::ERR>(
                "Util failed to read BIOSconfig property BaseBIOSTable");
            return false;
        }
        for (const auto& item : *baseBiosTable)
        {
            std::string attributeName = std::get<0>(item);
            auto attrValue = std::get<5>(std::get<1>(item));
            auto val = std::get_if<std::string>(&attrValue);
            if (val != nullptr && attributeName == "pvm_hmc_managed")
            {
                hmcManaged = *val;
                break;
            }
        }
        if (hmcManaged.empty())
        {
            log<level::ERR>(
                "Util failed to read pvm_hmc_managed property value");
            return false;
        }
        if (hmcManaged == "Enabled")
        {
            log<level::INFO>("Util system is HMC managed");
            return true;
        }
    }
    catch (const std::exception& ex)
    {
        log<level::ERR>(
            fmt::format("Util Failed to read pvm_hmc_managed property ({})",
                        ex.what())
                .c_str());
        return false;
    }

    log<level::INFO>("Util system is not HMC managed");
    return false;
}

bool isHostRunning(sdbusplus::bus::bus& bus)
{
    try
    {
        using PropertiesVariant =
            sdbusplus::utility::dedup_variant_t<ProgressStages>;

        constexpr auto hostStateObjPath = "/xyz/openbmc_project/state/host0";
        auto retVal = readDBusProperty<PropertiesVariant>(
            bus, "xyz.openbmc_project.State.Host", hostStateObjPath,
            "xyz.openbmc_project.State.Boot.Progress", "BootProgress");
        const ProgressStages* progPtr = std::get_if<ProgressStages>(&retVal);
        if (progPtr == nullptr)
        {
            return false;
        }
        if (*progPtr == ProgressStages::OSRunning)
        {
            log<level::INFO>("Util host is in  BootProgress::OSRunning");
            return true;
        }
    }
    catch (const std::exception& ex)
    {
        log<level::ERR>(
            fmt::format("Util failed to read BootProgress property ({})",
                        ex.what())
                .c_str());
    }
    log<level::INFO>("Util host is not in  BootProgress::OSRunning state");
    return false;
}

const std::vector<std::string>
    getDumpEntryObjPaths(sdbusplus::bus::bus& bus, const std::string& entryIntf)
{
    std::vector<std::string> liObjectPaths;
    try
    {
        auto mapperCall = bus.new_method_call(
            "xyz.openbmc_project.ObjectMapper",
            "/xyz/openbmc_project/object_mapper",
            "xyz.openbmc_project.ObjectMapper", "GetSubTreePaths");
        const int32_t depth = 0;
        std::vector<std::string> intf{entryIntf};
        mapperCall.append(dumpObjPath);
        mapperCall.append(depth);
        mapperCall.append(intf);
        auto response = bus.call(mapperCall);
        response.read(liObjectPaths);
        log<level::INFO>(
            fmt::format("Util dumps size received is ({}) entry({})",
                        liObjectPaths.size(), entryIntf)
                .c_str());
    }
    catch (const std::exception& ex)
    {
        log<level::ERR>(
            fmt::format(
                "Util failed to get dump entry objects entry({}) ex({})",
                entryIntf, ex.what())
                .c_str());
        throw;
    }
    return liObjectPaths;
}

} // namespace openpower::dump
