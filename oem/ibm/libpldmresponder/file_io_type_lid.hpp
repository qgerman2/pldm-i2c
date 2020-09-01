#pragma once

#include "config.h"

#include "file_io_by_type.hpp"

#include <filesystem>
#include <sstream>
#include <string>

namespace pldm
{
namespace responder
{

using namespace pldm::responder::dma;
namespace fs = std::filesystem;

/** @class LidHandler
 *
 *  @brief Inherits and implements FileHandler. This class is used
 *  to read/write LIDs.
 */
class LidHandler : public FileHandler
{
  public:
    /** @brief LidHandler constructor
     */
    LidHandler(uint32_t fileHandle, bool permSide) : FileHandler(fileHandle)
    {
        sideToRead = permSide ? Pside : Tside;
        std::string dir = permSide ? LID_ALTERNATE_DIR : LID_RUNNING_DIR;
        std::stringstream stream;
        stream << std::hex << fileHandle;
        auto lidName = stream.str() + ".lid";
        std::string patchDir =
            permSide ? LID_ALTERNATE_PATCH_DIR : LID_RUNNING_PATCH_DIR;
        auto patch = fs::path(patchDir) / lidName;
        if (fs::is_regular_file(patch))
        {
            lidPath = patch;
        }
        else
        {
            lidPath = std::move(dir) + '/' + lidName;
        }
    }

    /** @brief Method to construct the LID path based on current boot side
     *  @param[in] oemPlatformHandler - OEM platform handler
     *  @return bool - true if a new path is constructed
     */
    bool constructLIDPath(oem_platform::Handler* oemPlatformHandler)
    {
        if (oemPlatformHandler != nullptr)
        {
            pldm::responder::oem_ibm_platform::Handler* oemIbmPlatformHandler =
                dynamic_cast<pldm::responder::oem_ibm_platform::Handler*>(
                    oemPlatformHandler);
            std::string dir = LID_ALTERNATE_DIR;
            if (oemIbmPlatformHandler->codeUpdate->fetchCurrentBootSide() ==
                sideToRead)
            {
                dir = LID_RUNNING_DIR;
            }
            else if (oemIbmPlatformHandler->codeUpdate
                         ->isCodeUpdateInProgress())
            {
                return false;
            }

            std::stringstream stream;
            stream << std::hex << fileHandle;
            auto lidName = stream.str() + ".lid";
            lidPath = std::move(dir) + '/' + lidName;
        }
        return true;
    }

    virtual int writeFromMemory(uint32_t offset, uint32_t length,
                                uint64_t address,
                                oem_platform::Handler* oemPlatformHandler)
    {
        if (oemPlatformHandler != nullptr)
        {
            pldm::responder::oem_ibm_platform::Handler* oemIbmPlatformHandler =
                dynamic_cast<pldm::responder::oem_ibm_platform::Handler*>(
                    oemPlatformHandler);
            if (oemIbmPlatformHandler->codeUpdate->isCodeUpdateInProgress())
            {
                std::string dir = LID_STAGING_DIR;
                std::stringstream stream;
                stream << std::hex << fileHandle;
                auto lidName = stream.str() + ".lid";
                lidPath = std::move(dir) + '/' + lidName;
            }
        }
        return transferFileData(lidPath, false, offset, length, address);
    }

    virtual int readIntoMemory(uint32_t offset, uint32_t& length,
                               uint64_t address,
                               oem_platform::Handler* oemPlatformHandler)
    {
        if (constructLIDPath(oemPlatformHandler))
        {
            return transferFileData(lidPath, true, offset, length, address);
        }
        return PLDM_ERROR;
    }

    virtual int write(const char* buffer, uint32_t offset, uint32_t& length,
                      oem_platform::Handler* oemPlatformHandler)
    {
        if (oemPlatformHandler != nullptr)
        {
            pldm::responder::oem_ibm_platform::Handler* oemIbmPlatformHandler =
                dynamic_cast<pldm::responder::oem_ibm_platform::Handler*>(
                    oemPlatformHandler);
            if (oemIbmPlatformHandler->codeUpdate->isCodeUpdateInProgress())
            {
                std::string dir = LID_STAGING_DIR;
                std::stringstream stream;
                stream << std::hex << fileHandle;
                auto lidName = stream.str() + ".lid";
                lidPath = std::move(dir) + '/' + lidName;
            }
        }
        std::ios_base::openmode flags =
            std::ios::in | std::ios::out | std::ios::binary;
        if (!fs::exists(lidPath))
        {
            flags = std::ios::out | std::ios::binary;
        }
        else
        {
            size_t fileSize = fs::file_size(lidPath);
            if (offset > fileSize)
            {
                std::cerr << "Offset exceeds file size, OFFSET=" << offset
                          << " FILE_SIZE=" << fileSize << "\n";
                return PLDM_DATA_OUT_OF_RANGE;
            }
        }

        std::ofstream stream(lidPath, flags);
        stream.seekp(offset);
        stream.write(buffer, length);
        stream.close();

        return PLDM_SUCCESS;
    }

    virtual int read(uint32_t offset, uint32_t& length, Response& response,
                     oem_platform::Handler* oemPlatformHandler)
    {
        if (constructLIDPath(oemPlatformHandler))
        {
            return readFile(lidPath, offset, length, response);
        }
        return PLDM_ERROR;
    }

    virtual int fileAck(uint8_t /*fileStatus*/)
    {
        return PLDM_ERROR_UNSUPPORTED_PLDM_CMD;
    }

    virtual int newFileAvailable(uint64_t /*length*/)

    {
        return PLDM_ERROR_UNSUPPORTED_PLDM_CMD;
    }

    /** @brief LidHandler destructor
     */
    ~LidHandler()
    {}

  protected:
    std::string lidPath;
    std::string sideToRead;

  private:
    /** @brief Method to assemble code update images from LID files.
     *  @param[in] filePath - Path to the file to use for the image creation.
     *  @return PLDM status code
     */
    int assembleImage(const std::string& filePath);

    /** @brief Directory where the image files are stored as they are built */
    std::string imageDirPath = fs::path(LID_STAGING_DIR) / "image";

    /** @brief The file name of the hostfw image */
    std::string hostfwImageName = "image-host-fw";

    /** @brief The path to the hostfw image */
    std::string hostfwImagePath = fs::path(imageDirPath) / hostfwImageName;
};

} // namespace responder
} // namespace pldm
