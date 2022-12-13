#ifndef SSLMANAGER_H
#define SSLMANAGER_H
#include "../filter.h"
#include "openssl/ssl.h"

struct SSLClient {
  SSL* ssl;
  BIO* rio;
  BIO* wio;
  SSLClient(SSL* ssl,BIO* rio,BIO* wio):ssl(ssl),rio(rio),wio(wio) {

  }
  ~SSLClient() {
    SSL_free(ssl);
  }
};

class SSLManager {
public:
    SSLManager();
    std::string writeClient(std::string content,std::function<std::string(std::string)> filter=SSLManager::nonFilter);
    std::string writeProxy(std::string content,std::function<std::string(std::string)> filter=SSLManager::nonFilter);
private:
    static std::string nonFilter(std::string s) {
      return s;
    }
    static SSL_CTX* serverCtx;
    static SSL_CTX* clientCtx;
    std::shared_ptr<SSLClient> server,client;
    std::string buffer;
};
#endif