#pragma once
#include <http/HttpHandler.hpp>
#include <io/ApplicationHandler.hpp>
#include <mbedtls/config.h>
#include <mbedtls/platform.h>
#include <mbedtls/entropy.h>
#include <mbedtls/ctr_drbg.h>
#include <mbedtls/certs.h>
#include <mbedtls/x509.h>
#include <mbedtls/ssl.h>
#include <mbedtls/net_sockets.h>
#include <mbedtls/error.h>
#include <mbedtls/debug.h>
#include <functional>
#include <vector>


namespace http {
class TLSHandler :
    public io::ApplicationHandler
{
public:
    TLSHandler(
        const io::net::DispatcherPtr& dispatcher,
        const io::net::SocketBasePtr& ptr);
    ~TLSHandler();

    // Inherited via ApplicationHandler
    std::unique_ptr<std::vector<uint8_t>> HandleEvent(
        const uint32_t eventMask,
        const std::vector<uint8_t>& input) override;

private:
    struct TLSContext {
        TLSHandler* handler;
    };
    mbedtls_ssl_context ssl;
    mbedtls_ssl_config sslConfig;
    mbedtls_entropy_context entropy;
    mbedtls_ctr_drbg_context drbg;
    TLSContext tlsContext;
    mbedtls_x509_crt cert;
    mbedtls_pk_context privkey;
    bool sessionEstablished{ false };
    std::vector<uint8_t> data;

    HttpHandler httpHandler;

    static int tlsRecv(void* ctx, uint8_t* buf, size_t len);
    static int tlsSend(void* ctx, const uint8_t* buf, size_t len);
};

class TLSHandlerFactory {
public:
    static io::ApplicationHandlerPtr Make(io::net::SocketBasePtr socket);
};

}
