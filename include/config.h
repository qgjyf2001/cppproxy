#ifndef CONFIG_H
#define CONFIG_H
#include <iostream>
#include <fstream>
#include <sstream>
#include "json/jsonParser.h"
class config {
public:
    static config& instance() {
        static config instance_;
        return instance_;
    }
    void init(std::string file) {
        std::ifstream fin;
        fin.open(file, std::ios::in);
        std::stringstream buf;
        buf << fin.rdbuf();
        std::string content=buf.str();
        json=JsonParser(&content);
        fin.close();
    }
    JsonParser json;
};
#endif
