#include "SSLManager.h"
#include "threadPool.h"

SSLManager::SSLManager() {
    static std::once_flag flag;
    std::call_once(flag,[](){
        SSL_load_error_strings();
        SSL_library_init();
        OpenSSL_add_all_algorithms();
  
        static std::string certFile="registry.crt";
        static std::string keyFile="registry.key";
        
        serverCtx = SSL_CTX_new(SSLv23_method());
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
        SSL_CTX_set_options(serverCtx,SSL_OP_NO_TLSv1_3);
        
        clientCtx = SSL_CTX_new(TLS_client_method());
        SSL_CTX_set_verify(clientCtx, SSL_VERIFY_NONE, NULL);
        SSL_CTX_set_options(clientCtx,SSL_OP_NO_TLSv1_3);
    });
    server=std::make_shared<SSLClient>(
        SSL_new(serverCtx),BIO_new(BIO_s_mem()),BIO_new(BIO_s_mem()));
    SSL_set_bio(server->ssl, server->rio, server->wio);

    client=std::make_shared<SSLClient>(
        SSL_new(clientCtx),BIO_new(BIO_s_mem()),BIO_new(BIO_s_mem()));
    SSL_set_bio(client->ssl, client->rio, client->wio);

    SSL_accept(server->ssl);
    SSL_connect(client->ssl);
}
std::string SSLManager::writeProxy(std::string content,filterReason &reason,std::function<std::string(std::string)> filter) {
    char buf[MAXLINE];
    int n;
    if (SSL_is_init_finished(server->ssl) && !SSL_is_init_finished(client->ssl)) {    
        n=BIO_read(client->wio, buf, MAXLINE);
        //std::cout<<"SSL client not finished"<<n<<std::endl;
        reason=filterReason::REUSED;
        if (n>0) {
            return std::string(buf,n);
        } else {
            return "";
        }
    }
    BIO_write(server->rio, content.data(), content.length());
    n = SSL_read(server->ssl, buf, MAXLINE);
    if (n>0) {
        std::string buffer=std::string(buf,n);
        while ((n=SSL_read(server->ssl, buf, MAXLINE))>0) {
            buffer+=std::string(buf,n);
        }
        std::string filtered=filter(buffer);
        SSL_write(client->ssl,filtered.data(),filtered.length());
    }
    if (!SSL_is_init_finished(server->ssl)) { 
        int ret=SSL_accept(server->ssl);
        if (SSL_get_error(server->ssl,ret)==SSL_ERROR_SSL) {
            throw std::runtime_error("SSL error");
        } 
    }
    n=BIO_read(client->wio, buf, MAXLINE);
    if (n<=0) {
        return "";
    }
    std::string result=std::string(buf,n);
    while ((n=BIO_read(client->wio, buf, MAXLINE))>0) {
        result+=std::string(buf,n);
    }
    return result;
}
std::string SSLManager::writeClient(std::string content,filterReason &reason,std::function<std::string(std::string)> filter) {
    int n;
    char buf[MAXLINE];
    BIO_write(client->rio, content.data(), content.length());
    n=SSL_read(client->ssl, buf, MAXLINE);
    if (n>0) {
        std::string buffer=std::string(buf,n);
        while ((n=SSL_read(client->ssl, buf, MAXLINE))>0) {
            buffer+=std::string(buf,n);
        }
        std::string filtered=filter(buffer);
        SSL_write(server->ssl,filtered.data(),filtered.length());
    }
    n=BIO_read(server->wio, buf, MAXLINE);
    if (n<=0) {
        return "";
    }
    std::string result=std::string(buf,n);
    while ((n=BIO_read(server->wio, buf, MAXLINE))>0) {
        result+=std::string(buf,n);
    }
    return result;
}
SSL_CTX* SSLManager::serverCtx=nullptr;
SSL_CTX* SSLManager::clientCtx=nullptr;