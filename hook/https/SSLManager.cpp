#include "SSLManager.h"
#include "threadPool.h"

SSLManager::SSLManager() {
    std::once_flag flag;
    std::call_once(flag,[](){
        SSL_library_init();
        SSL_load_error_strings();
        ERR_load_BIO_strings();
        OpenSSL_add_all_algorithms();
        static std::string certFile="key.pem";
        static std::string keyFile="key.pem";
        serverCtx = SSL_CTX_new(SSLv23_server_method());
        SSL_CTX_set_verify(serverCtx, SSL_VERIFY_NONE, NULL);
        if ( SSL_CTX_use_certificate_file(serverCtx, certFile.c_str(), SSL_FILETYPE_PEM) <= 0 ) {
            printf("Error loading certFile\n");
            exit(1);
        }
        if ( SSL_CTX_use_PrivateKey_file(serverCtx, keyFile.c_str(), SSL_FILETYPE_PEM) <= 0 ) {
            printf("Error loading keyFile\n");
            exit(1);
        }
        if ( !SSL_CTX_check_private_key(serverCtx) ) {
            printf("Error with Key\n");
            exit(1);
        }
    });
    server=std::make_shared<SSLClient>(
        SSL_new(serverCtx),BIO_new(BIO_s_mem()),BIO_new(BIO_s_mem()));
    SSL_set_bio(server->ssl, server->rio, server->wio);

    client=std::make_shared<SSLClient>(
        SSL_new(serverCtx),BIO_new(BIO_s_mem()),BIO_new(BIO_s_mem()));
    SSL_set_bio(client->ssl, client->rio, client->wio);
    SSL_accept(server->ssl);
    SSL_connect(client->ssl);
}
std::string SSLManager::writeProxy(std::string content,std::function<std::string(std::string)> filter) {
    char buf[MAXLINE];
    BIO_write(server->rio, content.data(), content.length());
    int n = SSL_read(server->ssl, buf, MAXLINE);
    if (n!=0) {
        std::string filtered=filter(std::string(buf,n));
        SSL_write(client->ssl,filtered.data(),filtered.length());
    }
    n=BIO_read(client->wio, buf, MAXLINE);
    return std::string(buf,n);
}
std::string SSLManager::writeClient(std::string content,std::function<std::string(std::string)> filter) {
    char buf[MAXLINE];
    BIO_write(client->rio, content.data(), content.length());
    int n = SSL_read(client->ssl, buf, MAXLINE);
    if (n!=0) {
        std::string filtered=filter(std::string(buf,n));
        SSL_write(server->ssl,filtered.data(),filtered.length());
    }
    n=BIO_read(server->wio, buf, MAXLINE);
    return std::string(buf,n);
}
SSL_CTX* SSLManager::serverCtx=nullptr;