#pragma once

namespace vkengine
{
    template<typename T> class base_stringbuilder : public std::basic_stringstream<T>
    {
    public:
        base_stringbuilder() {}

        operator const std::basic_string<T>() const { return std::basic_stringstream<T>::str(); }
        base_stringbuilder<T>& operator<<   (bool _val) { std::basic_stringstream<T>::operator << (_val); return *this; }
        base_stringbuilder<T>& operator<<   (char _val) { std::basic_stringstream<T>::operator << (_val); return *this; }
        base_stringbuilder<T>& operator<<   (signed char _val) { std::basic_stringstream<T>::operator << (_val); return *this; }
        base_stringbuilder<T>& operator<<   (unsigned char _val) { std::basic_stringstream<T>::operator << (_val); return *this; }
        base_stringbuilder<T>& operator<<   (short _val) { std::basic_stringstream<T>::operator << (_val); return *this; }
        base_stringbuilder<T>& operator<<   (unsigned short _val) { std::basic_stringstream<T>::operator << (_val); return *this; }
        base_stringbuilder<T>& operator<<   (int _val) { std::basic_stringstream<T>::operator << (_val); return *this; }
        base_stringbuilder<T>& operator<<   (unsigned int _val) { std::basic_stringstream<T>::operator << (_val); return *this; }
        base_stringbuilder<T>& operator<<   (long _val) { std::basic_stringstream<T>::operator << (_val); return *this; }
        base_stringbuilder<T>& operator<<   (unsigned long _val) { std::basic_stringstream<T>::operator << (_val); return *this; }
        base_stringbuilder<T>& operator<<   (long long _val) { std::basic_stringstream<T>::operator << (_val); return *this; }
        base_stringbuilder<T>& operator<<   (unsigned long long _val) { std::basic_stringstream<T>::operator << (_val); return *this; }
        base_stringbuilder<T>& operator<<   (float _val) { std::basic_stringstream<T>::operator << (_val); return *this; }
        base_stringbuilder<T>& operator<<   (double _val) { std::basic_stringstream<T>::operator << (_val); return *this; }
        base_stringbuilder<T>& operator<<   (long double _val) { std::basic_stringstream<T>::operator << (_val); return *this; }
        base_stringbuilder<T>& operator<<   (void* _val) { std::basic_stringstream<T>::operator << (_val); return *this; }
        base_stringbuilder<T>& operator<<   (std::streambuf* _val) { std::basic_stringstream<T>::operator << (_val); return *this; }
        base_stringbuilder<T>& operator<<   (std::ostream& (*_val)(std::ostream&)) { std::basic_stringstream<T>::operator << (_val); return *this; }
        base_stringbuilder<T>& operator<<   (std::ios& (*_val)(std::ios&)) { std::basic_stringstream<T>::operator << (_val); return *this; }
        base_stringbuilder<T>& operator<<   (std::ios_base& (*_val)(std::ios_base&)) { std::basic_stringstream<T>::operator << (_val); return *this; }
        base_stringbuilder<T>& operator<<   (const T* _val) { return static_cast<base_stringbuilder<T>&>(std::operator << (*this, _val)); }
        base_stringbuilder<T>& operator<<   (const std::basic_string<T>& _val) { return static_cast<base_stringbuilder<T>&>(std::operator << (*this, _val.c_str())); }
    };

    typedef base_stringbuilder<char>        stringbuilder;
    typedef base_stringbuilder<wchar_t>     wstringbuilder;
}