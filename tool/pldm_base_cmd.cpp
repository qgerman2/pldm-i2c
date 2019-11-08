#include "pldm_base_cmd.hpp"

#include "pldm_cmd_helper.hpp"

#include <map>
#include <memory>
#include <string>
#include <vector>

constexpr uint8_t PLDM_ENTITY_ID = 8;
constexpr uint8_t MCTP_MSG_TYPE_PLDM = 1;
constexpr uint8_t PLDM_LOCAL_INSTANCE_ID = 0;

std::map<const char*, pldm_supported_types> pldmTypes{
    {"base", PLDM_BASE}, {"platform", PLDM_PLATFORM}, {"bios", PLDM_BIOS},
    {"fru", PLDM_FRU},   {"oem", PLDM_OEM},
};

class GetPLDMTypes : public CommandInterface
{
  public:
    ~GetPLDMTypes() = default;
    GetPLDMTypes() = delete;
    GetPLDMTypes(const GetPLDMTypes&) = default;
    GetPLDMTypes(GetPLDMTypes&&) = default;
    GetPLDMTypes& operator=(const GetPLDMTypes&) = default;
    GetPLDMTypes& operator=(GetPLDMTypes&&) = default;

    explicit GetPLDMTypes(const char* type, const char* name, CLI::App* app) :
        CommandInterface(type, name, app)
    {
    }

    std::pair<int, std::vector<uint8_t>> createRequestMsg() override
    {
        std::vector<uint8_t> requestMsg(sizeof(pldm_msg_hdr));
        auto request = reinterpret_cast<pldm_msg*>(requestMsg.data());
        auto rc = encode_get_types_req(PLDM_LOCAL_INSTANCE_ID, request);
        return {rc, requestMsg};
    }

    void parseResponseMsg(pldm_msg* responsePtr, size_t payloadLength) override
    {
        uint8_t cc = 0;
        std::vector<bitfield8_t> types(8);
        auto rc = decode_get_types_resp(responsePtr, payloadLength, &cc,
                                        types.data());
        if (rc != PLDM_SUCCESS || cc != PLDM_SUCCESS)
        {
            std::cout << "Response Message Error: "
                      << "rc=" << rc << ",cc=" << (int)cc << std::endl;
            return;
        }

        printPldmTypes(types);
    }

  private:
    void printPldmTypes(std::vector<bitfield8_t>& types)
    {
        std::cout << "Supported types:";
        for (int i = 0; i < PLDM_MAX_TYPES; i++)
        {
            bitfield8_t b = types[i / 8];
            if (b.byte & (1 << i % 8))
            {
                std::cout << " " << i;
                auto it =
                    std::find_if(pldmTypes.begin(), pldmTypes.end(),
                                 [i](const auto& t) { return t.second == i; });
                if (it != pldmTypes.end())
                {

                    std::cout << "(" << it->first << ")";
                }
            }
        }

        std::cout << std::endl;
    }
};

class GetPLDMVersion : public CommandInterface
{
  public:
    ~GetPLDMVersion() = default;
    GetPLDMVersion() = delete;
    GetPLDMVersion(const GetPLDMVersion&) = default;
    GetPLDMVersion(GetPLDMVersion&&) = default;
    GetPLDMVersion& operator=(const GetPLDMVersion&) = default;
    GetPLDMVersion& operator=(GetPLDMVersion&&) = default;

    explicit GetPLDMVersion(const char* type, const char* name, CLI::App* app) :
        CommandInterface(type, name, app)
    {
        app->add_option("-t,--type", pldmType, "pldm supported type")
            ->required()
            ->transform(CLI::CheckedTransformer(pldmTypes, CLI::ignore_case));
    }
    std::pair<int, std::vector<uint8_t>> createRequestMsg() override
    {
        std::vector<uint8_t> requestMsg(sizeof(pldm_msg_hdr) +
                                        PLDM_GET_VERSION_REQ_BYTES);
        auto request = reinterpret_cast<pldm_msg*>(requestMsg.data());

        auto rc = encode_get_version_req(PLDM_LOCAL_INSTANCE_ID, 0,
                                         PLDM_GET_FIRSTPART, pldmType, request);
        return {rc, requestMsg};
    }

    void parseResponseMsg(pldm_msg* responsePtr, size_t payloadLength) override
    {
        std::ignore = responsePtr;
        std::ignore = payloadLength;

        uint8_t cc = 0, transferFlag = 0;
        uint32_t transferHandle = 0;

        // ver32_t version;
        // decode_get_version_resp(responsePtr, payloadLength, &cc,
        //                         &transferHandle, &transferFlag, &version);

        std::cout << "parseResponseMsg for GetVersion command" << std::endl;
    }

  private:
    pldm_supported_types pldmType;
};

std::vector<std::shared_ptr<CommandInterface>> commands;

void registerBaseCommand(CLI::App& app)
{
    auto base = app.add_subcommand("base", "base type command");
    auto getPLDMTypes =
        base->add_subcommand("GetPLDMTypes", "Get PLDM Supported Types");
    commands.push_back(
        std::make_shared<GetPLDMTypes>("base", "GetPLDMTypes", getPLDMTypes));

    auto getPLDMVersion =
        base->add_subcommand("GetPLDMVersion", "Get PLDM Version");
    commands.push_back(std::make_shared<GetPLDMVersion>(
        "base", "GetPLDMVersion", getPLDMVersion));
}