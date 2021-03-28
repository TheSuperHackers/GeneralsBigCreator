#include "BIGFile.h"
#include "platform.h"
#include "utils.h"

#if _MSC_VER
#pragma warning(disable: 4996)
#endif

const char CBIGFile::s_simplified_charset[256] = 
{
	'\x00', '\x01', '\x02', '\x03', '\x04', '\x05', '\x06', '\x07',
	'\x08', '\x09', '\x0A', '\x0B', '\x0C', '\x0D', '\x0E', '\x0F',
	'\x10', '\x11', '\x12', '\x13', '\x14', '\x15', '\x16', '\x17',
	'\x18', '\x19', '\x1A', '\x1B', '\x1C', '\x1D', '\x1E', '\x1F',
	'\x20', '\x21', '\x22', '\x23', '\x24', '\x25', '\x26', '\x27',
	'\x28', '\x29', '\x2A', '\x2B', '\x2C', '\x2D', '\x2E', '\\',
	'\x30', '\x31', '\x32', '\x33', '\x34', '\x35', '\x36', '\x37',
	'\x38', '\x39', '\x3A', '\x3B', '\x3C', '\x3D', '\x3E', '\x3F',
	'\x40', 'a',    'b',    'c',    'd',    'e',    'f',    'g',
	'h',    'i',    'j',    'k',    'l',    'm',    'n',    'o',
	'p',    'q',    'r',    's',    't',    'u',    'v',    'w',
	'x',    'y',    'z',    '\x5B', '\x5C', '\x5D', '\x5E', '\x5F',
	'\x60', '\x61', '\x62', '\x63', '\x64', '\x65', '\x66', '\x67',
	'\x68', '\x69', '\x6A', '\x6B', '\x6C', '\x6D', '\x6E', '\x6F',
	'\x70', '\x71', '\x72', '\x73', '\x74', '\x75', '\x76', '\x77',
	'\x78', '\x79', '\x7A', '\x7B', '\x7C', '\x7D', '\x7E', '\x7F',
	'\x80', '\x81', '\x82', '\x83', '\x84', '\x85', '\x86', '\x87',
	'\x88', '\x89', '\x8A', '\x8B', '\x8C', '\x8D', '\x8E', '\x8F',
	'\x90', '\x91', '\x92', '\x93', '\x94', '\x95', '\x96', '\x97',
	'\x98', '\x99', '\x9A', '\x9B', '\x9C', '\x9D', '\x9E', '\x9F',
	'\xA0', '\xA1', '\xA2', '\xA3', '\xA4', '\xA5', '\xA6', '\xA7',
	'\xA8', '\xA9', '\xAA', '\xAB', '\xAC', '\xAD', '\xAE', '\xAF',
	'\xB0', '\xB1', '\xB2', '\xB3', '\xB4', '\xB5', '\xB6', '\xB7',
	'\xB8', '\xB9', '\xBA', '\xBB', '\xBC', '\xBD', '\xBE', '\xBF',
	'\xC0', '\xC1', '\xC2', '\xC3', '\xC4', '\xC5', '\xC6', '\xC7',
	'\xC8', '\xC9', '\xCA', '\xCB', '\xCC', '\xCD', '\xCE', '\xCF',
	'\xD0', '\xD1', '\xD2', '\xD3', '\xD4', '\xD5', '\xD6', '\xD7',
	'\xD8', '\xD9', '\xDA', '\xDB', '\xDC', '\xDD', '\xDE', '\xDF',
	'\xE0', '\xE1', '\xE2', '\xE3', '\xE4', '\xE5', '\xE6', '\xE7',
	'\xE8', '\xE9', '\xEA', '\xEB', '\xEC', '\xED', '\xEE', '\xEF',
	'\xF0', '\xF1', '\xF2', '\xF3', '\xF4', '\xF5', '\xF6', '\xF7',
	'\xF8', '\xF9', '\xFA', '\xFB', '\xFC', '\xFD', '\xFE', '\xFF'
};

CBIGFile::CBIGFile()
: m_fileId(0)
, m_flags(0)
, m_hasPendingFileChanges(false)
{
}

bool CBIGFile::OpenFile(const wchar_t* fileName, TFlags flags)
{
	bool success = false;

	if (HasBigFileExtension(fileName))
	{
		CloseFile();
		m_filename = fileName;
		m_flags = flags;
		OpenInputFileStream();

		if (m_ifstream.is_open())
		{
			TData headerData;
			headerData.resize(m_bigHeader.SizeOnDisk());

			if (ReadDataFromStream(headerData, m_ifstream))
			{
				if (ReadBigHeaderFromData(m_bigHeader, headerData))
				{
#ifndef _RELEASE
					m_ifstream.seekg(0, std::ios::end);
					assert(m_bigHeader.HasExpectedFileSize(m_ifstream.tellg()));
#endif
					const uint32 fileHeadersOffset = m_bigHeader.SizeOnDisk();
					const uint32 lastHeaderOffset = m_bigHeader.SizeOnDisk() + m_bigHeader.FileHeaderSize();
					headerData.resize(m_bigHeader.headerSize);

					if (ReadDataFromStream(headerData, m_ifstream))
					{
						if (ReadFileHeadersFromData(m_fileHeaders, m_bigHeader, headerData, fileHeadersOffset, m_flags))
						{
							if (ReadLastHeaderFromData(m_lastHeader, headerData, lastHeaderOffset))
							{
								m_fileDataVector.resize(m_fileHeaders.size());
								CreateFileHeaderIndices(m_fileHeaderIndices, m_fileHeaders);
								success = true;
							}
						}
					}
				}
			}
		}

		if (!success)
		{
			CloseFile();
		}
	}

	return success;
}

void CBIGFile::CloseFile()
{
	if (IsOpen())
	{
		WriteOutPendingFileChanges();

		m_bigHeader = SBigHeader();
		m_lastHeader = SLastHeader();
		utils::ClearMemory(m_fileHeaders);
		utils::ClearMemory(m_fileDataVector);
		utils::ClearMemory(m_fileHeaderIndices);
		utils::ClearMemory(m_filename);
		CloseInputFileStream();
		m_fileId = 0u;
		m_flags = eFlags_None;
	}
}

void CBIGFile::OpenInputFileStream()
{
	std::ios::openmode mode = std::ios::in | std::ios::ate | std::ios::binary;
	m_ifstream.open(m_filename.c_str(), mode);
}

void CBIGFile::CloseInputFileStream()
{
	m_ifstream.close();
}

bool CBIGFile::ReadDataFromStream(TData& data, std::istream& istream, uint32 offset)
{
	if (data.empty())
	{
		return true;
	}

	try
	{
		assert(istream.good());

		if (istream.good())
		{
			istream.seekg(0, std::ios::end);
			const uint64 fileSize = istream.tellg();

			assert(data.size() <= fileSize - offset);

			if ((data.size() <= fileSize - offset))
			{
				istream.seekg(offset, std::ios::beg);
				istream.read(&data[0], data.size());

				return istream.good();
			}
		}
	}
	catch (std::exception&)
	{
		assert(0);
	}

	return false;
}

bool CBIGFile::WriteDataToStream(const TData& data, std::ostream& ostream, uint32 offset)
{
	if (data.empty())
	{
		return true;
	}

	try
	{
		assert(ostream.good());

		if (ostream.good())
		{
			ostream.seekp(0, std::ios::end);
			const uint64 fileSize = ostream.tellp();
			const uint64 maxFileSize = 0xFFFFFFFFull;
			offset = std::min(offset, (uint32)fileSize);

			assert((uint64)offset + (uint64)data.size() <= maxFileSize);

			if ((uint64)offset + (uint64)data.size() <= maxFileSize)
			{
				ostream.seekp(offset, std::ios::beg);
				ostream.write(&data[0], data.size());

				return ostream.good();
			}
		}
	}
	catch (std::exception&)
	{
		assert(0);
	}

	return false;
}

bool CBIGFile::ReadBigHeaderFromData(SBigHeader& bigHeader, const TData& data)
{
	assert(data.size() >= bigHeader.SizeOnDisk());

	if (data.size() >= bigHeader.SizeOnDisk())
	{
		bigHeader.bigf        = utils::GetInvert(*reinterpret_cast<const uint32*>(&data[0]));
		bigHeader.bigFileSize =                  *reinterpret_cast<const uint32*>(&data[4]);
		bigHeader.fileCount   = utils::GetInvert(*reinterpret_cast<const uint32*>(&data[8]));
		bigHeader.headerSize  = utils::GetInvert(*reinterpret_cast<const uint32*>(&data[12]));

		return bigHeader.IsGood();
	}
	return false;
}

bool CBIGFile::WriteBigHeaderToData(TData& data, const SBigHeader& bigHeader)
{
	assert(data.size() >= bigHeader.SizeOnDisk());

	if (data.size() >= bigHeader.SizeOnDisk())
	{
		*reinterpret_cast<uint32*>(&data[0])  = utils::GetInvert(bigHeader.bigf);
		*reinterpret_cast<uint32*>(&data[4])  =                  bigHeader.bigFileSize;
		*reinterpret_cast<uint32*>(&data[8])  = utils::GetInvert(bigHeader.fileCount);
		*reinterpret_cast<uint32*>(&data[12]) = utils::GetInvert(bigHeader.headerSize);

		return true;
	}
	return false;
}

bool CBIGFile::ReadFileHeadersFromData(TFileHeaders& fileHeaders, const SBigHeader& bigHeader, const TData& data, uint32 offset, TFlags flags)
{
	const uint32 fileHeaderSize = bigHeader.FileHeaderSize();
	const uint32 fileCount = bigHeader.fileCount;

	assert(data.size() >= fileHeaderSize + offset);

	if (data.size() >= fileHeaderSize + offset)
	{
		fileHeaders.clear();
		fileHeaders.reserve(fileCount);

		uint32 dataIndex = offset;

		for (uint32 fileIndex = 0; fileIndex < fileCount; ++fileIndex)
		{
			fileHeaders.push_back(SFileHeader());
			SFileHeader& newFileHeader = fileHeaders.back();

			newFileHeader.offset = utils::GetInvert(*reinterpret_cast<const uint32*>(&data[dataIndex]));
			dataIndex += sizeof(uint32);

			newFileHeader.size = utils::GetInvert(*reinterpret_cast<const uint32*>(&data[dataIndex]));
			dataIndex += sizeof(uint32);

			newFileHeader.name = reinterpret_cast<const char*>(&data[dataIndex]);
			newFileHeader.simplifiedName = newFileHeader.name;

			assert(newFileHeader.name.size() == newFileHeader.simplifiedName.size());

			const uint32 nameLen = newFileHeader.name.size();
			dataIndex += nameLen + 1;

			// It is possible to use either kind of slashes in an embedded file
			// The game can work with both slashes "\" and "/"
			// For internal simplification it makes sense to default all slashes to "\"
			if (flags & eFlags_UseSimplifiedName)
			{
				ApplySimplifiedCharset(newFileHeader.simplifiedName);
			}

			// It is possible to embed files with the same name in a big file
			// The game will always load the last file with that name
			// Ignore previous duplicates to avoid inconsistent results
			if (flags & eFlags_IgnoreDuplicates)
			{
				const uint32 headerCount = fileHeaders.size() - 1;
				for (uint32 headerId = 0; headerId < headerCount; ++headerId)
				{
					const std::string& otherFileName = fileHeaders[headerId].simplifiedName;
					const std::string& newFileName = newFileHeader.simplifiedName;

					if (otherFileName == newFileName)
					{
						fileHeaders[headerId].ignore = true;
					}
				}
			}
		}
		
		assert(fileHeaders.size() == fileCount);

		for (uint32 fileIndex = 0; fileIndex < fileCount; ++fileIndex)
		{
			// Check that file headers are healthy
			assert(!fileHeaders[fileIndex].name.empty());
			assert((fileHeaders[fileIndex].offset + fileHeaders[fileIndex].size) <= bigHeader.bigFileSize);
		}

		return fileHeaders.size() != 0;
	}
	return false;
}

bool CBIGFile::WriteFileHeadersToData(TData& data, const TFileHeaders& fileHeaders, uint32 offset)
{
	const uint32 fileHeaderSize = GetSizeOnDisk(fileHeaders);
	const uint32 fileCount = fileHeaders.size();

	assert(data.size() >= fileHeaderSize + offset);

	if (data.size() >= fileHeaderSize + offset)
	{
		uint32 dataIndex = offset;
		
		for (uint32 fileIndex = 0; fileIndex < fileCount; ++fileIndex)
		{
			const SFileHeader& fileHeader = fileHeaders[fileIndex];

			*reinterpret_cast<uint32*>(&data[dataIndex]) = utils::GetInvert(fileHeader.offset);
			dataIndex += sizeof(uint32);

			*reinterpret_cast<uint32*>(&data[dataIndex]) = utils::GetInvert(fileHeader.size);
			dataIndex += sizeof(uint32);

			::strcpy(reinterpret_cast<char*>(&data[dataIndex]), fileHeader.name.c_str());
			dataIndex += fileHeader.name.size() + 1;
		}

		return true;
	}

	return false;
}

bool CBIGFile::ReadLastHeaderFromData(SLastHeader& lastHeader, const TData& data, uint32 offset)
{
	assert(data.size() >= lastHeader.SizeOnDisk() + offset);

	if (data.size() >= lastHeader.SizeOnDisk() + offset)
	{
		lastHeader.unknown1 = *reinterpret_cast<const uint32*>(&data[offset]);
		lastHeader.unknown2 = *reinterpret_cast<const uint32*>(&data[offset + 4]);
		return true;
	}
	return false;
}

bool CBIGFile::WriteLastHeaderToData(TData& data, const SLastHeader& lastHeader, uint32 offset)
{
	assert(data.size() >= lastHeader.SizeOnDisk() + offset);

	if (data.size() >= lastHeader.SizeOnDisk() + offset)
	{
		*reinterpret_cast<uint32*>(&data[offset])     = lastHeader.unknown1;
		*reinterpret_cast<uint32*>(&data[offset + 4]) = lastHeader.unknown2;
		return true;
	}
	return false;
}

void CBIGFile::CreateFileHeaderIndices(TIntegers& fileHeaderIndices, const TFileHeaders& fileHeaders)
{
	const uint32 fileCount = fileHeaders.size();
	fileHeaderIndices.clear();
	fileHeaderIndices.reserve(fileCount);

	for (uint32 fileIndex = 0; fileIndex < fileCount; ++fileIndex)
	{
		// Some file headers might be of no use, if the game will not load them
		// Build an indices vector to translate an id to the real file index
		if (!fileHeaders[fileIndex].ignore)
		{
			fileHeaderIndices.push_back(fileIndex);
		}
	}
}

void CBIGFile::UpdateBigHeaderAndFileHeaders(SBigHeader& bigHeader, TFileHeaders& fileHeaders, const TDataVector& fileDataVector)
{
	assert(fileHeaders.size() == fileDataVector.size());

	bigHeader.bigFileSize = 0;
	bigHeader.bigFileSize += bigHeader.SizeOnDisk();
	bigHeader.bigFileSize += GetSizeOnDisk(fileHeaders);
	bigHeader.bigFileSize += SLastHeader::SizeOnDisk();
	bigHeader.headerSize = bigHeader.bigFileSize;
	bigHeader.fileCount = fileHeaders.size();

	for (uint32 fileIndex = 0; fileIndex < bigHeader.fileCount; ++fileIndex)
	{
		SFileHeader& fileHeader = fileHeaders[fileIndex];
		const TData& fileData = fileDataVector[fileIndex];

		fileHeader.offset = bigHeader.bigFileSize;
		if (fileData.size() != 0)
			fileHeader.size = fileData.size();
		bigHeader.bigFileSize += fileHeader.size;
	}
}

uint32 CBIGFile::GetSizeOnDisk(const TFileHeaders& fileHeaders)
{
	uint32 sizeOnDisk = 0;
	const uint32 fileCount = fileHeaders.size();
	for (uint32 fileIndex = 0; fileIndex < fileCount; ++fileIndex)
	{
		sizeOnDisk += fileHeaders[fileIndex].SizeOnDisk();
	}
	return sizeOnDisk;
}

uint32 CBIGFile::GetMaxFileSize(const TFileHeaders& fileHeaders)
{
	uint32 maxFileSize = 0;
	const uint32 fileCount = fileHeaders.size();
	for (uint32 fileIndex = 0; fileIndex < fileCount; ++fileIndex)
	{
		maxFileSize = std::max(maxFileSize, fileHeaders[fileIndex].size);
	}
	return maxFileSize;
}

const CBIGFile::SFileHeader* CBIGFile::GetFileHeader(uint32 id) const
{
	if (id < m_fileHeaderIndices.size())
	{
		const uint32 index = m_fileHeaderIndices[id];
		return &m_fileHeaders[index];
	}
	return NULL;
}

const char* CBIGFile::GetFileNameById(uint32 id) const
{
	if (const SFileHeader* pFileHeader = GetFileHeader(id))
	{
		return pFileHeader->simplifiedName.c_str();
	}
	return NULL;
}

const char* CBIGFile::GetCurrentFileName() const
{
	return GetFileNameById(m_fileId);
}

const char* CBIGFile::GetFirstFileName()
{
	m_fileId = 0;
	return GetFileNameById(m_fileId);
}

const char* CBIGFile::GetNextFileName()
{
	return GetFileNameById(++m_fileId);
}

bool CBIGFile::ReadFileDataById(uint32 id, TData& data)
{
	bool success = false;
	if (id < m_fileHeaderIndices.size())
	{
		const uint32 index = m_fileHeaderIndices[id];
		const TData& internalData = m_fileDataVector[index];

		if (!internalData.empty())
		{
			// Get data that is not yet written out to the .big file
			data = internalData;
			success = true;
		}
		else
		{
			// Get data that is in the .big file on disk
			const SFileHeader& fileHeader = m_fileHeaders[index];
			data.resize(fileHeader.size);
			success = ReadDataFromStream(data, m_ifstream, fileHeader.offset);
		}
	}
	return success;
}

bool CBIGFile::WriteFileDataById(uint32 id, const TData& data, bool immediateWriteOut)
{
	bool success = false;
	if (id < m_fileHeaderIndices.size())
	{
		const uint32 index = m_fileHeaderIndices[id];
		m_fileDataVector[index] = data;

		if (immediateWriteOut)
		{
			success = WriteOutPendingFileChanges();
			if (!success)
				m_hasPendingFileChanges = true;
		}
		else
		{
			success = true;
			m_hasPendingFileChanges = true;
		}
	}
	return success;
}

bool CBIGFile::ReadDataFromCurrentFile(TData& data)
{
	return ReadFileDataById(m_fileId, data);
}

bool CBIGFile::WriteDataToCurrentFile(const TData& data, bool immediateWriteOut)
{
	return WriteFileDataById(m_fileId, data, immediateWriteOut);
}

bool CBIGFile::HasPendingFileChanges() const
{
	bool hasChanges = false;
	const uint32 fileCount = m_fileDataVector.size();
	for (uint32 fileIndex = 0; fileIndex < fileCount; ++fileIndex)
	{
		if (!m_fileDataVector[fileIndex].empty())
		{
			hasChanges = true;
			break;
		}
	}
	return hasChanges;
}

bool CBIGFile::WriteOutPendingFileChanges()
{
	// Writing out a change to a .big file is not that straight forward.
	// To change a file, the big header and file header must be updated and
	// the whole .big file data needs to be written out to a new temporary file.

	if (m_hasPendingFileChanges)
	{
		bool newFileCreated = false;
		const std::wstring newFilename = utils::AppendRandomNumbers(m_filename, 8);
		const TDataVector& newFileDataVector = m_fileDataVector;

		try
		{
			if (m_ifstream.is_open() && m_ifstream.good())
			{
				std::ios::openmode mode = std::ios::out | std::ios::binary;
				std::ofstream ofstream;
				ofstream.open(newFilename.c_str(), mode);
				newFileCreated = ofstream.is_open();

				if (newFileCreated)
				{
					SBigHeader newBigHeader = m_bigHeader;
					TFileHeaders newFileHeaders = m_fileHeaders;
					SLastHeader newLastHeader = m_lastHeader;

					UpdateBigHeaderAndFileHeaders(newBigHeader, newFileHeaders, newFileDataVector);

					const uint32 bigHeaderSize = newBigHeader.SizeOnDisk();
					const uint32 fileHeadersSize = GetSizeOnDisk(newFileHeaders);
					const uint32 lastHeaderSize = newLastHeader.SizeOnDisk();

					TData newHeaderData;
					newHeaderData.resize(bigHeaderSize + fileHeadersSize + lastHeaderSize);

					bool ok = true;
					ok = ok && WriteBigHeaderToData(newHeaderData, newBigHeader);
					ok = ok && WriteFileHeadersToData(newHeaderData, newFileHeaders, bigHeaderSize);
					ok = ok && WriteLastHeaderToData(newHeaderData, newLastHeader, bigHeaderSize + fileHeadersSize);

					if (ok)
					{
						ok = ok && WriteDataToStream(newHeaderData, ofstream);
						const uint32 fileCount = newFileHeaders.size();
						const uint32 maxFileSize = GetMaxFileSize(m_fileHeaders);
						TData fileData;
						fileData.reserve(maxFileSize);

						for (uint32 fileIndex = 0; fileIndex < fileCount; ++fileIndex)
						{
							const SFileHeader& fileHeader = m_fileHeaders[fileIndex];
							const TData& newFileData = newFileDataVector[fileIndex];
							if (newFileData.empty())
							{
								// Transfer file data from original .big file to new .big file
								fileData.resize(fileHeader.size);
								ok = ok && ReadDataFromStream(fileData, m_ifstream, fileHeader.offset);
								ok = ok && WriteDataToStream(fileData, ofstream);
							}
							else
							{
								// Save new file data to new .big file
								ok = ok && WriteDataToStream(newFileData, ofstream);
							}
						}

						if (ok)
						{
							ofstream.close();
							CloseInputFileStream();

							// Replace the new written file with the original file
							if (::DeleteFileW(m_filename.c_str()) != FALSE)
							{
								if (::MoveFileW(newFilename.c_str(), m_filename.c_str()) != FALSE)
								{
									m_bigHeader = newBigHeader;
									m_fileHeaders.swap(newFileHeaders);
									m_lastHeader = newLastHeader;
									ClearPendingFileChanges();
									CreateFileHeaderIndices(m_fileHeaderIndices, m_fileHeaders);
									newFileCreated = false;
									m_hasPendingFileChanges = false;
								}
							}

							OpenInputFileStream();
						}
					}
				}
			}
		}
		catch (std::exception&)
		{
			assert(0);
		}

		if (newFileCreated)
		{
			::DeleteFileW(newFilename.c_str());
		}
	}

	return !m_hasPendingFileChanges;
}

void CBIGFile::ClearPendingFileChanges()
{
	// Clears the data at each index in the data vector and keeps its size
	const uint32 fileCount = m_fileDataVector.size();
	for (uint32 fileIndex = 0; fileIndex < fileCount; ++fileIndex)
	{
		m_fileDataVector[fileIndex].clear();
	}
}

bool CBIGFile::HasBigFileExtension(const wchar_t* fileName)
{
	// Note that the game also loads a .big file if it is called .big__
	// File extension is also case insensitive
	const size_t len = 3;
	size_t i = ::wcslen(fileName);

	if (i > len)
	{
		while (--i)
		{
			if (fileName[i] == L'.')
			{
				const wchar_t* ext = L"big";
				return ::_wcsnicmp(&fileName[i+1], ext, len) == 0;
			}
		}
	}
	return false;
}

const char* CBIGFile::GetSimplifiedCharset()
{
	return s_simplified_charset;
}

void CBIGFile::ApplySimplifiedCharset(std::string& str)
{
	const size_t len = str.size();
	for (size_t i = 0; i < len; ++i)
		str[i] = s_simplified_charset[static_cast<size_t>(str[i])];
}