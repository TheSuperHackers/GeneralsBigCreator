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


void CBIGFile::SBigFullHeader::Clear()
{
	bigHeader = SBigHeader();
	lastHeader = SBigLastHeader();
	utils::ClearMemory(fileHeaders);
}

void CBIGFile::SBigFullHeaderEx::Clear()
{
	bigHeader = SBigHeader();
	lastHeader = SBigLastHeader();
	utils::ClearMemory(fileHeaders);
}


CBIGFile::CBIGFile()
: m_fileId(0)
, m_flags(0)
, m_hasPendingFileChanges(false)
{
}

CBIGFile::~CBIGFile()
{
	CloseFile();
}

bool CBIGFile::OpenFile(const wchar_t* wcsBigFileName, TFlags flags)
{
	bool success = false;

	if (HasBigFileExtension(wcsBigFileName))
	{
		CloseFile();
		m_bigFileName = wcsBigFileName;
		m_flags = flags;
		OpenFileStream();

		if (m_fstream.is_open())
		{
			if (m_flags & eFlags_Read)
			{
				success = BuildFromFileStream();
			}
			else if (m_flags & eFlags_Write)
			{
				success = BuildDefault();
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

		m_workingHeader.Clear();
		m_physicalHeader.Clear();
		utils::ClearMemory(m_fileDataVector);
		utils::ClearMemory(m_fileHeaderIndices);
		utils::ClearMemory(m_bigFileName);
		CloseFileStream();
		m_fileId = 0u;
		m_flags = eFlags_None;
	}
}

bool CBIGFile::IsOpen() const
{
	return m_fstream.is_open();
}

uint32 CBIGFile::GetFileCount() const
{
	return m_fileHeaderIndices.size();
}

void CBIGFile::OpenFileStream()
{
	std::ios::openmode mode = std::ios::ate | std::ios::binary;
	mode |= (m_flags & eFlags_Read) ? std::ios::in : 0;
	mode |= (m_flags & eFlags_Write) ? std::ios::out : 0;
	m_fstream.open(m_bigFileName.c_str(), mode);
}

void CBIGFile::CloseFileStream()
{
	m_fstream.close();
}

bool CBIGFile::BuildFromFileStream()
{
	bool success = false;
	TData headerData;
	headerData.resize(m_workingHeader.bigHeader.SizeOnDisk());

	if (ReadDataFromStream(headerData, m_fstream))
	{
		if (ReadBigHeaderFromData(m_workingHeader.bigHeader, headerData))
		{
#ifndef _RELEASE
			m_fstream.seekg(0, std::ios::end);
			assert(m_workingHeader.bigHeader.HasExpectedFileSize(m_fstream.tellg()));
#endif
			const uint32 fileHeadersOffset = m_workingHeader.bigHeader.SizeOnDisk();
			const uint32 lastHeaderOffset = m_workingHeader.bigHeader.SizeOnDisk() + m_workingHeader.bigHeader.FileHeaderSize();
			headerData.resize(m_workingHeader.bigHeader.headerSize);

			if (ReadDataFromStream(headerData, m_fstream))
			{
				if (ReadFileHeadersFromData(m_workingHeader.fileHeaders, m_workingHeader.bigHeader, headerData, fileHeadersOffset, m_flags))
				{
					if (ReadLastHeaderFromData(m_workingHeader.lastHeader, headerData, lastHeaderOffset))
					{
						m_fileDataVector.resize(m_workingHeader.fileHeaders.size());
						BuildFileHeaderIndices(m_fileHeaderIndices, m_workingHeader.fileHeaders);
						m_physicalHeader.Copy(m_workingHeader);
						success = true;
					}
				}
			}
		}
	}
	return success;
}

bool CBIGFile::BuildDefault()
{
	// Subsequent uses need to be read enabled.
	m_flags |= eFlags_Read;

	// Create empty big header.
	m_workingHeader.bigHeader.bigf = SBigHeader::GetSignature();
	BuildBigHeaderAndFileHeaders(m_workingHeader.bigHeader, m_workingHeader.fileHeaders, m_fileDataVector);

	// Set desire to write out changes.
	m_hasPendingFileChanges = true;

	return true;
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

bool CBIGFile::ReadFileHeadersFromData(TBigFileHeadersEx& fileHeaders, const SBigHeader& bigHeader, const TData& data, uint32 offset, TFlags flags)
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
			fileHeaders.push_back(SBigFileHeaderEx());
			SBigFileHeaderEx& newFileHeader = fileHeaders.back();

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

			newFileHeader.physical = true;
		}
		
		assert(fileHeaders.size() == fileCount);

		for (uint32 fileIndex = 0; fileIndex < fileCount; ++fileIndex)
		{
			// Check that file headers are healthy
			assert(!fileHeaders[fileIndex].name.empty());
			assert((fileHeaders[fileIndex].offset + fileHeaders[fileIndex].size) <= bigHeader.bigFileSize);
		}

		return true;
	}
	return false;
}

bool CBIGFile::WriteFileHeadersToData(TData& data, const TBigFileHeadersEx& fileHeaders, uint32 offset)
{
	const uint32 fileHeaderSize = GetSizeOnDisk(fileHeaders);
	const uint32 fileCount = fileHeaders.size();

	assert(data.size() >= fileHeaderSize + offset);

	if (data.size() >= fileHeaderSize + offset)
	{
		uint32 dataIndex = offset;
		
		for (uint32 fileIndex = 0; fileIndex < fileCount; ++fileIndex)
		{
			const SBigFileHeaderEx& fileHeader = fileHeaders[fileIndex];

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

bool CBIGFile::ReadLastHeaderFromData(SBigLastHeader& lastHeader, const TData& data, uint32 offset)
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

bool CBIGFile::WriteLastHeaderToData(TData& data, const SBigLastHeader& lastHeader, uint32 offset)
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

void CBIGFile::BuildFileHeaderIndices(TIntegers& fileHeaderIndices, const TBigFileHeadersEx& fileHeaders)
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

void CBIGFile::BuildBigHeaderAndFileHeaders(SBigHeader& bigHeader, TBigFileHeadersEx& fileHeaders, const TDataPtrVector& fileDataVector)
{
	assert(fileHeaders.size() == fileDataVector.size());

	bigHeader.bigFileSize = 0;
	bigHeader.bigFileSize += bigHeader.SizeOnDisk();
	bigHeader.bigFileSize += GetSizeOnDisk(fileHeaders);
	bigHeader.bigFileSize += SBigLastHeader::SizeOnDisk();
	bigHeader.headerSize = bigHeader.bigFileSize;
	bigHeader.fileCount = fileHeaders.size();

	for (uint32 fileIndex = 0; fileIndex < bigHeader.fileCount; ++fileIndex)
	{
		SBigFileHeaderEx& fileHeader = fileHeaders[fileIndex];
		const TDataPtr& fileDataPtr = fileDataVector[fileIndex];

		fileHeader.offset = bigHeader.bigFileSize;
		if (fileDataPtr.get())
			fileHeader.size = fileDataPtr->data.size();
		bigHeader.bigFileSize += fileHeader.size;
	}
}

uint32 CBIGFile::GetSizeOnDisk(const TBigFileHeadersEx& fileHeaders)
{
	uint32 sizeOnDisk = 0;
	const uint32 fileCount = fileHeaders.size();
	for (uint32 fileIndex = 0; fileIndex < fileCount; ++fileIndex)
	{
		sizeOnDisk += fileHeaders[fileIndex].SizeOnDisk();
	}
	return sizeOnDisk;
}

uint32 CBIGFile::GetMaxFileSize(const TBigFileHeadersEx& fileHeaders)
{
	uint32 maxFileSize = 0;
	const uint32 fileCount = fileHeaders.size();
	for (uint32 fileIndex = 0; fileIndex < fileCount; ++fileIndex)
	{
		maxFileSize = std::max(maxFileSize, fileHeaders[fileIndex].size);
	}
	return maxFileSize;
}

CBIGFile::SBigFileHeaderEx* CBIGFile::GetFileHeader(uint32 id)
{
	if (id < m_fileHeaderIndices.size())
	{
		const uint32 index = m_fileHeaderIndices[id];
		return &m_workingHeader.fileHeaders[index];
	}
	return NULL;
}

const CBIGFile::SBigFileHeaderEx* CBIGFile::GetFileHeader(uint32 id) const
{
	if (id < m_fileHeaderIndices.size())
	{
		const uint32 index = m_fileHeaderIndices[id];
		return &m_workingHeader.fileHeaders[index];
	}
	return NULL;
}

bool CBIGFile::SetFileNameById(uint32 id, const char* szName)
{
	if (SBigFileHeaderEx* pFileHeader = GetFileHeader(id))
	{
		pFileHeader->name = szName;
		pFileHeader->simplifiedName = szName;

		if (m_flags & eFlags_UseSimplifiedName)
		{
			ApplySimplifiedCharset(pFileHeader->simplifiedName);
		}
		return true;
	}
	return false;
}

const char* CBIGFile::GetFileNameById(uint32 id) const
{
	if (const SBigFileHeaderEx* pFileHeader = GetFileHeader(id))
	{
		return pFileHeader->simplifiedName.c_str();
	}
	return NULL;
}

void CBIGFile::SetCurrentFileId(uint32 id)
{
	m_fileId = (id < GetFileCount()) ? id : GetFileCount();
}

uint32 CBIGFile::GetCurrentFileId() const
{
	return m_fileId;
}

bool CBIGFile::SetCurrentFileName(const char* szName)
{
	return SetFileNameById(m_fileId, szName);
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
	m_fileId = (m_fileId < GetFileCount()) ? ++m_fileId : m_fileId;
	return GetFileNameById(m_fileId);
}

bool CBIGFile::AddNewFile(const char* szName, const TData& data, bool immediateWriteOut)
{
	m_fileId = (m_fileId < GetFileCount()) ? ++m_fileId : m_fileId;
	return AddNewFile(m_fileId, szName, data, immediateWriteOut);
}

bool CBIGFile::AddNewFile(uint32 id, const char* szName, const TData& data, bool immediateWriteOut)
{
	bool success = false;

	SBigFileHeaderEx newFileHeader;
	newFileHeader.name = szName;
	newFileHeader.simplifiedName = szName;

	if (m_flags & eFlags_UseSimplifiedName)
	{
		ApplySimplifiedCharset(newFileHeader.simplifiedName);
	}

	if (id >= GetFileCount())
	{
		const uint32 fileIndex = static_cast<uint32>(m_workingHeader.fileHeaders.size());

		// Add new file at end.
		m_fileId = GetFileCount();
		m_fileHeaderIndices.push_back(fileIndex);
		m_workingHeader.fileHeaders.push_back(newFileHeader);
		m_fileDataVector.push_back(new SDataRef(data));
	}
	else
	{
		const uint32 fileIndex = m_fileHeaderIndices[m_fileId];

		// Add new file at begin or middle.
		m_workingHeader.fileHeaders.insert(m_workingHeader.fileHeaders.begin() + fileIndex, newFileHeader);
		m_fileDataVector.insert(m_fileDataVector.begin() + fileIndex, new SDataRef(data));

		// Rebuild file header indices.
		BuildFileHeaderIndices(m_fileHeaderIndices, m_workingHeader.fileHeaders);
	}

	// Rebuild headers with new data added.
	BuildBigHeaderAndFileHeaders(m_workingHeader.bigHeader, m_workingHeader.fileHeaders, m_fileDataVector);

	// Write out pending changes if necessary.
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

	return success;
}

bool CBIGFile::ReadFileDataById(uint32 id, TData& data)
{
	bool success = false;
	if (id < m_fileHeaderIndices.size())
	{
		const uint32 index = m_fileHeaderIndices[id];
		const TDataPtr& internalDataPtr = m_fileDataVector[index];

		if (!internalDataPtr->data.empty())
		{
			// Get data that is not yet written out to the .big file
			data = internalDataPtr->data;
			success = true;
		}
		else
		{
			// Get data that is in the .big file on disk
			const SBigFileHeaderEx& fileHeader = m_workingHeader.fileHeaders[index];
			data.resize(fileHeader.size);
			success = ReadDataFromStream(data, m_fstream, fileHeader.offset);
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
		m_fileDataVector[index] = new SDataRef(data);

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
		if (!m_fileDataVector[fileIndex]->data.empty())
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
		const std::wstring newFilename = utils::AppendRandomNumbers(m_bigFileName, 8);

		try
		{
			if (m_fstream.is_open() && m_fstream.good())
			{
				std::ios::openmode mode = std::ios::out | std::ios::binary;
				std::ofstream ofstream;
				ofstream.open(newFilename.c_str(), mode);
				newFileCreated = ofstream.is_open();

				if (newFileCreated)
				{
					const uint32 bigHeaderSize = m_workingHeader.bigHeader.SizeOnDisk();
					const uint32 fileHeadersSize = GetSizeOnDisk(m_workingHeader.fileHeaders);
					const uint32 lastHeaderSize = m_workingHeader.lastHeader.SizeOnDisk();

					TData newHeaderData;
					newHeaderData.resize(bigHeaderSize + fileHeadersSize + lastHeaderSize);

					bool ok = true;
					ok = ok && WriteBigHeaderToData(newHeaderData, m_workingHeader.bigHeader);
					ok = ok && WriteFileHeadersToData(newHeaderData, m_workingHeader.fileHeaders, bigHeaderSize);
					ok = ok && WriteLastHeaderToData(newHeaderData, m_workingHeader.lastHeader, bigHeaderSize + fileHeadersSize);

					if (ok)
					{
						ok = ok && WriteDataToStream(newHeaderData, ofstream);
						const uint32 workingFileCount = static_cast<uint32>(m_workingHeader.fileHeaders.size());
						const uint32 physicalFileCount = static_cast<uint32>(m_physicalHeader.fileHeaders.size());
						const uint32 maxFileSize = GetMaxFileSize(m_workingHeader.fileHeaders);
						TData fileData;
						fileData.reserve(maxFileSize);
						uint32 workingFileIndex = 0;
						uint32 physicalFileIndex = 0;

						for (; workingFileIndex < workingFileCount; ++workingFileIndex)
						{
							const TDataPtr& newFileDataPtr = m_fileDataVector[workingFileIndex];

							if (!newFileDataPtr.get())
							{
								// Transfer file data from original .big file to new .big file
								assert(physicalFileIndex < physicalFileCount);
								const SBigFileHeader& fileHeader = m_physicalHeader.fileHeaders[physicalFileIndex];
								fileData.resize(fileHeader.size);
								ok = ok && ReadDataFromStream(fileData, m_fstream, fileHeader.offset);
								ok = ok && WriteDataToStream(fileData, ofstream);
								
							}
							else
							{
								// Save new file data to new .big file
								ok = ok && WriteDataToStream(newFileDataPtr->data, ofstream);
							}

							const SBigFileHeaderEx& fileHeader = m_workingHeader.fileHeaders[workingFileIndex];
							if (fileHeader.physical)
							{
								++physicalFileIndex;
							}
						}

						if (ok)
						{
							ofstream.close();
							CloseFileStream();

							// Replace the new written file with the original file
							if (::DeleteFileW(m_bigFileName.c_str()) != FALSE)
							{
								if (::MoveFileW(newFilename.c_str(), m_bigFileName.c_str()) != FALSE)
								{
									for (uint32 fileIndex = 0; fileIndex < workingFileCount; ++fileIndex)
									{
										m_workingHeader.fileHeaders[fileIndex].physical = true;
									}
									m_physicalHeader.Copy(m_workingHeader);
									ClearPendingFileChanges();
									BuildFileHeaderIndices(m_fileHeaderIndices, m_workingHeader.fileHeaders);

									newFileCreated = false;
									m_hasPendingFileChanges = false;
								}
							}

							OpenFileStream();
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
		m_fileDataVector[fileIndex].reset();
	}
}

bool CBIGFile::HasBigFileExtension(const wchar_t* wcsBigFileName)
{
	// Note that the game also loads a .big file if it is called .big__
	// File extension is also case insensitive
	const size_t len = 3;
	size_t i = ::wcslen(wcsBigFileName);

	if (i > len)
	{
		while (--i)
		{
			if (wcsBigFileName[i] == L'.')
			{
				const wchar_t* ext = L"big";
				return ::_wcsnicmp(&wcsBigFileName[i+1], ext, len) == 0;
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
