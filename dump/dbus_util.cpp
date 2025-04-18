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
        lg2::error(
            "Util failed to dump progress property path:{PATH} exception:{EX}",
            "PATH", objectPath, "EX", ex);
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
            lg2::error(
                "Size property value not set for dump object path:{PATH} ",
                "PATH", objectPath);
            throw std::runtime_error("Size property value not set for dump");
        }
        size = *sizePtr;
    }
    catch (const std::exception& ex)
    {
        lg2::info("Failed to get dump size property value "
                  "exception:{EX} path:{PATH}",
                  "EX", ex, "PATH", objectPath);
        throw;
    }
    return size;
}

bool isSystemHMCManaged(sdbusplus::bus::bus& bus)
{
    auto retVal = readDBusProperty<std::variant<BaseBIOSTableItemList>>(
        bus, "xyz.openbmc_project.BIOSConfigManager",
        "/xyz/openbmc_project/bios_config/manager",
        "xyz.openbmc_project.BIOSConfig.Manager", "BaseBIOSTable");

    if (std::holds_alternative<BaseBIOSTableItemList>(retVal))
    {
        const auto& baseBiosTable = std::get<BaseBIOSTableItemList>(retVal);
        auto it = baseBiosTable.find("pvm_hmc_managed");
        if (it != baseBiosTable.end())
        {
            const auto& attrValue = std::get<5>(it->second);
            if (std::holds_alternative<std::string>(attrValue))
            {
                const std::string& strValue = std::get<std::string>(attrValue);
                lg2::info("isSystemHMCManaged : {VALUE} ", "VALUE", strValue);
                return strValue == "Enabled";
            }
        }
    }
    throw std::runtime_error("Failed to read HMC managed property");
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
            lg2::info("Host is in  BootProgress::OSRunning");
            return true;
        }
    }
    catch (const std::exception& ex)
    {
        lg2::error("Failed to read BootProgress property exception:{EX}", "EX",
                   ex);
    }
    lg2::info("Host is not in BootProgress::OSRunning state");
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
        std::vector<std::string> intf;
        if (entryIntf.find("System") != std::string::npos)
        {
            intf.assign(
                {"com.ibm.Dump.Entry.SBE", "com.ibm.Dump.Entry.Hostboot",
                 "com.ibm.Dump.Entry.Hardware"});
        }
        else
        {
            intf.push_back(entryIntf);
        }
        mapperCall.append(dumpObjPath);
        mapperCall.append(depth);
        mapperCall.append(intf);
        auto response = bus.call(mapperCall);
        response.read(liObjectPaths);
        lg2::info("Dumps size received is:{SIZE} entryIntf:{INTF}", "SIZE",
                  liObjectPaths.size(), "INTF", entryIntf);
    }
    catch (const std::exception& ex)
    {
        lg2::error("Failed to get dump entry intf:{INTF} ex:{EX}", "INTF",
                   entryIntf, "EX", ex);
        throw;
    }
    return liObjectPaths;
}

} // namespace openpower::dump
