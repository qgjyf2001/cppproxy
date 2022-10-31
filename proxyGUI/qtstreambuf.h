#ifndef QTSTREAMBUF_H
#define QTSTREAMBUF_H

#include <streambuf>
#include <bits/c++io.h>  // For __c_file

class MainWindow;
using _CharT=char;
using _Traits = std::char_traits<_CharT>;
class qtStreamBuf : public std::basic_streambuf<_CharT, _Traits>
{
public:
    typedef _CharT char_type;
    typedef _Traits traits_type;
    typedef typename traits_type::int_type int_type;
private:
    MainWindow* window;
    int_type syncput(int_type c);
public:
      qtStreamBuf(MainWindow* window)
          : window(window)
      { }
protected:
      virtual int_type overflow(int_type __c = traits_type::eof())
      {
          int_type __ret;
          if (traits_type::eq_int_type(__c, traits_type::eof()))
          {
              __ret = traits_type::not_eof(__c);
          }
          else
              __ret = syncput(__c);
          return __ret;
       }
      virtual std::streamsize xsputn(const char_type* __s, std::streamsize __n);
    };

#endif
