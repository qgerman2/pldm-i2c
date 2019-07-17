#include "libpldmresponder/base.hpp"
#include "libpldmresponder/bios.hpp"
#include "libpldmresponder/utils.hpp"
#include "registration.hpp"

#include <err.h>
#include <poll.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>

#include <cstdio>
#include <cstring>
#include <iomanip>
#include <iterator>
#include <phosphor-logging/log.hpp>
#include <sstream>
#include <vector>

#include "libpldm/base.h"
#include "libpldm/bios.h"

#ifdef OEM_IBM
#include "libpldmresponder/file_io.hpp"
#endif

constexpr uint8_t MCTP_MSG_TYPE_PLDM = 1;

using namespace phosphor::logging;
using namespace pldm;

static Response processRxMsg(const std::vector<uint8_t>& requestMsg)
{

    Response response;
    uint8_t eid = requestMsg[0];
    uint8_t type = requestMsg[1];
    pldm_header_info hdrFields{};
    auto hdr = reinterpret_cast<const pldm_msg_hdr*>(
        requestMsg.data() + sizeof(eid) + sizeof(type));
    if (PLDM_SUCCESS != unpack_pldm_header(hdr, &hdrFields))
    {
        log<level::ERR>("Empty PLDM request header");
    }
    else if (PLDM_RESPONSE != hdrFields.msg_type)
    {
        auto request = reinterpret_cast<const pldm_msg*>(hdr);
        size_t requestLen = requestMsg.size() - sizeof(struct pldm_msg_hdr) -
                            sizeof(eid) - sizeof(type);
        response = pldm::responder::invokeHandler(
            hdrFields.pldm_type, hdrFields.command, request, requestLen);
        if (response.empty())
        {
            uint8_t completion_code = PLDM_ERROR_UNSUPPORTED_PLDM_CMD;
            response.resize(sizeof(pldm_msg_hdr));
            auto responseHdr = reinterpret_cast<pldm_msg_hdr*>(response.data());
            pldm_header_info header{};
            header.msg_type = PLDM_RESPONSE;
            header.instance = hdrFields.instance;
            header.pldm_type = hdrFields.pldm_type;
            header.command = hdrFields.command;
            auto result = pack_pldm_header(&header, responseHdr);
            if (PLDM_SUCCESS != result)
            {
                log<level::ERR>("Failed adding response header");
            }
            response.insert(response.end(), completion_code);
        }
    }
    return response;
}

void printBuffer(const std::vector<uint8_t>& buffer)
{
    std::ostringstream tempStream;
    tempStream << "Buffer Data: ";
    if (!buffer.empty())
    {
        for (int byte : buffer)
        {
            tempStream << std::setfill('0') << std::setw(2) << std::hex << byte
                       << " ";
        }
    }
    log<level::INFO>(tempStream.str().c_str());
}

int main(int argc, char** argv)
{

    pldm::responder::base::registerHandlers();
    pldm::responder::bios::registerHandlers();

    /* Outgoing message. */
    struct iovec iov[3];

    /* This structure contains the parameter information for the response
     * message. */
    struct msghdr sg_response;

#ifdef OEM_IBM
    pldm::responder::oem_ibm::registerHandlers();
#endif

    /* Create local socket. */
    int returnCode = 0;
    int sockfd = socket(AF_UNIX, SOCK_SEQPACKET, 0);
    if (-1 == sockfd)
    {
        returnCode = -errno;
        log<level::ERR>("Failed to create the socket",
                        entry("RC=%d", returnCode));
        exit(EXIT_FAILURE);
    }

    responder::utils::CustomFD socketFd(sockfd);

    struct sockaddr_un addr
    {
    };
    addr.sun_family = AF_UNIX;
    const char path[] = "\0mctp-mux";
    memcpy(addr.sun_path, path, sizeof(path) - 1);
    int result = connect(socketFd(), reinterpret_cast<struct sockaddr*>(&addr),
                         sizeof(path) + sizeof(addr.sun_family) - 1);
    if (-1 == result)
    {
        returnCode = -errno;
        log<level::ERR>("Failed to connect to the socket",
                        entry("RC=%d", returnCode));
        exit(EXIT_FAILURE);
    }

    result = write(socketFd(), &MCTP_MSG_TYPE_PLDM, sizeof(MCTP_MSG_TYPE_PLDM));
    if (-1 == result)
    {
        returnCode = -errno;
        log<level::ERR>("Failed to send message type as pldm to mctp",
                        entry("RC=%d", returnCode));
        exit(EXIT_FAILURE);
    }

    do
    {
        ssize_t peekedLength =
            recv(socketFd(), nullptr, 0, MSG_PEEK | MSG_TRUNC);
        if (0 == peekedLength)
        {
            log<level::ERR>("Socket has been closed");
            exit(EXIT_FAILURE);
        }
        else if (peekedLength <= -1)
        {
            returnCode = -errno;
            log<level::ERR>("recv system call failed",
                            entry("RC=%d", returnCode));
            exit(EXIT_FAILURE);
        }
        else
        {
            std::vector<uint8_t> requestMsg(peekedLength);
            auto recvDataLength = recv(
                sockfd, static_cast<void*>(requestMsg.data()), peekedLength, 0);
            if (recvDataLength == peekedLength)
            {
#ifdef VERBOSE
                log<level::INFO>("Received Msg ",
                                 entry("LENGTH=%zu", recvDataLength),
                                 entry("EID=0x%02x", requestMsg[0]),
                                 entry("TYPE=0x%02x", requestMsg[1]));
                printBuffer(requestMsg);
#endif
                if (MCTP_MSG_TYPE_PLDM != requestMsg[1])
                {
                    // Skip this message and continue.
                    log<level::ERR>("Encountered Non-PLDM type message",
                                    entry("TYPE=0x%02x", requestMsg[1]));
                }
                else
                {
                    // process message and send response
                    auto response = processRxMsg(requestMsg);
                    if (!response.empty())
                    {
#ifdef VERBOSE
                        log<level::INFO>("Sending Msg ");
                        printBuffer(response);
#endif
                        iov[0].iov_base = &requestMsg[0];
                        iov[0].iov_len = sizeof(requestMsg[0]);
                        iov[1].iov_base = &requestMsg[1];
                        iov[1].iov_len = sizeof(requestMsg[1]);
                        iov[2].iov_base = response.data();
                        iov[2].iov_len = response.size();

                        sg_response.msg_control = nullptr;
                        sg_response.msg_controllen = 0;
                        sg_response.msg_flags = 0;
                        sg_response.msg_iov = iov;
                        sg_response.msg_iovlen = sizeof(iov) / sizeof(iov[0]);
                        sg_response.msg_name = nullptr;
                        sg_response.msg_namelen = 0;

                        result = sendmsg(socketFd(), &sg_response, 0);
                        if (-1 == result)
                        {
                            returnCode = -errno;
                            log<level::ERR>("sendto system call failed",
                                            entry("RC=%d", returnCode));
                            exit(EXIT_FAILURE);
                        }
                    }
                }
            }
            else
            {
                log<level::ERR>("Failure to read peeked length packet",
                                entry("PEEKED_LENGTH=%zu", peekedLength),
                                entry("READ_LENGTH=%zu", recvDataLength));
                exit(EXIT_FAILURE);
            }
        }
    } while (true);

    result = shutdown(sockfd, SHUT_RDWR);
    if (-1 == result)
    {
        returnCode = -errno;
        log<level::ERR>("Failed to shutdown the socket",
                        entry("RC=%d", returnCode));
        exit(EXIT_FAILURE);
    }
    exit(EXIT_FAILURE);
}
