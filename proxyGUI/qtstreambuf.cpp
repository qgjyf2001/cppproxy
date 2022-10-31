#include "qtstreambuf.h"
#include "mainwindow.h"

std::streamsize qtStreamBuf::xsputn(const char* __s, std::streamsize __n)
{
    std::string s(__s,__n);
    window->Append(QString::fromStdString(s));
    return __n;
}
qtStreamBuf::int_type qtStreamBuf::syncput(qtStreamBuf::int_type c) {
    window->Append(QString(char(c)));
    return c;
}
