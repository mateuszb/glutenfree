#include <http/TLSHandler.hpp>
#include <memory>

using namespace http;
using namespace std;

#ifdef TLS_DEBUG
static void my_debug(void *ctx, int level,
    const char *file, int line,
    const char *str)
{
    ((void)level);

    mbedtls_fprintf((FILE *)ctx, "%s:%04d: %s", file, line, str);
    fflush((FILE *)ctx);
}
#endif

int TLSHandler::tlsRecv(void* ctx, uint8_t* buf, size_t len) {
    TLSContext* tlsCtx = reinterpret_cast<TLSContext*>(ctx);
    TLSHandler* that = tlsCtx->handler;

    if (that->data.empty()) {
        return MBEDTLS_ERR_SSL_WANT_READ;
    }

    size_t actualLength = min(len, that->data.size());
    memcpy(buf, that->data.data(), actualLength);

    that->data.erase(that->data.begin(), that->data.begin() + actualLength);
    return actualLength;
}

int TLSHandler::tlsSend(void* ctx, const uint8_t* buf, size_t len) {
    TLSContext* tlsCtx = reinterpret_cast<TLSContext*>(ctx);
    TLSHandler* that = tlsCtx->handler;

    auto data = make_unique<vector<uint8_t>>(buf, buf + len);
    copy(buf, buf + len, data->begin());
    bool sent = that->transmit(std::move(data));
    if (!sent) {
        return int(len);
    }
 
    return int(len);
}

TLSHandler::TLSHandler(
    const io::net::DispatcherPtr& dispatcher,
    const io::net::SocketBasePtr& ptr)
    : ApplicationHandler(dispatcher, ptr),
    httpHandler(dispatcher, ptr) {

    mbedtls_ssl_init(&ssl);
    mbedtls_ssl_config_init(&sslConfig);
    mbedtls_x509_crt_init(&cert);
    mbedtls_pk_init(&privkey);
    mbedtls_entropy_init(&entropy);
    mbedtls_ctr_drbg_init(&drbg);

#ifdef TLS_DEBUG
    mbedtls_ssl_conf_dbg(&sslConfig, my_debug, stdout);
    mbedtls_debug_set_threshold(7);
#endif

    tlsContext.handler = this;

    int ret;

    ret = mbedtls_x509_crt_parse_file(
        &cert,
        "optimize.today.cert.pem");
    if (ret != 0) {
        throw runtime_error("Error parsing PEM file");
    }

    ret = mbedtls_pk_parse_keyfile(
        &privkey,
        "optimize.today.key.pem",
        nullptr);
    if (ret != 0) {
        throw runtime_error("Error parsing key file");
    }

    string seed = "optimizer";
    ret = mbedtls_ctr_drbg_seed(
        &drbg,
        mbedtls_entropy_func,
        &entropy,
        reinterpret_cast<const uint8_t*>(seed.c_str()),
        seed.size());
    if (ret != 0) {
        throw runtime_error("Error seeding entropy source");
    }

    ret = mbedtls_ssl_config_defaults(&sslConfig,
        MBEDTLS_SSL_IS_SERVER,
        MBEDTLS_SSL_TRANSPORT_STREAM,
        MBEDTLS_SSL_PRESET_DEFAULT);
    if (ret != 0) {
        throw runtime_error("Error configuring SSL");
    }


    mbedtls_ssl_conf_rng(&sslConfig, mbedtls_ctr_drbg_random, &drbg);
    mbedtls_ssl_conf_ca_chain(&sslConfig, cert.next, nullptr);

    ret = mbedtls_ssl_conf_own_cert(&sslConfig, &cert, &privkey);
    if (ret != 0) {
        throw runtime_error("Failed to configure cert");
    }

    ret = mbedtls_ssl_setup(&ssl, &sslConfig);
    if (ret != 0) {
        throw runtime_error("SSL setup failed");
    }

    mbedtls_ssl_session_reset(&ssl);

    mbedtls_ssl_set_bio(
        &ssl,
        &tlsContext,
        &TLSHandler::tlsSend,
        &TLSHandler::tlsRecv,
        nullptr);
}

TLSHandler::~TLSHandler()
{
}

unique_ptr<vector<uint8_t>> TLSHandler::HandleEvent(
    const uint32_t eventMask,
    const vector<uint8_t>& ciphertext) {

    if (ciphertext.empty()) {
        return nullptr;
    }

    copy(ciphertext.begin(), ciphertext.end(), back_inserter(data));

    if (!sessionEstablished) {
        int ret = mbedtls_ssl_handshake(&ssl);

        if (ret != 0) {
            if (ret == MBEDTLS_ERR_SSL_WANT_READ || ret == MBEDTLS_ERR_SSL_WANT_WRITE) {
                return nullptr;
            } else {
                mbedtls_printf(" failed\n  ! mbedtls_ssl_handshake returned %d\n\n", ret);
                throw runtime_error("Failed TLS handshake");
            }
        }

        sessionEstablished = true;
    }

    if (data.empty()) {
        return nullptr;
    }

    vector<uint8_t> plaintext(4096u);
    int nread = mbedtls_ssl_read(&ssl, plaintext.data(), plaintext.size());
    if (nread == 0) {
        sessionEstablished = false;
        return nullptr;
    }

    if (nread > 0) {
        plaintext.resize(nread);
        auto response = httpHandler.HandleEvent(0, plaintext);
        if (response == nullptr) {
            return nullptr;
        }

        int nwritten = 0;

    retry:
        nwritten = mbedtls_ssl_write(&ssl, response->data(), response->size());
        if (nwritten == MBEDTLS_ERR_SSL_WANT_WRITE) {
            goto retry;
        }

        if (nwritten <= response->size()) {
            response->erase(response->begin(), response->begin() + nwritten);
            if (!response->empty()) {
                goto retry;
            }
        }

        if (nwritten < 0) {
            throw runtime_error("Error sending TLS data");
        }
    } else {
        if (nread == MBEDTLS_ERR_SSL_CONN_EOF) {
            return nullptr;
        } else if (nread == MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY) {
            return nullptr;
        }
        else {
            throw runtime_error("Error reading TLS data");
        }
    }

    return nullptr;
}

io::ApplicationHandlerPtr TLSHandlerFactory::Make(io::net::SocketBasePtr socket)
{
    using namespace io;
    using namespace io::net;
    ApplicationHandlerPtr handler;

    auto dispatcher = Dispatcher::Instance();
    handler = make_shared<TLSHandler>(dispatcher, socket);
    dispatcher->AddHandler(handler, EventType::Read | EventType::Write);
    return handler;
}
