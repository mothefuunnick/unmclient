/*

	Copyright 2010 Trevor Hogan

	Licensed under the Apache License, Version 2.0 (the "License");
	you may not use this file except in compliance with the License.
	You may obtain a copy of the License at

		http://www.apache.org/licenses/LICENSE-2.0

	Unless required by applicable law or agreed to in writing, software
	distributed under the License is distributed on an "AS IS" BASIS,
	WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
	See the License for the specific language governing permissions and
	limitations under the License.

*/

#ifndef PACKED_H
#define PACKED_H

//
// CPacked
//

class CCRC32;

class CPacked
{
public:
	CCRC32 *m_CRC;

protected:
	bool m_Valid;
    std::string m_Compressed;
    std::string m_Decompressed;
	uint32_t m_HeaderSize;
	uint32_t m_CompressedSize;
	uint32_t m_HeaderVersion;
	uint32_t m_DecompressedSize;
	uint32_t m_NumBlocks;
	uint32_t m_War3Identifier;
	uint32_t m_War3Version;
	uint16_t m_BuildNumber;
	uint16_t m_Flags;
	uint32_t m_ReplayLength;

public:
	CPacked( );
	virtual ~CPacked( );
	virtual bool GetValid( )
	{
		return m_Valid;
	}
    virtual void Load( std::string fileName, bool allBlocks );
    virtual bool Save( bool TFT, std::string fileName );
    virtual bool Extract( std::string inFileName, std::string outFileName );
    virtual bool Pack( bool TFT, std::string inFileName, std::string outFileName );
	virtual void Decompress( bool allBlocks );
	virtual void Compress( bool TFT );
};

#endif
