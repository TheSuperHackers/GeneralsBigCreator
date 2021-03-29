#pragma once

#include "types.h"
#include <string>
#include <vector>
#include <iostream>

namespace fileaccess
{
	enum EError
	{
		eError_Success = 0,
		eError_FileAccessError,
		eError_FileException,
		eError_ReadError,
		eError_WriteError,
	};

	typedef std::string TStringData;
	typedef std::vector<char> TVectorData;

	EError ReadDataFromFile(const wchar_t* fileName, TStringData& data);
	EError ReadDataFromFile(const wchar_t* fileName, TVectorData& data);

	EError WriteDataToFile(const wchar_t* fileName, const TStringData& data);
	EError WriteDataToFile(const wchar_t* fileName, const TVectorData& data);

	EError ReadDataFromStream(std::istream& stream, TStringData& data, size_t offset = 0u);
	EError ReadDataFromStream(std::istream& stream, TVectorData& data, size_t offset = 0u);

	EError WriteDataToStream(std::ostream& stream, const TStringData& data, size_t offset = 0xFFFFFFFFu);
	EError WriteDataToStream(std::ostream& stream, const TVectorData& data, size_t offset = 0xFFFFFFFFu);
};
