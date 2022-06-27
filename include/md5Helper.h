#include <iostream>
#include <cstring>

#include <openssl/md5.h>

class md5Helper
{
public:
    static std::string md5(const std::string src) {
        unsigned char mdStr[33] = {0};  
        MD5((const unsigned char *)src.c_str(), src.length(), mdStr);
        char buf[65] = {0};  
        char tmp[3] = {0};  
        for (int i = 0; i < 32; i++)  
        {  
            sprintf(tmp, "%02x", mdStr[i]);  
            strcat(buf, tmp);  
        }  
        buf[32] =0; // 后面都是0，从32字节截断    
        return std::string(buf);  
    }
};