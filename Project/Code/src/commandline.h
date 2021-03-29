#pragma once

#include "platform.h"
#include <string>


class CommandLineRAII
{
public:
	inline CommandLineRAII()
		: m_szArglist(0)
		, m_nArgs(0)
	{
		m_szArglist = ::CommandLineToArgvW(::GetCommandLineW(), &m_nArgs);
	}
	inline ~CommandLineRAII()
	{
		if (m_szArglist)
			::LocalFree(m_szArglist);
	}
	inline const LPWSTR* GetArgList() const
	{
		return m_szArglist;
	}
	inline const LPWSTR GetArgElement(size_t i) const
	{
		return m_szArglist[i];
	}
	inline int GetArgCount() const
	{
		return m_nArgs;
	}
	inline int FindArg(const wchar_t* name) const
	{
		if (m_szArglist)
			for (int i = 0; i < m_nArgs; ++i)
				if (::_wcsicmp(m_szArglist[i], name)==0)
					return i;
		return -1;
	}
	inline const wchar_t* FindArgAssignment(const wchar_t* name) const
	{
		const int i = FindArg(name);
		if (i >= 0 && i < m_nArgs-1)
			return m_szArglist[i+1];
		return NULL;
	}
	inline bool HasArg(const wchar_t* name) const
	{
		return FindArg(name) >= 0;
	}
private:
	LPWSTR* m_szArglist;
	int m_nArgs;
};

class CommandLineStrRef
{
public:
	inline CommandLineStrRef(std::wstring& str)
		: m_commandLine(str)
	{}
	inline CommandLineStrRef& operator=(CommandLineStrRef& other)
	{
		if (this != &other)
		{
			m_commandLine = other.m_commandLine;
		}
	}
	inline void AddAppCommandLine()
	{
		m_commandLine.append(::GetCommandLineW());
	}
	inline void AddArgument(const wchar_t* argument, const wchar_t* value = NULL)
	{
		if (!m_commandLine.empty())
		{
			m_commandLine.append(L" ");
		}
		m_commandLine.append(argument);
		if (value && *value)
		{
			m_commandLine.append(L" ");
			m_commandLine.append(value);
		}
	}
	inline void Swap(std::wstring& commandLine)
	{
		m_commandLine.swap(commandLine);
	}
	inline const wchar_t* GetStr() const
	{
		return m_commandLine.c_str();
	}
	inline const std::wstring& Get() const
	{
		return m_commandLine;
	}
private:
	std::wstring& m_commandLine;
};
