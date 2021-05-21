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

#ifndef UTIL_H
#define UTIL_H

// byte arrays

BYTEARRAY UTIL_CreateByteArray( char *a, unsigned int size );
BYTEARRAY UTIL_CreateByteArray( unsigned char *a, unsigned int size );
BYTEARRAY UTIL_CreateByteArray( unsigned char c );
BYTEARRAY UTIL_CreateByteArray( uint16_t i, bool reverse );
BYTEARRAY UTIL_CreateByteArray( uint32_t i, bool reverse );
uint16_t UTIL_ByteArrayToUInt16( BYTEARRAY b, bool reverse, unsigned int start = 0 );
uint32_t UTIL_ByteArrayToUInt32( BYTEARRAY b, bool reverse, unsigned int start = 0 );
std::string UTIL_ByteArrayToDecString( BYTEARRAY b );
std::string UTIL_ByteArrayToHexString( BYTEARRAY b );
void UTIL_AppendByteArray( BYTEARRAY &b, BYTEARRAY append );
void UTIL_AppendByteArrayFast( BYTEARRAY &b, BYTEARRAY &append );
void UTIL_AppendByteArray( BYTEARRAY &b, unsigned char *a, int size );
void UTIL_AppendByteArray( BYTEARRAY &b, std::string append, bool terminator = true );
void UTIL_AppendByteArrayFast( BYTEARRAY &b, std::string &append, bool terminator = true );
void UTIL_AppendByteArray( BYTEARRAY &b, uint16_t i, bool reverse );
void UTIL_AppendByteArray( BYTEARRAY &b, uint32_t i, bool reverse );
BYTEARRAY UTIL_ExtractCString( BYTEARRAY &b, unsigned int start );
unsigned char UTIL_ExtractHex( BYTEARRAY &b, unsigned int start, bool reverse );
void UTIL_ExtractStrings( std::string s, std::vector<std::string> &v );
void UTIL_AddStrings( std::vector<std::string> &dest, std::vector<std::string> sourc );
BYTEARRAY UTIL_ExtractNumbers( std::string s, unsigned int count );
BYTEARRAY UTIL_ExtractHexNumbers( std::string s );

// conversions

std::string UTIL_ToString( bool i );
std::string UTIL_ToString( unsigned long i );
QString UTIL_ToQString( unsigned long i );
std::string UTIL_ToString( unsigned short i );
std::string UTIL_ToString( unsigned int i );
QString UTIL_ToQString( unsigned int i );
std::string UTIL_ToString( long i );
QString UTIL_ToQString( long i );
std::string UTIL_ToString( short i );
std::string UTIL_ToString( int i );
QString UTIL_ToQString( int i );
std::string UTIL_ToString( float f, int digits = 0 );
std::string UTIL_ToString( double d, int digits = 0 );
QString UTIL_ToQString( double d, int digits = 0 );

#ifdef ULONG_MAX
#ifdef UINT64_MAX
#if ( ( ULONG_MAX ) != ( UINT64_MAX ) )
std::string UTIL_ToString( uint64_t i );
QString UTIL_ToQString( uint64_t i );
#endif
#endif
#endif

#ifdef ULONG_MAX
#ifdef UINT64_MAX
#if ( ( ULONG_MAX ) != ( UINT64_MAX ) )
std::string UTIL_ToString( int64_t i );
QString UTIL_ToQString( int64_t i );
#endif
#endif
#endif

std::string UTIL_MSToString( uint32_t ms );
std::string UTIL_ToBinaryString( uint32_t data );
std::string UTIL_ToHexString( uint32_t i );
bool UTIL_ToBool( std::string &s );
uint16_t UTIL_ToUInt16( std::string &s );
uint32_t UTIL_ToUInt32( std::string &s );
uint32_t UTIL_ToUInt32CopyValue( std::string s );
uint32_t UTIL_QSToUInt32( QString qs );
uint64_t UTIL_ToUInt64( std::string &s );
int16_t UTIL_ToInt16( std::string &s );
int32_t UTIL_ToInt32( std::string &s );
int32_t UTIL_ToInt32CopyValue( std::string s );
int32_t UTIL_QSToInt32( QString qs );
int64_t UTIL_ToInt64( std::string &s );
double UTIL_ToDouble( std::string &s );
unsigned long UTIL_ToULong( int i );
std::string UTIL_ENGToRUSOnlyLowerCase( std::string message );
std::string UTIL_RUSShort( std::string message );
std::string UTIL_StringToLower( std::string message );
std::string UTIL_Wc3ColorStringFix( std::string message );
std::string UTIL_MsgErrorCorrection( std::string message );
std::string UTIL_SubStrFix( std::string message, uint32_t start, uint32_t leng );
std::string UTIL_GetUserName( std::string message );
uint32_t UTIL_SizeStrFix( std::string message );
std::string UTIL_TriviaCloseLetter( std::string message );
std::string UTIL_TriviaOpenLetter( std::string cmessage, std::string omessage );
std::string UTIL_TriviaNewAnswerFormat( std::string message );
bool UTIL_StringSearch( std::string fullstring, std::string keystring, uint32_t type = 0 );
bool UTIL_SetVarInFile( std::string file, std::string var, std::string value );
bool UTIL_SetVarInFile( std::string file, std::string var, std::string value, std::string section );
bool UTIL_DelVarInFile( std::string file, std::string var );
uint32_t UTIL_CheckVarInFile( std::string file, std::vector<std::string> vars );
std::string UTIL_FixFileLine( std::string line, bool removespace = false );
uint32_t UTIL_GenerateRandomNumberUInt32( const uint32_t min, const uint32_t max );
uint64_t UTIL_GenerateRandomNumberUInt64( const uint64_t min, const uint64_t max );
std::string::size_type UTIL_GetPosOfLastLetter( std::string message );

#ifdef WIN32
std::string UTIL_UTF8ToCP1251( const char *str );
#endif

// files

bool UTIL_FileExists( std::string file );
int64_t UTIL_FileSize( std::string file, int64_t numberifabsent = 0 );
std::string UTIL_FileTime( std::string file );
bool UTIL_FileRename( std::string oldfilename, std::string newfilename );
bool UTIL_FileDelete( std::string filename );
std::string UTIL_FileRead( std::string file, uint32_t start, uint32_t length );
std::string UTIL_FileRead( std::string file );
bool UTIL_FileWrite( std::string file, unsigned char *data, uint32_t length );
std::string UTIL_FileSafeName( std::string fileName );
std::string UTIL_FixPath( std::string Path );
std::string UTIL_FixFilePath( std::string Path );
std::string UTIL_GetFileNameFromPath( std::string Path );
QString UTIL_GetFileNameFromPath( QString Path );

// stat strings

BYTEARRAY UTIL_EncodeStatString( BYTEARRAY &data );
BYTEARRAY UTIL_DecodeStatString( BYTEARRAY &data );

// other

bool UTIL_IsLanIP( BYTEARRAY ip );
bool UTIL_IsLocalIP( BYTEARRAY ip, std::vector<BYTEARRAY> &localIPs );
void UTIL_Replace( std::string &Text, std::string Key, std::string Value );
void UTIL_Replace2( std::string &Text, std::string Key, std::string Value );
char UTIL_FromHex( char ch );
std::string UTIL_GetMapNameFromUrl( std::string url );
std::string UTIL_UrlFix( std::string url );
std::string UTIL_UrlDecode( std::string text, bool url = true );
std::vector<std::string> UTIL_Tokenize( std::string s, char delim );
QImage UTIL_LoadTga( BYTEARRAY data, bool &success );
bool UTIL_CompareGameNames( std::string newgamename, std::string oldgamename );

// math

uint32_t UTIL_Factorial( uint32_t x );

#define nCr(n, r) (UTIL_Factorial(n) / UTIL_Factorial((n)-(r)) / UTIL_Factorial(r))
#define nPr(n, r) (UTIL_Factorial(n) / UTIL_Factorial((n)-(r)))

#endif
