#ifndef _STRINGADAPTER_H
#define _STRINGADAPTER_H
#include <string>

namespace xiaochufuji
{
#if defined(UNICODE) || defined(_UNICODE) 
	// use unicode set
#define _string std::wstring
#define _char wchar_t
#define _sizec (sizeof(wchar_t))
#define _to_string std::to_wstring
#define PRINT std::wcout 
#else
#define _string std::string
#define _char char
#define _sizec (sizeof(char))
#define _to_string std::to_string
#define PRINT std::cout 
#endif

	std::wstring StringToWString(const std::string& str);
	std::string WStringToString(const std::wstring& wstr);
}
#endif
