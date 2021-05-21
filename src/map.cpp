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

#include "unm.h"
#include "util.h"
#include "unmcrc32.h"
#include "unmsha1.h"
#include "config.h"
#include "map.h"

#define __STORMLIB_SELF__
#include "src/stormlib/StormLib.h"

#define ROTL(x,n) ((x)<<(n))|((x)>>(32-(n)))	// this won't work with signed types
#define ROTR(x,n) ((x)>>(n))|((x)<<(32-(n)))	// this won't work with signed types

using namespace std;

//
// CMap
//

CMap::CMap( CUNM *nUNM )
{
	CONSOLE_Print( "[MAP] using hardcoded Emerald Gardens map data for Warcraft 3 version 1.26 & 1.27" );
    m_UNM = nUNM;
    m_MapName = "unknown";
    m_MapAuthor = "N/A";
    m_MapDescription = "N/A";
    m_MapPlayers = "N/A";
    m_MapTGA.clear( );
	m_Valid = true;
    m_MeleeMap = true;
    m_MapLocalPath = string( );
    m_MapFileSize = -1;
	m_MapPath = "Maps\\FrozenThrone\\(12)EmeraldGardens.w3x";
	m_MapSize = UTIL_ExtractNumbers( "174 221 4 0", 4 );
	m_MapInfo = UTIL_ExtractNumbers( "251 57 68 98", 4 );
	m_MapCRC = UTIL_ExtractNumbers( "108 250 204 59", 4 );
	m_MapSHA1 = UTIL_ExtractNumbers( "35 81 104 182 223 63 204 215 1 17 87 234 220 66 3 185 82 99 6 13", 20 );

    if( m_UNM->m_LANWar3Version == 23 )
	{
		m_MapCRC = UTIL_ExtractNumbers( "112 185 65 97", 4 );
		m_MapSHA1 = UTIL_ExtractNumbers( "187 28 143 4 97 223 210 52 218 28 95 52 217 203 121 202 24 120 59 213", 20 );
	}

	m_MapSpeed = MAPSPEED_FAST;
	m_MapVisibility = MAPVIS_DEFAULT;
	m_MapObservers = MAPOBS_NONE;
	m_MapFlags = MAPFLAG_TEAMSTOGETHER | MAPFLAG_FIXEDTEAMS;
	m_MapGameType = 9;
	m_MapWidth = UTIL_ExtractNumbers( "172 0", 2 );
	m_MapHeight = UTIL_ExtractNumbers( "172 0", 2 );
	m_MapLoadInGame = false;
	m_MapNumPlayers = 12;
	m_MapNumTeams = 12;
	m_MapOnlyAutoWarnIfMoreThanXPlayers = 0;
    m_AutoWarnMarks = 0;
	m_LogAll = false;
    m_Slots.push_back( CGameSlot( 0, 255, SLOTSTATUS_OPEN, 0, 0, 0, m_UNM->m_PlayersCanChangeRace ? 96 : SLOTRACE_RANDOM ) );
    m_Slots.push_back( CGameSlot( 0, 255, SLOTSTATUS_OPEN, 0, 1, 1, m_UNM->m_PlayersCanChangeRace ? 96 : SLOTRACE_RANDOM ) );
    m_Slots.push_back( CGameSlot( 0, 255, SLOTSTATUS_OPEN, 0, 2, 2, m_UNM->m_PlayersCanChangeRace ? 96 : SLOTRACE_RANDOM ) );
    m_Slots.push_back( CGameSlot( 0, 255, SLOTSTATUS_OPEN, 0, 3, 3, m_UNM->m_PlayersCanChangeRace ? 96 : SLOTRACE_RANDOM ) );
    m_Slots.push_back( CGameSlot( 0, 255, SLOTSTATUS_OPEN, 0, 4, 4, m_UNM->m_PlayersCanChangeRace ? 96 : SLOTRACE_RANDOM ) );
    m_Slots.push_back( CGameSlot( 0, 255, SLOTSTATUS_OPEN, 0, 5, 5, m_UNM->m_PlayersCanChangeRace ? 96 : SLOTRACE_RANDOM ) );
    m_Slots.push_back( CGameSlot( 0, 255, SLOTSTATUS_OPEN, 0, 6, 6, m_UNM->m_PlayersCanChangeRace ? 96 : SLOTRACE_RANDOM ) );
    m_Slots.push_back( CGameSlot( 0, 255, SLOTSTATUS_OPEN, 0, 7, 7, m_UNM->m_PlayersCanChangeRace ? 96 : SLOTRACE_RANDOM ) );
    m_Slots.push_back( CGameSlot( 0, 255, SLOTSTATUS_OPEN, 0, 8, 8, m_UNM->m_PlayersCanChangeRace ? 96 : SLOTRACE_RANDOM ) );
    m_Slots.push_back( CGameSlot( 0, 255, SLOTSTATUS_OPEN, 0, 9, 9, m_UNM->m_PlayersCanChangeRace ? 96 : SLOTRACE_RANDOM ) );
    m_Slots.push_back( CGameSlot( 0, 255, SLOTSTATUS_OPEN, 0, 10, 10, m_UNM->m_PlayersCanChangeRace ? 96 : SLOTRACE_RANDOM ) );
    m_Slots.push_back( CGameSlot( 0, 255, SLOTSTATUS_OPEN, 0, 11, 11, m_UNM->m_PlayersCanChangeRace ? 96 : SLOTRACE_RANDOM ) );
}

CMap::CMap( CUNM *nUNM, CConfig *CFG, string nCFGFile, string nFilePath )
{
    m_UNM = nUNM;
	m_LogAll = false;
    Load( CFG, nCFGFile, nFilePath );
}

CMap::~CMap( )
{

}

BYTEARRAY CMap::GetMapGameFlags( )
{
	/*

	Speed: (mask 0x00000003) cannot be combined
		0x00000000 - Slow game speed
		0x00000001 - Normal game speed
		0x00000002 - Fast game speed
	Visibility: (mask 0x00000F00) cannot be combined
		0x00000100 - Hide terrain
		0x00000200 - Map explored
		0x00000400 - Always visible (no fog of war)
		0x00000800 - Default
	Observers/Referees: (mask 0x40003000) cannot be combined
		0x00000000 - No Observers
		0x00002000 - Observers on Defeat
		0x00003000 - Additional players as observer allowed
		0x40000000 - Referees
	Teams/Units/Hero/Race: (mask 0x07064000) can be combined
		0x00004000 - Teams Together (team members are placed at neighbored starting locations)
		0x00060000 - Fixed teams
		0x01000000 - Unit share
		0x02000000 - Random hero
		0x04000000 - Random races

	*/

	uint32_t GameFlags = 0;

	// speed

	if( m_MapSpeed == MAPSPEED_SLOW )
		GameFlags = 0x00000000;
	else if( m_MapSpeed == MAPSPEED_NORMAL )
		GameFlags = 0x00000001;
	else
		GameFlags = 0x00000002;

	// visibility

	if( m_MapVisibility == MAPVIS_HIDETERRAIN )
		GameFlags |= 0x00000100;
	else if( m_MapVisibility == MAPVIS_EXPLORED )
		GameFlags |= 0x00000200;
	else if( m_MapVisibility == MAPVIS_ALWAYSVISIBLE )
		GameFlags |= 0x00000400;
	else
		GameFlags |= 0x00000800;

	// observers

	if( m_MapObservers == MAPOBS_ONDEFEAT )
		GameFlags |= 0x00002000;
	else if( m_MapObservers == MAPOBS_ALLOWED )
		GameFlags |= 0x00003000;
	else if( m_MapObservers == MAPOBS_REFEREES )
		GameFlags |= 0x40000000;

	// teams/units/hero/race

	if( m_MapFlags & MAPFLAG_TEAMSTOGETHER )
		GameFlags |= 0x00004000;
	if( m_MapFlags & MAPFLAG_FIXEDTEAMS )
		GameFlags |= 0x00060000;
	if( m_MapFlags & MAPFLAG_UNITSHARE )
		GameFlags |= 0x01000000;
	if( m_MapFlags & MAPFLAG_RANDOMHERO )
		GameFlags |= 0x02000000;
	if( m_MapFlags & MAPFLAG_RANDOMRACES )
		GameFlags |= 0x04000000;

	return UTIL_CreateByteArray( GameFlags, false );
}

void CMap::Load( CConfig *CFG, string nCFGFile, string nFilePath )
{
    m_MapName = "unknown";
    m_MapAuthor = "N/A";
    m_MapDescription = "N/A";
    m_MapPlayers = "N/A";
    m_MapTGA.clear( );
	m_Valid = true;
    m_MeleeMap = false;
	m_CFGFile = nCFGFile;
	bool IsCFGFile = false;

	if( nCFGFile.size( ) <= 4 || nCFGFile.substr( nCFGFile.size( ) - 4 ) == ".cfg" )
		IsCFGFile = true;

	// load the map data

    m_MapData.clear( );
    m_MapFileSize = -1;
	m_MapLocalPath = CFG->GetString( "map_localpath", string( ) );
    string MapFullLocalPath = m_MapLocalPath;

    if( !MapFullLocalPath.empty( ) )
    {
        if( !nFilePath.empty( ) )
        {
            if( UTIL_FileExists( nFilePath ) )
                m_MapFileSize = UTIL_FileSize( nFilePath, -1 );

            m_MapData = UTIL_FileRead( nFilePath );
        }
        else
        {
            if( !UTIL_FileExists( m_UNM->m_MapPath + MapFullLocalPath ) )
                MapFullLocalPath = m_UNM->FindMap( MapFullLocalPath, MapFullLocalPath );

            if( UTIL_FileExists( m_UNM->m_MapPath + MapFullLocalPath ) )
                m_MapFileSize = UTIL_FileSize( m_UNM->m_MapPath + MapFullLocalPath, -1 );

            m_MapData = UTIL_FileRead( m_UNM->m_MapPath + MapFullLocalPath );
        }
    }

	// load the map MPQ

    string MapMPQFileName = m_UNM->m_MapPath + MapFullLocalPath;

    if( !nFilePath.empty( ) )
        MapMPQFileName = nFilePath;

    HANDLE MapMPQ;
    bool MapMPQReady = false;
    std::wstring WMapMPQFileName = std::wstring_convert<std::codecvt_utf8<wchar_t>>().from_bytes(MapMPQFileName);

    if( SFileOpenArchive( WMapMPQFileName.c_str( ), 0, MPQ_OPEN_FORCE_MPQ_V1, &MapMPQ ) )
    {
		CONSOLE_Print( "[MAP] loading MPQ file [" + MapMPQFileName + "]" );
		MapMPQReady = true;
	}
	else
		CONSOLE_Print( "[MAP] warning - unable to load MPQ file [" + MapMPQFileName + "]" );

	// try to calculate map_size, map_info, map_crc, map_sha1

	BYTEARRAY MapSize;
	BYTEARRAY MapInfo;
	BYTEARRAY MapCRC;
	BYTEARRAY MapSHA1;
	bool calculated = false;

	if( !m_MapData.empty( ) )
	{
        m_UNM->m_SHA->Reset( );

		// calculate map_size

		MapSize = UTIL_CreateByteArray( (uint32_t)m_MapData.size( ), false );
		CONSOLE_Print( "[MAP] calculated map_size = " + UTIL_ToString( MapSize[0] ) + " " + UTIL_ToString( MapSize[1] ) + " " + UTIL_ToString( MapSize[2] ) + " " + UTIL_ToString( MapSize[3] ) );

		// calculate map_info (this is actually the CRC)

        MapInfo = UTIL_CreateByteArray( (uint32_t)m_UNM->m_CRC->FullCRC( (unsigned char *)m_MapData.c_str( ), m_MapData.size( ) ), false );
		CONSOLE_Print( "[MAP] calculated map_info = " + UTIL_ToString( MapInfo[0] ) + " " + UTIL_ToString( MapInfo[1] ) + " " + UTIL_ToString( MapInfo[2] ) + " " + UTIL_ToString( MapInfo[3] ) );

		// calculate map_crc (this is not the CRC) and map_sha1
		// a big thank you to Strilanc for figuring the map_crc algorithm out

        string CommonJ = UTIL_FileRead( m_UNM->m_MapCFGPath + "common.j" );

		if( CommonJ.empty( ) )
            CONSOLE_Print( "[MAP] unable to calculate map_crc/sha1 - unable to read file [" + m_UNM->m_MapCFGPath + "common.j]" );
		else
		{
            string BlizzardJ = UTIL_FileRead( m_UNM->m_MapCFGPath + "blizzard.j" );

			if( BlizzardJ.empty( ) )
                CONSOLE_Print( "[MAP] unable to calculate map_crc/sha1 - unable to read file [" + m_UNM->m_MapCFGPath + "blizzard.j]" );
			else
			{
				uint32_t Val = 0;

				// update: it's possible for maps to include their own copies of common.j and/or blizzard.j
				// this code now overrides the default copies if required

				bool OverrodeCommonJ = false;
				bool OverrodeBlizzardJ = false;

				if( MapMPQReady )
				{
					HANDLE SubFile;

					// override common.j

					if( SFileOpenFileEx( MapMPQ, "Scripts\\common.j", 0, &SubFile ) )
					{
						uint32_t FileLength = SFileGetFileSize( SubFile, nullptr );

						if( FileLength > 0 && FileLength != 0xFFFFFFFF )
						{
							char *SubFileData = new char[FileLength];
							DWORD BytesRead = 0;
							LPOVERLAPPED lpOverlapped = 0;

							if( SFileReadFile( SubFile, SubFileData, FileLength, &BytesRead, lpOverlapped ) )
							{
								CONSOLE_Print( "[MAP] overriding default common.j with map copy while calculating map_crc/sha1" );
								OverrodeCommonJ = true;
								Val = Val ^ XORRotateLeft( (unsigned char *)SubFileData, BytesRead );
                                m_UNM->m_SHA->Update( (unsigned char *)SubFileData, BytesRead );
							}

							delete[] SubFileData;
						}

						SFileCloseFile( SubFile );
					}
				}

				if( !OverrodeCommonJ )
				{
					Val = Val ^ XORRotateLeft( (unsigned char *)CommonJ.c_str( ), CommonJ.size( ) );
                    m_UNM->m_SHA->Update( (unsigned char *)CommonJ.c_str( ), CommonJ.size( ) );
				}

				if( MapMPQReady )
				{
					HANDLE SubFile;

					// override blizzard.j

					if( SFileOpenFileEx( MapMPQ, "Scripts\\blizzard.j", 0, &SubFile ) )
                    {
						uint32_t FileLength = SFileGetFileSize( SubFile, nullptr );

						if( FileLength > 0 && FileLength != 0xFFFFFFFF )
                        {
							char *SubFileData = new char[FileLength];
							DWORD BytesRead = 0;
							LPOVERLAPPED lpOverlapped = 0;

							if( SFileReadFile( SubFile, SubFileData, FileLength, &BytesRead, lpOverlapped ) )
                            {
								CONSOLE_Print( "[MAP] overriding default blizzard.j with map copy while calculating map_crc/sha1" );
								OverrodeBlizzardJ = true;
								Val = Val ^ XORRotateLeft( (unsigned char *)SubFileData, BytesRead );
                                m_UNM->m_SHA->Update( (unsigned char *)SubFileData, BytesRead );
							}

							delete[] SubFileData;
						}

						SFileCloseFile( SubFile );
					}
				}

				if( !OverrodeBlizzardJ )
				{
					Val = Val ^ XORRotateLeft( (unsigned char *)BlizzardJ.c_str( ), BlizzardJ.size( ) );
                    m_UNM->m_SHA->Update( (unsigned char *)BlizzardJ.c_str( ), BlizzardJ.size( ) );
				}

				Val = ROTL( Val, 3 );
				Val = ROTL( Val ^ 0x03F1379E, 3 );
                m_UNM->m_SHA->Update( ( unsigned char * )"\x9E\x37\xF1\x03", 4 );

				if( MapMPQReady )
				{
					vector<string> FileList;
					FileList.push_back( "war3map.j" );
					FileList.push_back( "scripts\\war3map.j" );
					FileList.push_back( "war3map.w3e" );
					FileList.push_back( "war3map.wpm" );
					FileList.push_back( "war3map.doo" );
					FileList.push_back( "war3map.w3u" );
					FileList.push_back( "war3map.w3b" );
					FileList.push_back( "war3map.w3d" );
					FileList.push_back( "war3map.w3a" );
					FileList.push_back( "war3map.w3q" );
					bool FoundScript = false;

					for( vector<string> ::iterator i = FileList.begin( ); i != FileList.end( ); i++ )
					{
						// don't use scripts\war3map.j if we've already used war3map.j (yes, some maps have both but only war3map.j is used)

						if( FoundScript && *i == "scripts\\war3map.j" )
							continue;

						HANDLE SubFile;

						if( SFileOpenFileEx( MapMPQ, ( *i ).c_str( ), 0, &SubFile ) )
						{
							uint32_t FileLength = SFileGetFileSize( SubFile, nullptr );

							if( FileLength > 0 && FileLength != 0xFFFFFFFF )
							{
								char *SubFileData = new char[FileLength];
								DWORD BytesRead = 0;
								LPOVERLAPPED lpOverlapped = 0;

								if( SFileReadFile( SubFile, SubFileData, FileLength, &BytesRead, lpOverlapped ) )
								{
									if( *i == "war3map.j" || *i == "scripts\\war3map.j" )
										FoundScript = true;

									Val = ROTL( Val ^ XORRotateLeft( (unsigned char *)SubFileData, BytesRead ), 3 );
                                    m_UNM->m_SHA->Update( (unsigned char *)SubFileData, BytesRead );
								}

								delete[] SubFileData;
							}

							SFileCloseFile( SubFile );
						}
					}

					if( !FoundScript )
						CONSOLE_Print( "[MAP] couldn't find war3map.j or scripts\\war3map.j in MPQ file, calculated map_crc/sha1 is probably wrong" );

					MapCRC = UTIL_CreateByteArray( Val, false );
					CONSOLE_Print( "[MAP] calculated map_crc = " + UTIL_ToString( MapCRC[0] ) + " " + UTIL_ToString( MapCRC[1] ) + " " + UTIL_ToString( MapCRC[2] ) + " " + UTIL_ToString( MapCRC[3] ) );
                    m_UNM->m_SHA->Final( );
					unsigned char SHA1[20];
					memset( SHA1, 0, sizeof( unsigned char ) * 20 );
                    m_UNM->m_SHA->GetHash( SHA1 );
					MapSHA1 = UTIL_CreateByteArray( SHA1, 20 );
					CONSOLE_Print( "[MAP] calculated map_sha1 = " + UTIL_ByteArrayToDecString( MapSHA1 ) );
					calculated = true;
				}
				else
					CONSOLE_Print( "[MAP] unable to calculate map_crc/sha1 - map MPQ file not loaded" );
			}
		}
	}
	else
		CONSOLE_Print( "[MAP] no map data available, using config file for map_size, map_info, map_crc, map_sha1" );

	// try to calculate map_width, map_height, map_slot<x>, map_numplayers, map_numteams

	BYTEARRAY MapWidth;
	BYTEARRAY MapHeight;
	uint32_t MapNumPlayers = 0;
	uint32_t MapNumTeams = 0;
	uint32_t MapFilterType = MAPFILTER_TYPE_SCENARIO;
	uint32_t MapGameType = 0;
	uint32_t FreeSlots = 0;
	vector<CGameSlot> Slots;

	if( !m_MapData.empty( ) )
	{
        bool MapNameTRIGSTR = false;
        bool MapAuthorTRIGSTR = false;
        bool MapDescriptionTRIGSTR = false;
        bool MapPlayersTRIGSTR = false;
        string MapName;
        string MapAuthor;
        string MapDescription;
        string MapPlayers;

		if( calculated && MapSize[0] == 106 && MapSize[1] == 204 && MapSize[2] == 113 && MapSize[3] == 0 && MapInfo[0] == 54 && MapInfo[1] == 0 && MapInfo[2] == 170 && MapInfo[3] == 185 && MapCRC[0] == 92 && MapCRC[1] == 10 && MapCRC[2] == 227 && MapCRC[3] == 204 && UTIL_ByteArrayToDecString( MapSHA1 ) == "247 25 5 205 31 119 161 197 250 66 155 171 112 27 98 223 232 98 73 102" )
		{
			for( uint32_t i = 0; i < 9; i++ )
			{
				if( i < 8 )
				{
                    CGameSlot Slot( 0, 255, SLOTSTATUS_OPEN, 0, 0, static_cast<unsigned char>(i), m_UNM->m_PlayersCanChangeRace ? 96 : SLOTRACE_RANDOM );
					Slots.push_back( Slot );
				}
				else
				{
                    CGameSlot Slot( 0, 255, SLOTSTATUS_OCCUPIED, 1, 1, 9, m_UNM->m_PlayersCanChangeRace ? 66 : SLOTRACE_ORC );
					Slots.push_back( Slot );
				}
			}

			CONSOLE_Print( "[MAP] sao_project_0.05(engfix).w3x map detected, overriding calculated slots values..." );
			MapWidth = UTIL_CreateByteArray( (uint16_t)474, false );
			CONSOLE_Print( "[MAP] calculated map_width = " + UTIL_ToString( MapWidth[0] ) + " " + UTIL_ToString( MapWidth[1] ) );
			MapHeight = UTIL_CreateByteArray( (uint16_t)472, false );
			CONSOLE_Print( "[MAP] calculated map_height = " + UTIL_ToString( MapHeight[0] ) + " " + UTIL_ToString( MapHeight[1] ) );
			MapNumPlayers = 9;
			CONSOLE_Print( "[MAP] calculated map_numplayers = " + UTIL_ToString( MapNumPlayers ) );
			MapNumTeams = 2;
			CONSOLE_Print( "[MAP] calculated map_numteams = " + UTIL_ToString( MapNumTeams ) );
			FreeSlots = 8;
			CONSOLE_Print( "[MAP] found " + UTIL_ToString( FreeSlots ) + "/" + UTIL_ToString( Slots.size( ) ) + " slots" );
			MapGameType = 1;
		}
		else if( MapMPQReady )
		{
			HANDLE SubFile;

			if( SFileOpenFileEx( MapMPQ, "war3map.w3i", 0, &SubFile ) )
			{
				uint32_t FileLength = SFileGetFileSize( SubFile, nullptr );

				if( FileLength > 0 && FileLength != 0xFFFFFFFF )
				{
					char *SubFileData = new char[FileLength];
					DWORD BytesRead = 0;
					LPOVERLAPPED lpOverlapped = 0;

					if( SFileReadFile( SubFile, SubFileData, FileLength, &BytesRead, lpOverlapped ) )
					{
						istringstream ISS( string( SubFileData, BytesRead ) );

						// war3map.w3i format found at http://www.wc3campaigns.net/tools/specs/index.html by Zepir/PitzerMike

						string GarbageString;
						uint32_t FileFormat;
						uint32_t RawMapWidth;
						uint32_t RawMapHeight;
						uint32_t RawMapFlags;
						uint32_t RawMapNumPlayers;
                        uint32_t RawMapNumTeams;
                        ISS.read( (char *)&FileFormat, 4 );				// file format (18 = ROC, 25 = TFT)

						if( FileFormat == 18 || FileFormat == 25 )
						{
							ISS.seekg( 4, ios::cur );					// number of saves
							ISS.seekg( 4, ios::cur );					// editor version
							getline( ISS, GarbageString, '\0' );		// map name
                            MapName = GarbageString;
							getline( ISS, GarbageString, '\0' );		// map author
                            MapAuthor = GarbageString;
							getline( ISS, GarbageString, '\0' );		// map description
                            MapDescription = GarbageString;
							getline( ISS, GarbageString, '\0' );		// players recommended
                            MapPlayers = GarbageString;

                            if( MapName.size( ) > 8 && MapName.substr( 0, 8 ) == "TRIGSTR_" )
                            {
                                MapName = "STRING " + UTIL_ToString( UTIL_ToUInt32CopyValue( MapName.substr( 8 ) ) );
                                MapNameTRIGSTR = true;
                            }
                            else if( !MapName.empty( ) )
                                m_MapName = MapName;

                            if( MapAuthor.size( ) > 8 && MapAuthor.substr( 0, 8 ) == "TRIGSTR_" )
                            {
                                MapAuthor = "STRING " + UTIL_ToString( UTIL_ToUInt32CopyValue( MapAuthor.substr( 8 ) ) );
                                MapAuthorTRIGSTR = true;
                            }
                            else if( !MapAuthor.empty( ) )
                                m_MapAuthor = MapAuthor;

                            if( MapDescription.size( ) > 8 && MapDescription.substr( 0, 8 ) == "TRIGSTR_" )
                            {
                                MapDescription = "STRING " + UTIL_ToString( UTIL_ToUInt32CopyValue( MapDescription.substr( 8 ) ) );
                                MapDescriptionTRIGSTR = true;
                            }
                            else if( !MapDescription.empty( ) )
                                m_MapDescription = MapDescription;

                            if( MapPlayers.size( ) > 8 && MapPlayers.substr( 0, 8 ) == "TRIGSTR_" )
                            {
                                MapPlayers = "STRING " + UTIL_ToString( UTIL_ToUInt32CopyValue( MapPlayers.substr( 8 ) ) );
                                MapPlayersTRIGSTR = true;
                            }
                            else if( !MapPlayers.empty( ) )
                                m_MapPlayers = MapPlayers;

                            ISS.seekg( 32, ios::cur );                  // camera bounds
                            ISS.seekg( 16, ios::cur );                  // camera bounds complements
							ISS.read( (char *)&RawMapWidth, 4 );		// map width
							ISS.read( (char *)&RawMapHeight, 4 );		// map height
							ISS.read( (char *)&RawMapFlags, 4 );		// flags
							ISS.seekg( 1, ios::cur );					// map main ground type

							if( FileFormat == 18 )
								ISS.seekg( 4, ios::cur );				// campaign background number
							else if( FileFormat == 25 )
							{
								ISS.seekg( 4, ios::cur );				// loading screen background number
								getline( ISS, GarbageString, '\0' );	// path of custom loading screen model
							}

							getline( ISS, GarbageString, '\0' );		// map loading screen text
							getline( ISS, GarbageString, '\0' );		// map loading screen title
							getline( ISS, GarbageString, '\0' );		// map loading screen subtitle

							if( FileFormat == 18 )
								ISS.seekg( 4, ios::cur );				// map loading screen number
							else if( FileFormat == 25 )
							{
								ISS.seekg( 4, ios::cur );				// used game data set
								getline( ISS, GarbageString, '\0' );	// prologue screen path
							}

							getline( ISS, GarbageString, '\0' );		// prologue screen text
							getline( ISS, GarbageString, '\0' );		// prologue screen title
							getline( ISS, GarbageString, '\0' );		// prologue screen subtitle

							if( FileFormat == 25 )
							{
								ISS.seekg( 4, ios::cur );				// uses terrain fog
								ISS.seekg( 4, ios::cur );				// fog start z height
								ISS.seekg( 4, ios::cur );				// fog end z height
								ISS.seekg( 4, ios::cur );				// fog density
								ISS.seekg( 1, ios::cur );				// fog red value
								ISS.seekg( 1, ios::cur );				// fog green value
								ISS.seekg( 1, ios::cur );				// fog blue value
								ISS.seekg( 1, ios::cur );				// fog alpha value
								ISS.seekg( 4, ios::cur );				// global weather id
								getline( ISS, GarbageString, '\0' );	// custom sound environment
								ISS.seekg( 1, ios::cur );				// tileset id of the used custom light environment
								ISS.seekg( 1, ios::cur );				// custom water tinting red value
								ISS.seekg( 1, ios::cur );				// custom water tinting green value
								ISS.seekg( 1, ios::cur );				// custom water tinting blue value
								ISS.seekg( 1, ios::cur );				// custom water tinting alpha value
							}

							ISS.read( (char *)&RawMapNumPlayers, 4 );	// number of players
							uint32_t ClosedSlots = 0;

							for( uint32_t i = 0; i < RawMapNumPlayers; i++ )
							{
								CGameSlot Slot( 0, 255, SLOTSTATUS_OPEN, 0, 0, 1, SLOTRACE_RANDOM );
								uint32_t Colour;
								uint32_t Status;
								uint32_t Race;

								ISS.read( (char *)&Colour, 4 );			// colour
                                Slot.SetColour( static_cast<unsigned char>(Colour) );
								ISS.read( (char *)&Status, 4 );			// status

								if( Status == 1 )
									Slot.SetSlotStatus( SLOTSTATUS_OPEN );
								else if( Status == 2 )
								{
									Slot.SetSlotStatus( SLOTSTATUS_OCCUPIED );
									Slot.SetComputer( 1 );
									Slot.SetComputerType( SLOTCOMP_NORMAL );
								}
								else
								{
									Slot.SetSlotStatus( SLOTSTATUS_CLOSED );
									ClosedSlots++;
								}

								ISS.read( (char *)&Race, 4 );			// race

								if( Race == 1 )
                                    Slot.SetRace( m_UNM->m_PlayersCanChangeRace ? 65 : SLOTRACE_HUMAN );
								else if( Race == 2 )
                                    Slot.SetRace( m_UNM->m_PlayersCanChangeRace ? 66 : SLOTRACE_ORC );
								else if( Race == 3 )
                                    Slot.SetRace( m_UNM->m_PlayersCanChangeRace ? 72 : SLOTRACE_UNDEAD );
								else if( Race == 4 )
                                    Slot.SetRace( m_UNM->m_PlayersCanChangeRace ? 68 : SLOTRACE_NIGHTELF );
								else
                                    Slot.SetRace( m_UNM->m_PlayersCanChangeRace ? 96 : SLOTRACE_RANDOM );

								ISS.seekg( 4, ios::cur );				// fixed start position
								getline( ISS, GarbageString, '\0' );	// player name
								ISS.seekg( 4, ios::cur );				// start position x
								ISS.seekg( 4, ios::cur );				// start position y
								ISS.seekg( 4, ios::cur );				// ally low priorities
								ISS.seekg( 4, ios::cur );				// ally high priorities

								if( Slot.GetSlotStatus( ) != SLOTSTATUS_CLOSED )
									Slots.push_back( Slot );
							}

							ISS.read( (char *)&RawMapNumTeams, 4 );		// number of teams

							for( uint32_t i = 0; i < RawMapNumTeams; i++ )
							{
								uint32_t Flags;
								uint32_t PlayerMask;

								ISS.read( (char *)&Flags, 4 );			// flags
								ISS.read( (char *)&PlayerMask, 4 );		// player mask

								for( unsigned char j = 0; j < 12; j++ )
								{
									if( PlayerMask & 1 )
									{
										for( vector<CGameSlot> ::iterator k = Slots.begin( ); k != Slots.end( ); k++ )
										{
											if( ( *k ).GetColour( ) == j )
                                                ( *k ).SetTeam( static_cast<unsigned char>(i) );
										}
									}

									PlayerMask >>= 1;
								}

								getline( ISS, GarbageString, '\0' );	// team name
							}

							MapWidth = UTIL_CreateByteArray( (uint16_t)RawMapWidth, false );
							CONSOLE_Print( "[MAP] calculated map_width = " + UTIL_ToString( MapWidth[0] ) + " " + UTIL_ToString( MapWidth[1] ) );
							MapHeight = UTIL_CreateByteArray( (uint16_t)RawMapHeight, false );
							CONSOLE_Print( "[MAP] calculated map_height = " + UTIL_ToString( MapHeight[0] ) + " " + UTIL_ToString( MapHeight[1] ) );
							MapNumPlayers = RawMapNumPlayers - ClosedSlots;
							CONSOLE_Print( "[MAP] calculated map_numplayers = " + UTIL_ToString( MapNumPlayers ) );
							MapNumTeams = RawMapNumTeams;
							CONSOLE_Print( "[MAP] calculated map_numteams = " + UTIL_ToString( MapNumTeams ) );
							BYTEARRAY b;

							for( vector<CGameSlot> ::iterator i = Slots.begin( ); i != Slots.end( ); i++ )
							{
								b = ( *i ).GetByteArray( );

								if( b.size( ) >= 3 && (int)b[2] == 0 )
									FreeSlots++;
							}

							CONSOLE_Print( "[MAP] found " + UTIL_ToString( FreeSlots ) + "/" + UTIL_ToString( Slots.size( ) ) + " slots" );

							// if it's a melee map...

							if( RawMapFlags & 4 )
							{
								CONSOLE_Print( "[MAP] found melee map, initializing slots" );

								// give each slot a different team and set the race to random

								unsigned char Team = 0;

								for( vector<CGameSlot> ::iterator i = Slots.begin( ); i != Slots.end( ); i++ )
								{
									( *i ).SetTeam( Team++ );
									( *i ).SetRace( 96 );
								}

                                m_MeleeMap = true;
								MapFilterType = MAPFILTER_TYPE_MELEE;
								MapGameType = 2;
							}
							else
								MapGameType = 1;
						}
					}
					else
						CONSOLE_Print( "[MAP] unable to calculate map_width, map_height, map_slot<x>, map_numplayers, map_numteams - unable to extract war3map.w3i from MPQ file" );

					delete[] SubFileData;
				}

				SFileCloseFile( SubFile );
			}
			else
				CONSOLE_Print( "[MAP] unable to calculate map_width, map_height, map_slot<x>, map_numplayers, map_numteams - couldn't find war3map.w3i in MPQ file" );
		}
		else
            CONSOLE_Print( "[MAP] unable to calculate map_width, map_height, map_slot<x>, map_numplayers, map_numteams - map MPQ file not loaded" );

        if( MapMPQReady )
        {
            vector<string> FileList;
            FileList.push_back( "war3mapPreview.tga" );
            FileList.push_back( "war3mapMap.tga" );
//            FileList.push_back( "war3mapMap.blp" );

            for( vector<string> ::iterator i = FileList.begin( ); i != FileList.end( ); i++ )
            {
                HANDLE SubFile;
                bool tgafound = false;

                if( SFileOpenFileEx( MapMPQ, ( *i ).c_str( ), 0, &SubFile ) )
                {
                    uint32_t FileLength = SFileGetFileSize( SubFile, nullptr );

                    if( FileLength > 0 && FileLength != 0xFFFFFFFF )
                    {
                        char *SubFileData = new char[FileLength];
                        DWORD BytesRead = 0;
                        LPOVERLAPPED lpOverlapped = 0;

                        if( SFileReadFile( SubFile, SubFileData, FileLength, &BytesRead, lpOverlapped ) )
                        {
                            if( BytesRead > 10 )
                            {
                                m_MapTGA = BYTEARRAY( SubFileData, SubFileData + BytesRead );
                                tgafound = true;
                            }
                        }

                        delete[] SubFileData;
                    }

                    SFileCloseFile( SubFile );
                }

                if( tgafound )
                    break;
            }
        }

        if( MapMPQReady && ( MapNameTRIGSTR || MapAuthorTRIGSTR || MapDescriptionTRIGSTR || MapPlayersTRIGSTR ) )
        {
            HANDLE SubFile;

            if( SFileOpenFileEx( MapMPQ, "war3map.wts", 0, &SubFile ) )
            {
                uint32_t FileLength = SFileGetFileSize( SubFile, nullptr );

                if( FileLength > 0 && FileLength != 0xFFFFFFFF )
                {
                    char *SubFileData = new char[FileLength];
                    DWORD BytesRead = 0;
                    LPOVERLAPPED lpOverlapped = 0;

                    if( SFileReadFile( SubFile, SubFileData, FileLength, &BytesRead, lpOverlapped ) )
                    {
                        string strs = string( SubFileData, BytesRead );

                        if( MapNameTRIGSTR )
                        {
                            string::size_type MapNameStart = strs.find( MapName );

                            if( MapNameStart != string::npos )
                            {
                                MapName = strs.substr( MapNameStart + MapName.size( ) );
                                MapNameStart = MapName.find( "{" );

                                if( MapNameStart != string::npos )
                                {
                                    MapName = MapName.substr( MapNameStart + 1 );
                                    MapNameStart = MapName.find( "}" );

                                    if( MapNameStart != string::npos )
                                    {
                                        MapName = MapName.substr( 0, MapNameStart );
                                        std::string::iterator it = std::remove( MapName.begin( ), MapName.end( ), '\n' );
                                        MapName.erase( it, MapName.end( ) );
                                        std::string::iterator it2 = std::remove( MapName.begin( ), MapName.end( ), '\r' );
                                        MapName.erase( it2, MapName.end( ) );

                                        while( !MapName.empty( ) && MapName.back( ) == ' ' )
                                            MapName.erase( MapName.end( ) - 1 );
                                        while( !MapName.empty( ) && MapName.front( ) == ' ' )
                                            MapName.erase( MapName.begin( ) );

                                        if( !MapName.empty( ) )
                                            m_MapName = MapName;
                                    }
                                }
                            }
                        }

                        if( MapAuthorTRIGSTR )
                        {
                            string::size_type MapAuthorStart = strs.find( MapAuthor );

                            if( MapAuthorStart != string::npos )
                            {
                                MapAuthor = strs.substr( MapAuthorStart + MapAuthor.size( ) );
                                MapAuthorStart = MapAuthor.find( "{" );

                                if( MapAuthorStart != string::npos )
                                {
                                    MapAuthor = MapAuthor.substr( MapAuthorStart + 1 );
                                    MapAuthorStart = MapAuthor.find( "}" );

                                    if( MapAuthorStart != string::npos )
                                    {
                                        MapAuthor = MapAuthor.substr( 0, MapAuthorStart );
                                        std::string::iterator it = std::remove( MapAuthor.begin( ), MapAuthor.end( ), '\n' );
                                        MapAuthor.erase( it, MapAuthor.end( ) );
                                        std::string::iterator it2 = std::remove( MapAuthor.begin( ), MapAuthor.end( ), '\r' );
                                        MapAuthor.erase( it2, MapAuthor.end( ) );

                                        while( !MapAuthor.empty( ) && MapAuthor.back( ) == ' ' )
                                            MapAuthor.erase( MapAuthor.end( ) - 1 );
                                        while( !MapAuthor.empty( ) && MapAuthor.front( ) == ' ' )
                                            MapAuthor.erase( MapAuthor.begin( ) );

                                        if( !MapAuthor.empty( ) )
                                            m_MapAuthor = MapAuthor;
                                    }
                                }
                            }
                        }

                        if( MapDescriptionTRIGSTR )
                        {
                            string::size_type MapDescriptionStart = strs.find( MapDescription );

                            if( MapDescriptionStart != string::npos )
                            {
                                MapDescription = strs.substr( MapDescriptionStart + MapDescription.size( ) );
                                MapDescriptionStart = MapDescription.find( "{" );

                                if( MapDescriptionStart != string::npos )
                                {
                                    MapDescription = MapDescription.substr( MapDescriptionStart + 1 );
                                    MapDescriptionStart = MapDescription.find( "}" );

                                    if( MapDescriptionStart != string::npos )
                                    {
                                        MapDescription = MapDescription.substr( 0, MapDescriptionStart );
                                        std::string::iterator it = std::remove( MapDescription.begin( ), MapDescription.end( ), '\n' );
                                        MapDescription.erase( it, MapDescription.end( ) );
                                        std::string::iterator it2 = std::remove( MapDescription.begin( ), MapDescription.end( ), '\r' );
                                        MapDescription.erase( it2, MapDescription.end( ) );

                                        while( !MapDescription.empty( ) && MapDescription.back( ) == ' ' )
                                            MapDescription.erase( MapDescription.end( ) - 1 );
                                        while( !MapDescription.empty( ) && MapDescription.front( ) == ' ' )
                                            MapDescription.erase( MapDescription.begin( ) );

                                        if( !MapDescription.empty( ) )
                                            m_MapDescription = MapDescription;
                                    }
                                }
                            }
                        }

                        if( MapPlayersTRIGSTR )
                        {
                            string::size_type MapPlayersStart = strs.find( MapPlayers );

                            if( MapPlayersStart != string::npos )
                            {
                                MapPlayers = strs.substr( MapPlayersStart + MapPlayers.size( ) );
                                MapPlayersStart = MapPlayers.find( "{" );

                                if( MapPlayersStart != string::npos )
                                {
                                    MapPlayers = MapPlayers.substr( MapPlayersStart + 1 );
                                    MapPlayersStart = MapPlayers.find( "}" );

                                    if( MapPlayersStart != string::npos )
                                    {
                                        MapPlayers = MapPlayers.substr( 0, MapPlayersStart );
                                        std::string::iterator it = std::remove( MapPlayers.begin( ), MapPlayers.end( ), '\n' );
                                        MapPlayers.erase( it, MapPlayers.end( ) );
                                        std::string::iterator it2 = std::remove( MapPlayers.begin( ), MapPlayers.end( ), '\r' );
                                        MapPlayers.erase( it2, MapPlayers.end( ) );

                                        while( !MapPlayers.empty( ) && MapPlayers.back( ) == ' ' )
                                            MapPlayers.erase( MapPlayers.end( ) - 1 );
                                        while( !MapPlayers.empty( ) && MapPlayers.front( ) == ' ' )
                                            MapPlayers.erase( MapPlayers.begin( ) );

                                        if( !MapPlayers.empty( ) )
                                            m_MapPlayers = MapPlayers;
                                    }
                                }
                            }
                        }
                    }

                    delete[] SubFileData;
                }

                SFileCloseFile( SubFile );
            }
        }
	}
	else
		CONSOLE_Print( "[MAP] no map data available, using config file for map_width, map_height, map_slot<x>, map_numplayers, map_numteams" );

    // write config

	if( m_LogAll )
	{
		string s = string( );
		string spad = string( );
		string fmap = m_MapLocalPath.substr( 0, m_MapLocalPath.length( ) - 4 );
        string File = m_UNM->m_MapCFGPath + fmap + ".cfg";

#ifdef WIN32
        string FilePath = m_UNM->m_MapCFGPath;
        File = UTIL_UTF8ToCP1251( File.c_str( ) );

        if( FilePath.size( ) > 1 && ( FilePath.substr( FilePath.size( ) - 1 ) == "\\" || FilePath.substr( FilePath.size( ) - 1 ) == "/" ) )
        {
            FilePath = FilePath.substr( 0, FilePath.size( ) - 1 );

            std::wstring WFilePath = std::wstring_convert<std::codecvt_utf8<wchar_t>>( ).from_bytes(FilePath);

            if( !std::filesystem::exists( WFilePath ) )
            {
                if( std::filesystem::create_directory( WFilePath ) )
                    CONSOLE_Print( "[UNM] Была создана папка \"" + FilePath + "\"" );
                else
                    CONSOLE_Print( "[UNM] Не удалось создать папку \"" + FilePath + "\"" );
            }
        }
#endif
		ofstream tmpcfg;
        tmpcfg.open( File, ios::trunc );
		s = "# map file for " + m_MapLocalPath + " #";
        QString Qs = QString::fromUtf8( s.c_str( ) );
        spad = spad.insert( 0, Qs.size( ), '#' );
		tmpcfg << spad << endl;
		tmpcfg << s << endl;
		tmpcfg << spad << endl;
		tmpcfg << endl;
        tmpcfg << "# путь к карте сообщает клиентам Warcraft III, где найти карту в их системе" << endl;
        tmpcfg << "# это НЕ путь к карте в системе UNM, фактически UNM вообще не нуждается в карте" << endl;
        tmpcfg << "map_path = Maps\\Download\\" + m_MapLocalPath << endl;

		if( MapSize.empty( ) )
			tmpcfg << "map_size = " << endl;
		else
			tmpcfg << "map_size = " << UTIL_ToString( MapSize[0] ) + " " + UTIL_ToString( MapSize[1] ) + " " + UTIL_ToString( MapSize[2] ) + " " + UTIL_ToString( MapSize[3] ) << endl;

		if( MapInfo.empty( ) )
			tmpcfg << "map_info = " << endl;
		else
			tmpcfg << "map_info = " << UTIL_ToString( MapInfo[0] ) + " " + UTIL_ToString( MapInfo[1] ) + " " + UTIL_ToString( MapInfo[2] ) + " " + UTIL_ToString( MapInfo[3] ) << endl;

		if( MapCRC.empty( ) )
			tmpcfg << "map_crc = " << endl;
		else
			tmpcfg << "map_crc = " << UTIL_ToString( MapCRC[0] ) + " " + UTIL_ToString( MapCRC[1] ) + " " + UTIL_ToString( MapCRC[2] ) + " " + UTIL_ToString( MapCRC[3] ) << endl;

		if( MapSHA1.empty( ) )
			tmpcfg << "map_sha1 = " << endl;
		else
			tmpcfg << "map_sha1 = " << UTIL_ByteArrayToDecString( MapSHA1 ) << endl;

		tmpcfg << endl;
        tmpcfg << "# скорость игры" << endl;
        tmpcfg << "# 1 = медленный" << endl;
        tmpcfg << "# 2 = средний" << endl;
        tmpcfg << "# 3 = быстрый" << endl;
        tmpcfg << "map_speed = 3" << endl;
		tmpcfg << endl;
        tmpcfg << "# обзор карты" << endl;
        tmpcfg << "# 1 = скрыта" << endl;
        tmpcfg << "# 2 = разведана" << endl;
        tmpcfg << "# 3 = открыта" << endl;
        tmpcfg << "# 4 = по умолчанию" << endl;
        tmpcfg << "map_visibility = 4" << endl;
		tmpcfg << endl;
        tmpcfg << "# зрители" << endl;
        tmpcfg << "# 1 = запрещены" << endl;
        tmpcfg << "# 2 = после поражения" << endl;
        tmpcfg << "# 3 = разрешены" << endl;
        tmpcfg << "# 4 = судьи" << endl;

		if( FreeSlots > 1 )
            tmpcfg << "map_observers = 1" << endl;
		else
            tmpcfg << "map_observers = 4" << endl;

		tmpcfg << endl;
        tmpcfg << "# опции (флаги) игры" << endl;
        tmpcfg << "# вы можете объединить эти флаги, сложив вместе все опции, которые вы хотите использовать" << endl;
        tmpcfg << "# например, чтобы включить опции \"города союзников рядом\" и \"фиксация кланов\", вы должны указать \"map_flags = 3\"" << endl;
        tmpcfg << "# 1 = города союзников рядом" << endl;
        tmpcfg << "# 2 = фиксация кланов" << endl;
        tmpcfg << "# 4 = общие войска (передать союзникам контроль над юнитами)" << endl;
        tmpcfg << "# 8 = случайный выбор героев" << endl;
        tmpcfg << "# 16 = случайный выбор рас" << endl;
        tmpcfg << "map_flags = 3" << endl;
		tmpcfg << endl;
        tmpcfg << "# тип карты (custom/melee)" << endl;
        tmpcfg << "# 1 = custom" << endl;
        tmpcfg << "# 2 = melee" << endl;

        if( MapGameType == 0 )
            tmpcfg << "map_gametype = 1" << endl;
        else
            tmpcfg << "map_gametype = " << UTIL_ToString( MapGameType ) << endl;

        if( MapFilterType == MAPFILTER_TYPE_MELEE )
            tmpcfg << "map_filter_type = 2" << endl;
        else
            tmpcfg << "map_filter_type = 1" << endl;

		tmpcfg << endl;
        tmpcfg << "# размер карты" << endl;

		if( MapWidth.empty( ) )
			tmpcfg << "map_width = " << endl;
		else
			tmpcfg << "map_width = " << UTIL_ToString( MapWidth[0] ) + " " + UTIL_ToString( MapWidth[1] ) << endl;

		if( MapHeight.empty( ) )
			tmpcfg << "map_height = " << endl;
		else
			tmpcfg << "map_height = " << UTIL_ToString( MapHeight[0] ) + " " + UTIL_ToString( MapHeight[1] ) << endl;

		tmpcfg << endl;
        tmpcfg << "# количество игровых слотов" << endl;
		tmpcfg << "map_numplayers = " << UTIL_ToString( MapNumPlayers ) << endl;
        tmpcfg << endl;
        tmpcfg << "# количество команд" << endl;
		tmpcfg << "map_numteams = " << UTIL_ToString( MapNumTeams ) << endl;
		tmpcfg << endl;
        tmpcfg << "# структура слотов" << endl;
        tmpcfg << "# [номер игрока (PID)] [статус загрузки карты] [статус слота] [бот] [команда] [цвет] [раса] [скилл бота] [гандикап]" << endl;
        tmpcfg << "# номер игрока (PID) должен быть всегда \"0\"" << endl;
        tmpcfg << "# статус загрузки карты должен быть всегда \"255\"" << endl;
        tmpcfg << "# статус слота: 0 = открыт, 1 = закрыт, 2 = занятый" << endl;
        tmpcfg << "# бот: 1 = добавить на слот бота, 0 = не добавлять" << endl;
        tmpcfg << "# команда (0-11), не может быть >= map_numteams" << endl;
        tmpcfg << "# цвет (0-11)" << endl;
        tmpcfg << "# раса: 1 = альянс, 2 = орда, 4 = ночные эльфы, 8 = нежить, 32 = случайная раса (чтобы разрешить менять расу - необходимо добавить к выбранному значению \"64\")" << endl;
        tmpcfg << "# например, чтобы выбрать для слота расу нежить, а также разрешить игрокам менять расу на данном слоте, вы должны указать значение \"72\"" << endl;
        tmpcfg << "# скилл бота: 0 = легкий, 1 = средний, 2 = тяжелый" << endl;
        tmpcfg << "# гандикап указывает сколько процентов от максимального здоровья оставить для всех юнитов игрока на этом слоте, возможные значения: 50/60/70/80/90/100" << endl;
		BYTEARRAY b;
		string ss;
        int k = 0;

		for( vector<CGameSlot> ::iterator i = Slots.begin( ); i != Slots.end( ); i++ )
		{
			k++;
			b = ( *i ).GetByteArray( );
			ss = "map_slot" + UTIL_ToString( k ) + " = ";

			for( uint32_t j = 0; j < b.size( ); j++ )
			{
				ss = ss + UTIL_ToString( (int)b[j] );

				if( j < b.size( ) - 1 )
					ss = ss + " ";
			}

			tmpcfg << ss << endl;
		}

		tmpcfg << endl;
        tmpcfg << "# дополнительные правила игры (специальные кодовые слова)" << endl;
        tmpcfg << "# используйте специальные кодовые слова для установки определенных правил для конкретной карты:" << endl;
        tmpcfg << "# obs - при создании игры добавляет в лобби obs-player'а (!obs) под именем указанным в unm.cfg (bot_obsplayername)" << endl;
        tmpcfg << "# noobs - запрещает добавлять в лобби obs-player'ов" << endl;
        tmpcfg << "# noafk - при включенном в unm.cfg AFK Patch (bot_afkkick = 1) разрешает боту кикать из начатой игры AFK игроков в автоматическом порядке" << endl;
        tmpcfg << "# notrade - разрешает боту кикать из начатой игры игроков передающих ресурсы (золото/дерево) в автоматическом порядке" << endl;
        tmpcfg << "# nopause - запрещает игрокам установку пауз в игре (пауза будет мгновенно сниматься в автоматическом порядке)" << endl;
        tmpcfg << "# nosave - запрещает игрокам сохранять игру (не реплей)" << endl;
        tmpcfg << "# guard - устанавливает правила 'noafk' + 'notrade' + 'nopause' + 'nosave' одним махом" << endl;
        tmpcfg << "# noemptyteams - запрещает начинать игру простым юзерам (не админам) если имеется 2 команды (клана), в одной из которых нет игроков; актуально если bot_autohostallowstart = 1" << endl;
        tmpcfg << "# equalteams - запрещает начинать игру простым юзерам (не админам) если имеется 2 команды (клана) равные по количеству слотов, но не равные по количеству игроков; актуально если bot_autohostallowstart = 1" << endl;
        tmpcfg << "# 2team - делает актуальными правила 'noemptyteams' и 'equalteams' (не включает!) для карт, которые имеет более двух команд (кланов); актуально если bot_autohostallowstart = 1" << endl;
        tmpcfg << "# nochangehandicap - запрещает игрокам менять значение гандикапа на своем слоте" << endl;
		s = "map_type = ";
		bool dota = false;
		string MapLocalPathLower = m_MapLocalPath;
		transform( MapLocalPathLower.begin( ), MapLocalPathLower.end( ), MapLocalPathLower.begin( ), static_cast<int(*)(int)>(tolower) );

		if( MapLocalPathLower.find( "dota v" ) != string::npos && MapLocalPathLower.find( "lod" ) == string::npos && MapLocalPathLower.find( "omg" ) == string::npos && MapLocalPathLower.find( "fun" ) == string::npos && MapLocalPathLower.find( "mod" ) == string::npos )
			dota = true;

		if( dota )
			s = s + "dota";

		tmpcfg << s << endl;
		s = "map_matchmakingcategory = ";

        if( dota )
            s = s + "dota_elo";

        tmpcfg << endl;
        tmpcfg << "# категория матчмейкинга" << endl;
        tmpcfg << s << endl;
        tmpcfg << endl;
        tmpcfg << "# категория статистики w3mmd" << endl;
		tmpcfg << "map_statsw3mmdcategory = " << endl;
		tmpcfg << endl;
        tmpcfg << "# HCL по умолчанию (мод игры)" << endl;
        tmpcfg << "# примечание: актуально, если карта поддерживает HCL" << endl;
        tmpcfg << "map_defaulthcl = " << endl;
        tmpcfg << endl;
        tmpcfg << "# стартовые очки (актуально при включенной статистике)" << endl;
        tmpcfg << "map_defaultplayerscore = 1000" << endl;
        tmpcfg << endl;
        tmpcfg << "# поведение бота при загрузке карты игроками после старта" << endl;
        tmpcfg << "# если = 0, то игроки, загрузившие карту раньше других, будут ожидать остальных на этапе лоадскрина" << endl;
        tmpcfg << "# если = 1, то игроки, загрузившие карту раньше других, будут ожидать остальных прямо в игре, имея возможность писать в чат, пользоваться некоторыми командами бота" << endl;
        tmpcfg << "# примечание: если = 1, то игроки, загрузившие карту раньше других, могут посмотреть кто еще не загрузился на лагскрине (который не даст игре начаться пока все не загрузят карту)" << endl;

        if( m_UNM->m_ForceLoadInGame )
            tmpcfg << "map_loadingame = 1" << endl;
        else
            tmpcfg << "map_loadingame = 0" << endl;

        tmpcfg << endl;
        tmpcfg << "# значение map_autowarntime указывает сколько процентов от продолжительности игры (не меньше чем) должен пробыть в игре юзер, чтобы избежать автоварна за преждевременный выход из игры" << endl;
        tmpcfg << "# значение map_autowarnplayers указывает сколько игроков (минимум) должно находится в игре на момент лива (включая самого ливера) чтобы данная опция работала" << endl;
        tmpcfg << "# примечание: если map_autowarntime = 0, либо map_autowarnplayers = 0, то данная опция отключена" << endl;
        tmpcfg << "# опция актуальна только при включенном автоварне в главном конфигурационном файле \"unm.cfg\" (bot_autowarn = 1)" << endl;
        tmpcfg << "map_autowarntime = 0" << endl;
        tmpcfg << "map_autowarnplayers = 0" << endl;
        tmpcfg << endl;
        tmpcfg << "# локальный путь карты (для загрузки карт)" << endl;
        tmpcfg << "# UNM не требует файлов карт, но если у него есть доступ к ним, он может отправлять их игрокам" << endl;
        tmpcfg << "# UNM будет искать файл карты по пути [bot_mappath + map_localpath] (bot_mappath устанавливается в основном конфигурационном файле)" << endl;
        tmpcfg << "# таким образом значение map_localpath должно равняться названию карты" << endl;
		tmpcfg << "map_localpath = " << m_MapLocalPath << endl;
		tmpcfg.close( );
        CONSOLE_Print( "[MAP] создание конфигурационного файла [" + fmap + ".cfg] завершено" );
    }

	// close the map MPQ

	if( MapMPQReady )
		SFileCloseArchive( MapMPQ );

	m_MapPath = CFG->GetString( "map_path", string( ) );

	if( MapSize.empty( ) )
		MapSize = UTIL_ExtractNumbers( CFG->GetString( "map_size", string( ) ), 4 );
    else if( ( IsCFGFile || m_UNM->m_OverrideMapSizeFromCFG ) && CFG->Exists( "map_size" ) )
	{
		CONSOLE_Print( "[MAP] overriding calculated map_size with config value map_size = " + CFG->GetString( "map_size", string( ) ) );
		MapSize = UTIL_ExtractNumbers( CFG->GetString( "map_size", string( ) ), 4 );
	}

	m_MapSize = MapSize;

	if( MapInfo.empty( ) )
		MapInfo = UTIL_ExtractNumbers( CFG->GetString( "map_info", string( ) ), 4 );
    else if( ( IsCFGFile || m_UNM->m_OverrideMapInfoFromCFG ) && CFG->Exists( "map_info" ) )
	{
		CONSOLE_Print( "[MAP] overriding calculated map_info with config value map_info = " + CFG->GetString( "map_info", string( ) ) );
		MapInfo = UTIL_ExtractNumbers( CFG->GetString( "map_info", string( ) ), 4 );
	}

	m_MapInfo = MapInfo;

	if( MapCRC.empty( ) )
		MapCRC = UTIL_ExtractNumbers( CFG->GetString( "map_crc", string( ) ), 4 );
    else if( ( IsCFGFile || m_UNM->m_OverrideMapCrcFromCFG ) && CFG->Exists( "map_crc" ) )
	{
		CONSOLE_Print( "[MAP] overriding calculated map_crc with config value map_crc = " + CFG->GetString( "map_crc", string( ) ) );
		MapCRC = UTIL_ExtractNumbers( CFG->GetString( "map_crc", string( ) ), 4 );
	}

	m_MapCRC = MapCRC;

	if( MapSHA1.empty( ) )
		MapSHA1 = UTIL_ExtractNumbers( CFG->GetString( "map_sha1", string( ) ), 20 );
    else if( ( IsCFGFile || m_UNM->m_OverrideMapSha1FromCFG ) && CFG->Exists( "map_sha1" ) )
	{
		CONSOLE_Print( "[MAP] overriding calculated map_sha1 with config value map_sha1 = " + CFG->GetString( "map_sha1", string( ) ) );
		MapSHA1 = UTIL_ExtractNumbers( CFG->GetString( "map_sha1", string( ) ), 20 );
	}

	m_MapSHA1 = MapSHA1;

    if( IsCFGFile || m_UNM->m_OverrideMapSpeedFromCFG )
        m_MapSpeed = static_cast<unsigned char>(CFG->GetInt( "map_speed", MAPSPEED_FAST ));
	else
		m_MapSpeed = MAPSPEED_FAST;

    if( IsCFGFile || m_UNM->m_OverrideMapVisibilityFromCFG )
        m_MapVisibility = static_cast<unsigned char>(CFG->GetInt( "map_visibility", MAPVIS_DEFAULT ));
	else
		m_MapVisibility = MAPVIS_DEFAULT;

    if( IsCFGFile || m_UNM->m_OverrideMapFlagsFromCFG )
        m_MapFlags = static_cast<unsigned char>(CFG->GetInt( "map_flags", MAPFLAG_TEAMSTOGETHER | MAPFLAG_FIXEDTEAMS ));
	else
		m_MapFlags = MAPFLAG_TEAMSTOGETHER | MAPFLAG_FIXEDTEAMS;

    if( ( IsCFGFile || m_UNM->m_OverrideMapFilterTypeFromCFG ) && CFG->Exists( "map_filter_type" ) )
	{
		CONSOLE_Print( "[MAP] overriding calculated map_filter_type with config value map_filter_type = " + CFG->GetString( "map_filter_type", string( ) ) );

        if( CFG->GetInt( "map_filter_type", 0 ) == 2 )
            MapFilterType = MAPFILTER_TYPE_MELEE;
        else
            MapFilterType = MAPFILTER_TYPE_SCENARIO;
	}

    m_MapFilterType = static_cast<unsigned char>(MapFilterType);

	if( MapGameType == 0 )
    {
        CONSOLE_Print( "[MAP] overriding calculated map_gametype with config value map_gametype = " + CFG->GetString( "map_gametype", string( ) ) + " (because value of MapGameType is undefined)" );
        MapGameType = CFG->GetUInt( "map_gametype", 1 );
    }
    else if( ( IsCFGFile || m_UNM->m_OverrideMapGameTypeFromCFG ) && CFG->Exists( "map_gametype" ) )
	{
		CONSOLE_Print( "[MAP] overriding calculated map_gametype with config value map_gametype = " + CFG->GetString( "map_gametype", string( ) ) );
        MapGameType = CFG->GetUInt( "map_gametype", 1 );
	}

    m_MapGameType = static_cast<unsigned char>(MapGameType);

	if( MapWidth.empty( ) )
		MapWidth = UTIL_ExtractNumbers( CFG->GetString( "map_width", string( ) ), 2 );
    else if( ( IsCFGFile || m_UNM->m_OverrideMapWidthFromCFG ) && CFG->Exists( "map_width" ) )
	{
		CONSOLE_Print( "[MAP] overriding calculated map_width with config value map_width = " + CFG->GetString( "map_width", string( ) ) );
		MapWidth = UTIL_ExtractNumbers( CFG->GetString( "map_width", string( ) ), 2 );
	}

	m_MapWidth = MapWidth;

	if( MapHeight.empty( ) )
		MapHeight = UTIL_ExtractNumbers( CFG->GetString( "map_height", string( ) ), 2 );
    else if( ( IsCFGFile || m_UNM->m_OverrideMapHeightFromCFG ) && CFG->Exists( "map_height" ) )
	{
		CONSOLE_Print( "[MAP] overriding calculated map_height with config value map_height = " + CFG->GetString( "map_height", string( ) ) );
		MapHeight = UTIL_ExtractNumbers( CFG->GetString( "map_height", string( ) ), 2 );
	}

	m_MapHeight = MapHeight;

    if( IsCFGFile || m_UNM->m_OverrideMapTypeFromCFG )
        m_MapType = CFG->GetString( "map_type", string( ) );
	else
		m_MapType = string( );


    if( IsCFGFile || m_UNM->m_OverrideMapDefaultHCLFromCFG )
        m_MapDefaultHCL = CFG->GetString( "map_defaulthcl", string( ) );
	else
		m_MapDefaultHCL = string( );

    if( ( IsCFGFile || m_UNM->m_OverrideMapLoadInGameFromCFG ) && CFG->Exists( "map_loadingame" ) )
		m_MapLoadInGame = CFG->GetInt( "map_loadingame", 0 ) == 0 ? false : true;
    else if( m_UNM->m_ForceLoadInGame )
		m_MapLoadInGame = true;
	else
		m_MapLoadInGame = false;

	if( MapNumPlayers == 0 )
        MapNumPlayers = CFG->GetUInt( "map_numplayers", 0 );
    else if( ( IsCFGFile || m_UNM->m_OverrideMapNumPlayersFromCFG ) && CFG->Exists( "map_numplayers" ) )
	{
		CONSOLE_Print( "[MAP] overriding calculated map_numplayers with config value map_numplayers = " + CFG->GetString( "map_numplayers", string( ) ) );
        MapNumPlayers = CFG->GetUInt( "map_numplayers", 0 );
	}

	m_MapNumPlayers = MapNumPlayers;

	if( MapNumTeams == 0 )
        MapNumTeams = CFG->GetUInt( "map_numteams", 0 );
    else if( ( IsCFGFile || m_UNM->m_OverrideMapNumTeamsFromCFG ) && CFG->Exists( "map_numteams" ) )
	{
		CONSOLE_Print( "[MAP] overriding calculated map_numteams with config value map_numteams = " + CFG->GetString( "map_numteams", string( ) ) );
        MapNumTeams = CFG->GetUInt( "map_numteams", 0 );
	}

    m_MapNumTeams = MapNumTeams;

    if( Slots.empty( ) )
	{
		FreeSlots = 0;

		for( uint32_t Slot = 1; Slot <= 12; Slot++ )
		{
			string SlotString = CFG->GetString( "map_slot" + UTIL_ToString( Slot ), string( ) );

			if( SlotString.empty( ) )
				break;

			if( SlotString.find( "0 255 0 " ) != string::npos )
				FreeSlots++;

			BYTEARRAY SlotData = UTIL_ExtractNumbers( SlotString, 9 );
			Slots.push_back( CGameSlot( SlotData ) );
		}
	}
    else if( ( IsCFGFile || m_UNM->m_OverrideMapSlotsFromCFG ) && CFG->Exists( "map_slot1" ) )
	{
		FreeSlots = 0;
		Slots.clear( );

		for( uint32_t Slot = 1; Slot <= 12; Slot++ )
		{
			string SlotString = CFG->GetString( "map_slot" + UTIL_ToString( Slot ), string( ) );

			if( SlotString.empty( ) )
				break;

			if( SlotString.find( "0 255 0 " ) != string::npos )
				FreeSlots++;

			BYTEARRAY SlotData = UTIL_ExtractNumbers( SlotString, 9 );
			Slots.push_back( CGameSlot( SlotData ) );
		}

		CONSOLE_Print( "[MAP] overriding slots (" + UTIL_ToString( FreeSlots ) + "/" + UTIL_ToString( Slots.size( ) ) + ")" );
	}

	m_Slots = Slots;

    if( ( IsCFGFile || m_UNM->m_OverrideMapObserversFromCFG ) && CFG->Exists( "map_observers" ) )
        m_MapObservers = static_cast<unsigned char>(CFG->GetInt( "map_observers", MAPOBS_NONE ));
	else if( FreeSlots > 1 )
		m_MapObservers = 1;
	else
		m_MapObservers = 4;

	// if random races is set force every slot's race to random + fixed

	if( m_MapFlags & MAPFLAG_RANDOMRACES )
	{
		CONSOLE_Print( "[MAP] forcing races to random" );

		for( vector<CGameSlot> ::iterator i = m_Slots.begin( ); i != m_Slots.end( ); i++ )
			( *i ).SetRace( SLOTRACE_RANDOM );
	}

	// add observer slots

	if( m_MapObservers == MAPOBS_ALLOWED || m_MapObservers == MAPOBS_REFEREES )
	{
		CONSOLE_Print( "[MAP] adding " + UTIL_ToString( 12 - m_Slots.size( ) ) + " observer slots" );

		while( m_Slots.size( ) < 12 )
			m_Slots.push_back( CGameSlot( 0, 255, SLOTSTATUS_OPEN, 0, 12, 12, SLOTRACE_RANDOM ) );
	}

	CheckValid( );
}

void CMap::CheckValid( )
{
	// todotodo: should this code fix any errors it sees rather than just warning the user?

	if( m_MapPath.empty( ) || m_MapPath.size( ) > 53 )
	{
		m_Valid = false;
		CONSOLE_Print( "[MAP] invalid map_path detected" );
	}
	else if( m_MapPath[0] == '\\' )
        CONSOLE_Print( "[MAP] warning - map_path starts with '\\', any replays saved by UNM will not be playable in Warcraft III" );

	if( m_MapPath.find( '/' ) != string::npos )
		CONSOLE_Print( "[MAP] warning - map_path contains forward slashes '/' but it must use Windows style back slashes '\\'" );

	if( m_MapSize.size( ) != 4 )
	{
		m_Valid = false;
		CONSOLE_Print( "[MAP] invalid map_size detected" );
	}
	else if( !m_MapData.empty( ) && m_MapData.size( ) != UTIL_ByteArrayToUInt32( m_MapSize, false ) )
	{
		m_Valid = false;
		CONSOLE_Print( "[MAP] invalid map_size detected - size mismatch with actual map data" );
	}

	if( m_MapInfo.size( ) != 4 )
	{
		m_Valid = false;
		CONSOLE_Print( "[MAP] invalid map_info detected" );
	}

	if( m_MapCRC.size( ) != 4 )
	{
		m_Valid = false;
		CONSOLE_Print( "[MAP] invalid map_crc detected" );
	}

	if( m_MapSHA1.size( ) != 20 )
	{
		m_Valid = false;
		CONSOLE_Print( "[MAP] invalid map_sha1 detected" );
	}

	if( m_MapSpeed != MAPSPEED_SLOW && m_MapSpeed != MAPSPEED_NORMAL && m_MapSpeed != MAPSPEED_FAST )
	{
		m_Valid = false;
		CONSOLE_Print( "[MAP] invalid map_speed detected" );
	}

	if( m_MapVisibility != MAPVIS_HIDETERRAIN && m_MapVisibility != MAPVIS_EXPLORED && m_MapVisibility != MAPVIS_ALWAYSVISIBLE && m_MapVisibility != MAPVIS_DEFAULT )
	{
		m_Valid = false;
		CONSOLE_Print( "[MAP] invalid map_visibility detected" );
	}

	if( m_MapObservers != MAPOBS_NONE && m_MapObservers != MAPOBS_ONDEFEAT && m_MapObservers != MAPOBS_ALLOWED && m_MapObservers != MAPOBS_REFEREES )
	{
		m_Valid = false;
		CONSOLE_Print( "[MAP] invalid map_observers detected" );
	}

	// todotodo: m_MapFlags

	if( m_MapGameType != 1 && m_MapGameType != 2 && m_MapGameType != 9 )
	{
		m_Valid = false;
		CONSOLE_Print( "[MAP] invalid map_gametype detected" );
	}

	if( m_MapWidth.size( ) != 2 )
	{
		m_Valid = false;
		CONSOLE_Print( "[MAP] invalid map_width detected" );
	}

	if( m_MapHeight.size( ) != 2 )
	{
		m_Valid = false;
		CONSOLE_Print( "[MAP] invalid map_height detected" );
	}

	if( m_MapNumPlayers == 0 || m_MapNumPlayers > 12 )
	{
		m_Valid = false;
		CONSOLE_Print( "[MAP] invalid map_numplayers detected" );
	}

	if( m_MapNumTeams == 0 || m_MapNumTeams > 12 )
	{
		m_Valid = false;
		CONSOLE_Print( "[MAP] invalid map_numteams detected" );
	}

	if( m_Slots.empty( ) || m_Slots.size( ) > 12 )
	{
		m_Valid = false;
		CONSOLE_Print( "[MAP] invalid map_slot<x> detected" );
	}
}

uint32_t CMap::XORRotateLeft( unsigned char *data, uint32_t length )
{
	// a big thank you to Strilanc for figuring this out

	uint32_t i = 0;
	uint32_t Val = 0;

	if( length > 3 )
	{
		while( i < length - 3 )
		{
			Val = ROTL( Val ^ ( (uint32_t)data[i] + (uint32_t)( data[i + 1] << 8 ) + (uint32_t)( data[i + 2] << 16 ) + (uint32_t)( data[i + 3] << 24 ) ), 3 );
			i += 4;
		}
	}

	while( i < length )
	{
		Val = ROTL( Val ^ data[i], 3 );
		i++;
	}

	return Val;
}
