#include "stringUtils.h"
void replaceString(std::string& str, const std::string& be_replaced_str, const std::string& new_replace_str)
{
    for (std::string::size_type pos = 0; pos != std::string::npos; pos += new_replace_str.length())
    {
        pos = str.find(be_replaced_str, pos);
        if (pos != std::string::npos)
        {
            str.replace(pos, be_replaced_str.length(), new_replace_str);
        }
        else
        {
            break;
        }
    }
}