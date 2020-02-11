#pragma once

#include "utils.hpp"

#include <stdint.h>

#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <map>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>
#include <xyz/openbmc_project/Common/error.hpp>

#include "libpldm/pdr.h"

using InternalFailure =
    sdbusplus::xyz::openbmc_project::Common::Error::InternalFailure;
using namespace pldm::utils;

namespace fs = std::filesystem;

namespace pldm
{

namespace responder
{

namespace pdr_utils
{

/** @struct PdrEntry
 *  PDR entry structure that acts as a PDR record structure to a PDR repository
 *  for handing PDR APIs.
 */
struct PdrEntry
{
    uint8_t* data;
    uint32_t size;
    union
    {
        uint32_t recordHandle;
        uint32_t nextRecordHandle;
    } handle;
};
using Type = uint8_t;
using Json = nlohmann::json;
using RecordHandle = uint32_t;
using StateId = uint16_t;
using PossibleValues = std::vector<uint8_t>;

/** @brief Map of DBus property stateId to attribute value
 */
using DbusIdToValMap = std::map<StateId, PropertyValue>;

/** @brief Parse PDR JSON file and output Json object
 *
 *  @param[in] path - path of PDR JSON file
 *
 *  @return Json - Json object
 */
inline Json readJson(const std::string& path)
{
    std::ifstream jsonFile(path);
    if (!jsonFile.is_open())
    {
        std::cerr << "Error opening PDR JSON file, PATH=" << path << "\n";
        return {};
    }

    return Json::parse(jsonFile);
}

/** @brief Populate the mapping between D-Bus property stateId and attribute
 *          value for the effecter PDR enumeration attribute.
 *
 *  @param[in] type - type of the D-Bus property
 *  @param[in] dBusValues - json array of D-Bus property values
 *  @param[in] pv - Possible values for the effecter PDR enumeration attribute
 *
 */
DbusIdToValMap populateMapping(const std::string& type, const Json& dBusValues,
                               const PossibleValues& pv);

/**
 *  @class RepoInterface
 *
 *  @brief Abstract class describing the interface API to the PDR repository
 *
 *  Wrapper class to handle the new PDR APIs
 *
 *  This class wraps operations used to handle PDR APIs.
 */
class RepoInterface
{
  public:
    virtual ~RepoInterface() = default;

    /** @brief Get an opaque pldm_pdr structure
     *
     *  @return pldm_pdr - pldm_pdr structure
     */
    virtual const pldm_pdr* getPdr() = 0;

    /** @brief Add a PDR record to a PDR repository
     *
     *  @param[in] pdrEntry - PDR records entry(data, size, recordHandle)
     *
     *  @return uint32_t - record handle assigned to PDR record
     */
    virtual RecordHandle addRecord(const PdrEntry& pdrEntry) = 0;

    /** @brief get the first PDR record from a PDR repository
     *
     *  @param[in] pdrEntry - PDR records entry(data, size, nextRecordHandle)
     *
     *  @return opaque pointer acting as PDR record handle, will be NULL if
     *          record was not found
     */
    virtual const pldm_pdr_record* getFirstRecord(PdrEntry& pdrEntry) = 0;

    /** @brief get the next PDR record from a PDR repository
     *
     *  @param[in] curr_record - opaque pointer acting as a PDR record handle
     *  @param[in] pdrEntry - PDR records entry(data, size, nextRecordHandle)
     *
     *  @return opaque pointer acting as PDR record handle, will be NULL if
     *          record was not found
     */
    virtual const pldm_pdr_record*
        getNextRecord(const pldm_pdr_record* curr_record,
                      PdrEntry& pdrEntry) = 0;

    /** @brief Get record handle of a PDR record
     *
     *  @param[in] record - opaque pointer acting as a PDR record handle
     *
     *  @return uint32_t - record handle assigned to PDR record; 0 if record is
     *                     not found
     */
    virtual uint32_t getRecordHandle(const pldm_pdr_record* record) = 0;

    /** @brief Get number of records in a PDR repository
     *
     *  @return uint32_t - number of records
     */
    virtual uint32_t getRecordCount() = 0;

    /** @brief Determine if records are empty in a PDR repository
     *
     *  @return bool - true means empty and false means not empty
     */
    virtual bool empty() = 0;

  protected:
    pldm_pdr* repo;
};

/**
 *  @class Repo
 *
 *  Wrapper class to handle the PDR APIs
 *
 *  This class wraps operations used to handle PDR APIs.
 */
class Repo : public RepoInterface
{
  public:
    Repo()
    {
        repo = pldm_pdr_init();
    }

    ~Repo()
    {
        pldm_pdr_destroy(repo);
    }

    const pldm_pdr* getPdr();

    RecordHandle addRecord(const PdrEntry& pdrEntry);

    const pldm_pdr_record* getFirstRecord(PdrEntry& pdrEntry);

    const pldm_pdr_record* getNextRecord(const pldm_pdr_record* curr_record,
                                         PdrEntry& pdrEntry);

    uint32_t getRecordHandle(const pldm_pdr_record* record);

    uint32_t getRecordCount();

    bool empty();
};

} // namespace pdr_utils
} // namespace responder
} // namespace pldm
