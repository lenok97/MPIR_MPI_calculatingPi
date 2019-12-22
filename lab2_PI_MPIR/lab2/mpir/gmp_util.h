#pragma once
/*
	Programmed By Thomas Kim
	August 6, 2017
*/

#ifndef _GMP_UTIL_H
#define _GMP_UTIL_H

#ifdef _MSC_VER
	#pragma comment(lib, "mpir.lib")
	
	#pragma warning(push)
	#pragma warning(disable: 4244)
	
	#ifdef max
	#define _STUPID_MAX_DEFINED
	#pragma push_macro("max")
    #undef max
	#endif

	#ifdef min
	#define _STUPID_MIN_DEFINED
	#pragma push_macro("min")
	#undef min
	#endif

	#include <mpirxx.h>

	#ifdef _STUPID_MIN_DEFINED
	#undef _STUPID_MIN_DEFINED
    #pragma pop_macro("min")
	#endif

	#ifdef _STUPID_MAX_DEFINED
	#undef _STUPID_MAX_DEFINED 
	#pragma pop_macro("max")
	#endif

	#pragma warning(pop)
	#pragma comment(lib, "mpfr.lib")

#else
	#ifdef MPIR_COMPATIBLE
		#include <mpirxx.h>
	#else
		#include <gmpxx.h>
	#endif
#endif

#include <mpir/mpreal.h>
#include <clocale>
#include <iostream>
#include <sstream>
#include <string>
#include <cstring>
#include <type_traits>
#include <memory>

namespace tpf
{
	using mpfr::mpreal;

	inline mpfr_prec_t set_precision_digits(int digits)
	{
		mpfr_prec_t prec = mpfr::digits2bits(digits);
		mpreal::set_default_prec(prec);
		mpf_set_default_prec(prec);
		
		std::cout.precision(digits);
		std::wcout.precision(digits);
		
		return prec;
	}
	
    // convert from const char* to const wchar_t*

    inline std::wstring to_wstr(const char* mbstr)
    {
		if (mbstr == NULL || strlen(mbstr) == 0)
			return L"";

		#ifdef _MSC_VER
		
			std::mbstate_t state{}; // conversion state

			const char* p = mbstr;

			// get the number of characters
			// when successfully converted
			// https://docs.microsoft.com/en-US/cpp/c-runtime-library/reference/mbsrtowcs-s

			/*
				errno_t mbsrtowcs_s(  
				   size_t * pReturnValue,  
				   wchar_t * wcstr,  
				   size_t sizeInWords,  
				   const char ** mbstr,  
				   size_t count,  
				   mbstate_t * mbstate  
				);  

				Parameters
				pReturnValue
				The number of characters converted.

				wcstr
				Address of buffer to store the
				resulting converted wide character string.

				sizeInWords
				The size of wcstr in words (wide characters).

				mbstr
				Indirect pointer to the location of the multibyte
				character string to be converted.

				count
				The maximum number of wide characters to store
				in the wcstr buffer, not including the terminating null, or _TRUNCATE.

				mbstate
				A pointer to an mbstate_t conversion state object.
				If this value is a null pointer, a static internal
				conversion state object is used. Because the internal
				mbstate_t object is not thread-safe, we recommend that
				you always pass your own mbstate parameter.
			*/

			size_t clen; errno_t err;

			err = mbsrtowcs_s(&clen, NULL, 0, &p, 0, &state);

			// + 1 is redundant because
			// std::wstring manages terminating null character

			// failed to calculate
			// the character length of the converted string
			if (clen == 0 || clen == static_cast<size_t>(-1)
				|| err == EINVAL || err == EILSEQ || err == ERANGE)
			{
				return L""; // empty wstring
			}

			// reserve clen characters,
			// wstring reserves +1 character
			std::wstring rlt(clen, L'\0'); clen = 0;

			err = mbsrtowcs_s(&clen, &rlt[0], rlt.size(), &mbstr, rlt.size(), &state);

			// conversion failed
			if (clen == 0 || clen == static_cast<size_t>(-1)
				|| err == EINVAL || err == EILSEQ || err == ERANGE)
			{
				return L""; // empty string
			}
			else
			{
				// we have to remove the last character
				// because null character is added by mbsrtowcs_s()
				// also is added by std::wstring,
				// so we have to remove one trailing null character
				rlt.pop_back();
				return rlt;
			}
		
		#else
		
			std::mbstate_t state{}; // conversion state

			const char* p = mbstr;
			
			// get the number of characters
			// when successfully converted
			// http://en.cppreference.com/w/cpp/string/multibyte/mbsrtowcs
			size_t clen = mbsrtowcs(NULL, &p, 0 /* ignore */, &state);
				// + 1 is redundant because
				// std::wstring manages terminating null character

			// failed to calculate
			// the character length of the converted string
			if(clen == 0 || clen == static_cast<size_t>(-1))
			{
				return L""; // empty wstring
			}
			
			// reserve clen characters,
			// wstring reserves +1 character
			std::wstring rlt(clen, L'\0'); 

			size_t converted = mbsrtowcs(&rlt[0], &mbstr, rlt.size(), &state);
			
			// conversion failed
			if(converted == 
				static_cast<std::size_t>(-1))
			{
			return L""; // empty string
			}
			else
				return rlt;
		
		#endif
    }

    // convert from const wchar_t* to const char*
    //
    inline std::string to_str(const wchar_t* wcstr)
    {
        if(wcstr==NULL || wcslen(wcstr)==0)
            return ""; // empty string

		#ifdef _MSC_VER
			std::mbstate_t state{};
			const wchar_t* p = wcstr;

			// wcsrtombs - [wide char string] to [multi byte string]
			// https://docs.microsoft.com/en-US/cpp/c-runtime-library/reference/wcsrtombs-s

			size_t clen; errno_t err;
			err = wcsrtombs_s(&clen, NULL, 0, &p, 0, &state);
			// +1 is redundant, because
			// std::string manages terminating null character

			// cannot determine or convert to const char*
			if (clen == 0 || clen == static_cast<size_t>(-1)
				|| err == EINVAL || err == EILSEQ || err == ERANGE)
			{
				return ""; // empty string
			}

			std::string rlt(clen, '\0'); clen = 0;

			err = wcsrtombs_s(&clen, &rlt[0], rlt.size(), &wcstr, rlt.size(), &state);

			if (clen == 0 || clen == static_cast<size_t>(-1)
				|| err == EINVAL || err == EILSEQ || err == ERANGE)
			{
				return ""; // return empty string
			}
			else
			{
				// we have to remove the last character
				// because null character is added by wcsrtombs_s()
				// also is added by std::string,
				// so we have to remove one trailing null character
				rlt.pop_back();
				return rlt;
			}
		#else
			std::mbstate_t state{};

			const wchar_t* p = wcstr;

			// wcsrtombs - [wide char string] to [multi byte string]
			// http://en.cppreference.com/w/cpp/string/multibyte/wcsrtombs
			size_t clen = wcsrtombs(NULL, &p, 0, &state);
				// +1 is redundant, because
				// std::string manages terminating null character

			// cannot determine or convert to const char*
			if(clen == 0 || 
				clen == static_cast<std::size_t>(-1))
			{
				return ""; // empty string
			}

			std::string rlt(clen, '\0');

			size_t converted = 
				wcsrtombs(&rlt[0], &wcstr, rlt.size(), &state);

			if(converted == static_cast<std::size_t>(-1))
			{
				return ""; // return empty string
			}
			else
				return rlt;	
		#endif
    }

    inline std::string to_str(const std::wstring& wstr)
    {
        return to_str(wstr.c_str());
    }

    inline std::wstring to_wstr(const std::string& str)
    {
        return to_wstr(str.c_str());
    }

    inline std::ostream& operator<<(std::ostream& os, const std::wstring& wstr)
    {
        os << to_str(wstr);
        return os;
    }

    inline std::ostream& operator<<(std::ostream& os, const wchar_t* wstr)
    {
        os << to_str(wstr);
        return os;
    }

    inline std::wostream& operator<<(std::wostream& os, const std::string& str)
    {
        os << to_wstr(str);
        return os;
    }

    inline std::wostream& operator<<(std::wostream& os, const char* str)
    {
        os << to_wstr(str);
        return os;
    }
	
	
	template<typename T>
	typename std::enable_if<std::is_same<T, mpfr::mpreal>::value || std::is_same<T, mpf_class>::value, std::string>::type 
	to_string(const T& v)
	{
		std::ostringstream os;
		os.precision(std::cout.precision());
		os<<v; return os.str();
	}

	template<typename T>
	typename std::enable_if<std::is_same<T, mpfr::mpreal>::value || std::is_same<T, mpf_class>::value, std::wstring>::type
	to_wstring(const T& v)
	{  return to_wstr(to_string(v)); }
		
	std::string delimited_string(const mpz_class& v, char delimiter=',')
    {
        std::string str;
        
        {
            std::ostringstream ost;
            ost.precision(std::cout.precision());
            ost<<v; 
        
            str = ost.str();
			
            std::ostringstream os;

            // integral part
            int j, size = (int)str.size();
            int mode = size % 3;

            for (int i = 0; i < size; ++i)
            {
                os << str[i];

                j = i + 1;
                if (j % 3 == mode && j != size)
                    os << delimiter;
            }
			
			str = os.str();
        }

        return str;
    }
	
	std::wstring delimited_wstring(const mpz_class& v, wchar_t delimiter=L',')
    {
        std::wstring str;
        
        {
            std::ostringstream ost;
            ost.precision(std::cout.precision());
            ost<<v; 
        
            str = to_wstr(ost.str());
			
            std::wostringstream os;

            // integral part
            int j, size = (int)str.size();
            int mode = size % 3;

            for (int i = 0; i < size; ++i)
            {
                os << str[i];

                j = i + 1;
                if (j % 3 == mode && j != size)
                    os << delimiter;
            }
			
			str = os.str();
        }

        return str;
    }
	
	inline std::string to_string(const mpz_class& v)
	{
		std::ostringstream os;
		os<<v; return os.str();
	}
	
	inline std::wstring to_wstring(const mpz_class& v)
	{
		return to_wstr(to_string(v));
	}
	
	std::wostream& operator<<(std::wostream& os, const mpz_class& v)
	{
		os<<to_wstring(v); return os;
	}
		
		
    template<typename T>
	typename std::enable_if<std::is_same<T, mpfr::mpreal>::value||std::is_same<T, mpf_class>::value, std::string>::type
    delimited_string(const T& v, char delimiter = ',')
    {
        std::string str;
        
        {
            std::ostringstream os;
            os.precision(std::cout.precision());
            os<<v; str = os.str();
        }

        auto pos = str.find_first_of('.');
        
        if(pos != std::string::npos)
        {
            std::string int_part = str.substr(0, pos);
            std::string fra_part = str.substr(pos+1, str.size());

            std::ostringstream os;

            // integral part
            int j, size = (int)int_part.size();
            int mode = size % 3;

            for (int i = 0; i < size; ++i)
            {
                os << int_part[i];

                j = i + 1;
                if (j % 3 == mode && j != size)
                    os << delimiter;
            }

            os<<"."; // decimal point

            // fractional part
            size = (int)fra_part.size();
            mode = size % 3;

            for (int i = 0; i < size; ++i)
            {
                os << fra_part[i];

                j = i + 1;
                if ((j % 3) == 0 && j != size)
                    os << delimiter;
            }

            str = os.str();
        }
		else
		{
			std::ostringstream os;

            // integral part
            int j, size = (int)str.size();
            int mode = size % 3;

            for (int i = 0; i < size; ++i)
            {
                os << str[i];

                j = i + 1;
                if (j % 3 == mode && j != size)
                    os << delimiter;
            }
			
			str = os.str();
        }

        return str;
    }

    template<typename T>
	typename std::enable_if<std::is_same<T, mpfr::mpreal>::value||std::is_same<T, mpf_class>::value, std::wstring>::type
     delimited_wstring(const T& v, wchar_t delimiter=L',')
    {
        std::wstring str;
        
        {
            std::ostringstream os;
            os.precision(std::cout.precision());
            os<<v; str = to_wstr(os.str());
        }

        auto pos = str.find_first_of('.');
        
        if(pos != std::wstring::npos)
        {
            std::wstring int_part = str.substr(0, pos);
            std::wstring fra_part = str.substr(pos+1, str.size());

            std::wostringstream os;

            // integral part
            int j, size = (int)int_part.size();
            int mode = size % 3;

            for (int i = 0; i < size; ++i)
            {
                os << int_part[i];

                j = i + 1;
                if (j % 3 == mode && j != size)
                    os << delimiter;
            }

            os<<"."; // decimal point

            // fractional part
            size = (int)fra_part.size();
            mode = size % 3;

            for (int i = 0; i < size; ++i)
            {
                os << fra_part[i];

                j = i + 1;
                if ((j % 3) == 0 && j != size)
                    os << delimiter;
            }

            str = os.str();
        }
		else
		{
			std::wostringstream os;

            // integral part
            int j, size = (int)str.size();
            int mode = size % 3;

            for (int i = 0; i < size; ++i)
            {
                os << str[i];

                j = i + 1;
                if (j % 3 == mode && j != size)
                    os << delimiter;
            }
			
			str = os.str();
        }

        return str;
    }

    template<typename T>
	typename std::enable_if<std::is_same<T, mpfr::mpreal>::value||std::is_same<T, mpf_class>::value, std::wostream&>::type
    operator<<(std::wostream& os, const T& v)
    {
        os<<to_wstring(v); return os;
    }
}

#endif _GMP_UTIL_H
