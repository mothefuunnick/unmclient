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
#include "bncsutilinterface.h"
#include "src/bncsutil/bncsutil.h"

using namespace std;

//
// CBNCSUtilInterface
//

CBNCSUtilInterface::CBNCSUtilInterface( string userName, string userPassword )
{
	m_NLS = new NLS( userName, userPassword );
}

CBNCSUtilInterface::~CBNCSUtilInterface( )
{
    delete static_cast<NLS *>(m_NLS);
}

string CBNCSUtilInterface::GetEXEVersionStr( )
{
    if( m_EXEVersion.size( ) >= 4 )
        return UTIL_ToString( m_EXEVersion[0] ) + " " + UTIL_ToString( m_EXEVersion[1] ) + " " + UTIL_ToString( m_EXEVersion[2] ) + " " + UTIL_ToString( m_EXEVersion[3] );
    else
        return "N/A";
}

string CBNCSUtilInterface::GetEXEVersionHashStr( )
{
    if( m_EXEVersionHash.size( ) >= 4 )
        return UTIL_ToString( m_EXEVersionHash[0] ) + " " + UTIL_ToString( m_EXEVersionHash[1] ) + " " + UTIL_ToString( m_EXEVersionHash[2] ) + " " + UTIL_ToString( m_EXEVersionHash[3] );
    else
        return "N/A";
}

void CBNCSUtilInterface::Reset( string userName, string userPassword )
{
    delete static_cast<NLS *>(m_NLS);
	m_NLS = new NLS( userName, userPassword );
}

bool CBNCSUtilInterface::HELP_SID_AUTH_CHECK( bool TFT, string war3Path, string keyROC, string keyTFT, string valueStringFormula, string mpqFileName, BYTEARRAY clientToken, BYTEARRAY serverToken )
{
	// set m_EXEVersion, m_EXEVersionHash, m_EXEInfo, m_InfoROC, m_InfoTFT

    string FileWar3EXE = war3Path + "war3.exe";
	string FileStormDLL = war3Path + "Storm.dll";
    string FileGameDLL = war3Path + "game.dll";

    if( !UTIL_FileExists( FileWar3EXE ) )
    {
        if( UTIL_FileExists( war3Path + "Warcraft III.exe" ) )
            FileWar3EXE = war3Path + "Warcraft III.exe";
        else if( UTIL_FileExists( war3Path + "warcraft.exe" ) )
            FileWar3EXE = war3Path + "warcraft.exe";
        else if( UTIL_FileExists( war3Path + "Frozen Throne.exe" ) )
            FileWar3EXE = war3Path + "Frozen Throne.exe";
        else if( UTIL_FileExists( war3Path + "w3l.exe" ) )
            FileWar3EXE = war3Path + "w3l.exe";
    }

    if( !UTIL_FileExists( FileStormDLL ) && UTIL_FileExists( war3Path + "storm.dll" ) )
        FileStormDLL = war3Path + "storm.dll";

    bool ExistsWar3EXE = UTIL_FileExists( FileWar3EXE );
    bool ExistsStormDLL = UTIL_FileExists( FileStormDLL );
    bool ExistsGameDLL = UTIL_FileExists( FileGameDLL );

	if( ExistsWar3EXE && ExistsStormDLL && ExistsGameDLL )
	{
		// todotodo: check getExeInfo return value to ensure 1024 bytes was enough

		char buf[1024];
		uint32_t EXEVersion;
        getExeInfo( FileWar3EXE.c_str( ), reinterpret_cast<char *>(&buf), 1024, static_cast<uint32_t *>(&EXEVersion), BNCSUTIL_PLATFORM_X86 );
		m_EXEInfo = buf;
		m_EXEVersion = UTIL_CreateByteArray( EXEVersion, false );
		uint32_t EXEVersionHash;
        checkRevisionFlat( valueStringFormula.c_str( ), FileWar3EXE.c_str( ), FileStormDLL.c_str( ), FileGameDLL.c_str( ), extractMPQNumber( mpqFileName.c_str( ) ), reinterpret_cast<unsigned long *>(&EXEVersionHash) );
		m_EXEVersionHash = UTIL_CreateByteArray( EXEVersionHash, false );
		m_KeyInfoROC = CreateKeyInfo( keyROC, UTIL_ByteArrayToUInt32( clientToken, false ), UTIL_ByteArrayToUInt32( serverToken, false ) );

		if( TFT )
			m_KeyInfoTFT = CreateKeyInfo( keyTFT, UTIL_ByteArrayToUInt32( clientToken, false ), UTIL_ByteArrayToUInt32( serverToken, false ) );

		if( m_KeyInfoROC.size( ) == 36 && ( !TFT || m_KeyInfoTFT.size( ) == 36 ) )
			return true;
		else
		{
			if( m_KeyInfoROC.size( ) != 36 )
				CONSOLE_Print( "[BNCSUI] unable to create ROC key info - invalid ROC key" );

			if( TFT && m_KeyInfoTFT.size( ) != 36 )
				CONSOLE_Print( "[BNCSUI] unable to create TFT key info - invalid TFT key" );
		}
	}
	else
	{
		if( !ExistsWar3EXE )
			CONSOLE_Print( "[BNCSUI] unable to open [" + FileWar3EXE + "]" );

		if( !ExistsStormDLL )
			CONSOLE_Print( "[BNCSUI] unable to open [" + FileStormDLL + "]" );

		if( !ExistsGameDLL )
			CONSOLE_Print( "[BNCSUI] unable to open [" + FileGameDLL + "]" );
	}

	return false;
}

bool CBNCSUtilInterface::HELP_SID_AUTH_ACCOUNTLOGON( )
{
	// set m_ClientKey

	char buf[32];
	// nls_get_A( (nls_t *)m_nls, buf );
    ( static_cast<NLS *>(m_NLS) )->getPublicKey( buf );
    m_ClientKey = UTIL_CreateByteArray( reinterpret_cast<unsigned char *>(buf), 32 );
	return true;
}

bool CBNCSUtilInterface::HELP_SID_AUTH_ACCOUNTLOGONPROOF( BYTEARRAY salt, BYTEARRAY serverKey )
{
	// set m_M1

	char buf[20];
    ( static_cast<NLS *>(m_NLS) )->getClientSessionKey( buf, string( salt.begin( ), salt.end( ) ).c_str( ), string( serverKey.begin( ), serverKey.end( ) ).c_str( ) );
    m_M1 = UTIL_CreateByteArray( reinterpret_cast<unsigned char *>(buf), 20 );
	return true;
}

bool CBNCSUtilInterface::HELP_PvPGNPasswordHash( string userPassword )
{
	// set m_PvPGNPasswordHash

	char buf[20];
	hashPassword( userPassword.c_str( ), buf );
    m_PvPGNPasswordHash = UTIL_CreateByteArray( reinterpret_cast<unsigned char *>(buf), 20 );
	return true;
}

BYTEARRAY CBNCSUtilInterface::CreateKeyInfo( string key, uint32_t clientToken, uint32_t serverToken )
{
	unsigned char Zeros[] = { 0, 0, 0, 0 };
	BYTEARRAY KeyInfo;
	CDKeyDecoder Decoder( key.c_str( ), key.size( ) );

	if( Decoder.isKeyValid( ) )
	{
        UTIL_AppendByteArray( KeyInfo, UTIL_CreateByteArray( static_cast<uint32_t>(key.size( )), false ) );
		UTIL_AppendByteArray( KeyInfo, UTIL_CreateByteArray( Decoder.getProduct( ), false ) );
		UTIL_AppendByteArray( KeyInfo, UTIL_CreateByteArray( Decoder.getVal1( ), false ) );
		UTIL_AppendByteArray( KeyInfo, UTIL_CreateByteArray( Zeros, 4 ) );
		size_t Length = Decoder.calculateHash( clientToken, serverToken );
		char *buf = new char[Length];
		Length = Decoder.getHash( buf );
        UTIL_AppendByteArray( KeyInfo, UTIL_CreateByteArray( reinterpret_cast<unsigned char *>(buf), Length ) );
		delete[] buf;
	}

	return KeyInfo;
}
