#include "pldm_cmd_helper.hpp"

#include <string>
#include <vector>

namespace pldmtool
{

namespace bios
{

namespace
{

using namespace pldmtool::helper;
std::vector<std::unique_ptr<CommandInterface>> commands;

} // namespace

class SetDateTime : public CommandInterface
{
  public:
    ~SetDateTime() = default;
    SetDateTime() = delete;
    SetDateTime(const SetDateTime&) = delete;
    SetDateTime(SetDateTime&&) = default;
    SetDateTime& operator=(const SetDateTime&) = delete;
    SetDateTime& operator=(SetDateTime&&) = default;

    explicit SetDateTime(const char* type, const char* name, CLI::App* app) :
        CommandInterface(type, name, app)
    {
        app->add_option("-d,--data", tmData,
                        "set date time data \n"
                        "eg: 20191010080000")
            ->required()
            ->expected(-1);
    }

    std::pair<int, std::vector<uint8_t>> createRequestMsg() override
    {
        std::vector<uint8_t> requestMsg(sizeof(pldm_msg_hdr) +
                                        sizeof(struct pldm_set_date_time_req));
        auto request = reinterpret_cast<pldm_msg*>(requestMsg.data());

        uint64_t uData = tmData[0];
        uint16_t year = uData / 10000000000;
        uData = uData % 10000000000;
        uint8_t month = uData / 100000000;
        uData = uData % 100000000;
        uint8_t day = uData / 1000000;
        uData = uData % 1000000;
        uint8_t hours = uData / 10000;
        uData = uData % 10000;
        uint8_t minutes = uData / 100;
        uint8_t seconds = uData % 100;

        auto rc = encode_set_date_time_req(
            PLDM_LOCAL_INSTANCE_ID, seconds, minutes, hours, day, month, year,
            request, sizeof(struct pldm_set_date_time_req));

        return {rc, requestMsg};
    }

    void parseResponseMsg(pldm_msg* responsePtr, size_t payloadLength) override
    {
        uint8_t completionCode = 0;
        auto rc = decode_set_date_time_resp(responsePtr, payloadLength,
                                            &completionCode);

        if (rc != PLDM_SUCCESS || completionCode != PLDM_SUCCESS)
        {
            std::cerr << "Response Message Error: "
                      << "rc=" << rc << ",cc=" << (int)completionCode
                      << std::endl;
            return;
        }
    }

  private:
    std::vector<uint64_t> tmData;
};

void registerCommand(CLI::App& app)
{
    auto bios = app.add_subcommand("bios", "bios type command");
    bios->require_subcommand(1);

    auto setDateTime = bios->add_subcommand("SetDateTime", "set bmc date time");
    commands.push_back(
        std::make_unique<SetDateTime>("bios", "setDateTime", setDateTime));
}

} // namespace bios
} // namespace pldmtool
