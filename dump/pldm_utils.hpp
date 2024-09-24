// SPDX-License-Identifier: Apache-2.0

#pragma once
#include <libpldm/pldm.h>
#include <unistd.h>

namespace openpower::dump::pldm
{
namespace internal
{
/** @struct CustomFd
 *
 *  RAII wrapper for file descriptor.
 */
struct CustomFd
{
  private:
    /** @brief File descriptor */
    int fd = -1;

  public:
    CustomFd() = delete;
    CustomFd(const CustomFd&) = delete;
    CustomFd& operator=(const CustomFd&) = delete;
    CustomFd(CustomFd&&) = delete;
    CustomFd& operator=(CustomFd&&) = delete;

    /** @brief Saves File descriptor and uses it to do file operation
     *
     *  @param[in] fd - File descriptor
     */
    CustomFd(int fd) : fd(fd) {}

    ~CustomFd()
    {
        if (fd >= 0)
        {
            close(fd);
        }
    }

    int operator()() const
    {
        return fd;
    }
};
} // namespace internal
/**
 * @brief Opens the PLDM file descriptor
 *
 * @return file descriptor on success and throw
 *         exception (xyz::openbmc_project::Common::Error::NotAllowed) on
 *         failures.
 */
int openPLDM();

/**
 * @brief Returns the PLDM instance ID to use for PLDM commands
 *
 * @param[in] eid - The PLDM EID
 *
 * @return uint8_t - The instance ID
 **/
uint8_t getPLDMInstanceID(uint8_t eid);

} // namespace openpower::dump::pldm
