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

#ifdef WIN32
#include <windows.h>
#endif

#include <wchar.h>
#include "unm.h"
#include "util.h"
#include <sys/stat.h>

using namespace std;

BYTEARRAY UTIL_CreateByteArray( char *a, unsigned int size )
{
	BYTEARRAY result( size );
	copy( a, a + size, result.begin( ) );
	return result;
}

BYTEARRAY UTIL_CreateByteArray( unsigned char *a, unsigned int size )
{
	BYTEARRAY result( size );
	copy( a, a + size, result.begin( ) );
	return result;
}

BYTEARRAY UTIL_CreateByteArray( unsigned char c )
{
	BYTEARRAY result( 1 );
	result[0] = c;
	return result;
}

BYTEARRAY UTIL_CreateByteArray( uint16_t i, bool reverse )
{
	BYTEARRAY result( 2 );

	if( reverse )
	{
		result[0] = static_cast<unsigned char>( i >> 8 );
        result[1] = static_cast<unsigned char>(i);
	}
	else
		*(uint16_t*)&result[0] = i;

	return result;
}

BYTEARRAY UTIL_CreateByteArray( uint32_t i, bool reverse )
{
	BYTEARRAY result( 4 );

	if( reverse )
	{
		result[0] = static_cast<unsigned char>( i >> 24 );
		result[1] = static_cast<unsigned char>( i >> 16 );
		result[2] = static_cast<unsigned char>( i >> 8 );
		result[3] = static_cast<unsigned char>( i );
	}
	else
		*(uint32_t*)&result[0] = i;

	return result;
}

uint16_t UTIL_ByteArrayToUInt16( BYTEARRAY b, bool reverse, unsigned int start )
{
	if( b.size( ) < start + 2 )
		return 0;

	BYTEARRAY temp = BYTEARRAY( b.begin( ) + start, b.begin( ) + start + 2 );

	if( reverse )
		temp = BYTEARRAY( temp.rbegin( ), temp.rend( ) );

	return (uint16_t)( temp[1] << 8 | temp[0] );
}

uint32_t UTIL_ByteArrayToUInt32( BYTEARRAY b, bool reverse, unsigned int start )
{
	if( b.size( ) < start + 4 )
		return 0;

	BYTEARRAY temp = BYTEARRAY( b.begin( ) + start, b.begin( ) + start + 4 );

	if( reverse )
		temp = BYTEARRAY( temp.rbegin( ), temp.rend( ) );

	return (uint32_t)( temp[3] << 24 | temp[2] << 16 | temp[1] << 8 | temp[0] );
}

string UTIL_ByteArrayToDecString( BYTEARRAY b )
{
	if( b.empty( ) )
		return string( );

	string result = UTIL_ToString( b[0] );

	for( BYTEARRAY::iterator i = b.begin( ) + 1; i != b.end( ); i++ )
		result += " " + UTIL_ToString( *i );

	return result;
}

string UTIL_ByteArrayToHexString( BYTEARRAY b )
{
	if( b.empty( ) )
		return string( );

	string result = UTIL_ToHexString( b[0] );

	for( BYTEARRAY::iterator i = b.begin( ) + 1; i != b.end( ); i++ )
        result += " " + UTIL_ToHexString( *i );

	return result;
}

void UTIL_AppendByteArray( BYTEARRAY &b, BYTEARRAY append )
{
	b.insert( b.end( ), append.begin( ), append.end( ) );
}

void UTIL_AppendByteArrayFast( BYTEARRAY &b, BYTEARRAY &append )
{
	b.insert( b.end( ), append.begin( ), append.end( ) );
}

void UTIL_AppendByteArray( BYTEARRAY &b, unsigned char *a, int size )
{
	UTIL_AppendByteArray( b, UTIL_CreateByteArray( a, (unsigned int)size ) );
}

void UTIL_AppendByteArray( BYTEARRAY &b, string append, bool terminator )
{
	// append the string plus a null terminator

	b.insert( b.end( ), append.begin( ), append.end( ) );

	if( terminator )
		b.push_back( 0 );
}

void UTIL_AppendByteArrayFast( BYTEARRAY &b, string &append, bool terminator )
{
	// append the string plus a null terminator

	b.insert( b.end( ), append.begin( ), append.end( ) );

	if( terminator )
		b.push_back( 0 );
}

void UTIL_AppendByteArray( BYTEARRAY &b, uint16_t i, bool reverse )
{
	UTIL_AppendByteArray( b, UTIL_CreateByteArray( i, reverse ) );
}

void UTIL_AppendByteArray( BYTEARRAY &b, uint32_t i, bool reverse )
{
	UTIL_AppendByteArray( b, UTIL_CreateByteArray( i, reverse ) );
}

BYTEARRAY UTIL_ExtractCString( BYTEARRAY &b, unsigned int start )
{
	// start searching the byte array at position 'start' for the first null value
	// if found, return the subarray from 'start' to the null value but not including the null value

	if( start < b.size( ) )
	{
		for( uint32_t i = start; i < b.size( ); i++ )
		{
			if( b[i] == 0 )
				return BYTEARRAY( b.begin( ) + start, b.begin( ) + i );
		}

		// no null value found, return the rest of the byte array

		return BYTEARRAY( b.begin( ) + start, b.end( ) );
	}

	return BYTEARRAY( );
}

unsigned char UTIL_ExtractHex( BYTEARRAY &b, unsigned int start, bool reverse )
{
	// consider the byte array to contain a 2 character ASCII encoded hex value at b[start] and b[start + 1] e.g. "FF"
	// extract it as a single decoded byte

	if( start + 1 < b.size( ) )
	{
		unsigned int c;
		string temp = string( b.begin( ) + start, b.begin( ) + start + 2 );

		if( reverse )
			temp = string( temp.rend( ), temp.rbegin( ) );

		stringstream SS;
		SS << temp;
		SS >> hex >> c;
        return static_cast<unsigned char>(c);
	}

	return 0;
}

void UTIL_AddStrings( vector<string> &dest, vector<string> sourc )
{
	bool n;

	for( uint32_t o = 0; o < sourc.size( ); o++ )
	{
		n = true;

		for( uint32_t p = 0; p < dest.size( ); p++ )
		{
			if( sourc[o] == dest[p] )
				n = false;
		}

		if( n )
			dest.push_back( sourc[o] );
	}
}

void UTIL_ExtractStrings( string s, vector<string> &v )
{
	// consider the string to contain strings, e.g. "one two three"
	v.clear( );
	string istr;
	stringstream SS;
	SS << s;
	while( SS >> istr ) v.push_back( istr );

	return;
}

BYTEARRAY UTIL_ExtractNumbers( string s, unsigned int count )
{
	// consider the string to contain a bytearray in dec-text form, e.g. "52 99 128 1"

	BYTEARRAY result;
	unsigned int c;
	stringstream SS;
	SS << s;

	for( uint32_t i = 0; i < count; i++ )
	{
		if( SS.eof( ) )
			break;

		SS >> c;

		// todotodo: if c > 255 handle the error instead of truncating

        result.push_back( static_cast<unsigned char>(c) );
	}

	return result;
}

BYTEARRAY UTIL_ExtractHexNumbers( string s )
{
	// consider the string to contain a bytearray in hex-text form, e.g. "4e 17 b7 e6"

	BYTEARRAY result;
	unsigned int c;
	stringstream SS;
	SS << s;

	while( !SS.eof( ) )
	{
		SS >> hex >> c;

		// todotodo: if c > 255 handle the error instead of truncating

        result.push_back( static_cast<unsigned char>(c) );
	}

	return result;
}

string UTIL_ToString( bool i )
{
    if( i )
        return "1";
    else
        return "0";
}

string UTIL_ToString( unsigned long i )
{
	string result;
	stringstream SS;
	SS << i;
	SS >> result;
	return result;
}

QString UTIL_ToQString( unsigned long i )
{
    string result;
    stringstream SS;
    SS << i;
    SS >> result;
    return QString::fromUtf8( result.c_str( ) );
}

string UTIL_ToString( unsigned short i )
{
	string result;
	stringstream SS;
	SS << i;
	SS >> result;
	return result;
}

string UTIL_ToString( unsigned int i )
{
	string result;
	stringstream SS;
	SS << i;
	SS >> result;
	return result;
}

QString UTIL_ToQString( unsigned int i )
{
    string result;
    stringstream SS;
    SS << i;
    SS >> result;
    return QString::fromUtf8( result.c_str( ) );
}

string UTIL_ToString( long i )
{
	string result;
	stringstream SS;
	SS << i;
	SS >> result;
	return result;
}

QString UTIL_ToQString( long i )
{
    string result;
    stringstream SS;
    SS << i;
    SS >> result;
    return QString::fromUtf8( result.c_str( ) );
}

string UTIL_ToString( short i )
{
	string result;
	stringstream SS;
	SS << i;
	SS >> result;
	return result;
}

string UTIL_ToString( int i )
{
	string result;
	stringstream SS;
	SS << i;
	SS >> result;
	return result;
}

QString UTIL_ToQString( int i )
{
    string result;
    stringstream SS;
    SS << i;
    SS >> result;
    return QString::fromUtf8( result.c_str( ) );
}

string UTIL_ToString( float f, int digits )
{
	string result;
	stringstream SS;
	SS << std::fixed << std::setprecision( digits ) << f;
	SS >> result;
	return result;
}

string UTIL_ToString( double d, int digits )
{
    string result;
    stringstream SS;
    SS << std::fixed << std::setprecision( digits ) << d;
    SS >> result;
    return result;
}

QString UTIL_ToQString( double d, int digits )
{
    string result;
    stringstream SS;
    SS << std::fixed << std::setprecision( digits ) << d;
    SS >> result;
    return QString::fromUtf8( result.c_str( ) );
}

#ifdef ULONG_MAX
#ifdef UINT64_MAX
#if ( ( ULONG_MAX ) != ( UINT64_MAX ) )
string UTIL_ToString( uint64_t i )
{
	string result;
	stringstream SS;
	SS << i;
	SS >> result;
	return result;
}

QString UTIL_ToQString( uint64_t i )
{
    string result;
    stringstream SS;
    SS << i;
    SS >> result;
    return QString::fromUtf8( result.c_str( ) );
}
#endif
#endif
#endif

#ifdef LONG_MAX
#ifdef INT64_MAX
#if ( ( LONG_MAX ) != ( INT64_MAX ) )
string UTIL_ToString( int64_t i )
{
	string result;
	stringstream SS;
	SS << i;
	SS >> result;
	return result;
}

QString UTIL_ToQString( int64_t i )
{
    string result;
    stringstream SS;
    SS << i;
    SS >> result;
    return QString::fromUtf8( result.c_str( ) );
}
#endif
#endif
#endif

string UTIL_ToHexString( uint32_t i )
{
	string result;
	stringstream SS;
	SS << std::hex << i;
	SS >> result;
	return result;
}

#ifdef WIN32
string UTIL_UTF8ToCP1251( const char *str )
{
	string res;
	WCHAR *ures = nullptr;
	char *cres = nullptr;
	int result_u = MultiByteToWideChar( CP_UTF8, 0, str, -1, 0, 0 );

	if( result_u != 0 )
	{
		ures = new WCHAR[(unsigned int)result_u];

		if( MultiByteToWideChar( CP_UTF8, 0, str, -1, ures, result_u ) )
		{
			int result_c = WideCharToMultiByte( 1251, 0, ures, -1, 0, 0, 0, 0 );

			if( result_c != 0 )
			{
				cres = new char[(unsigned int)result_c];

				if( WideCharToMultiByte( 1251, 0, ures, -1, cres, result_c, 0, 0 ) )
					res = cres;
			}
		}
    }

	delete[] ures;
	delete[] cres;
	return res;
}
#endif

string UTIL_ENGToRUSOnlyLowerCase( string message )
{
	string Key[37] = { "e'","yo","i'","zh","ch","sh","sch","ju","ja","iu","ia","yu","ya","a","b","v","g","d","e","z","i","j","k","l","m","n","o","p","r","s","t","u","f","h","c","y","'" };
	string Value[37] = { "э","ё","ы","ж","ч","ш","щ","ю","я","ю","я","ю","я","а","б","в","г","д","е","з","и","й","к","л","м","н","о","п","р","с","т","у","ф","х","ц","ы","ь" };
	string::size_type KeyStart = string::npos;

	for( uint32_t i = 0; i <= 36; i++ )
	{
		KeyStart = message.find( Key[i] );

		while( KeyStart != string::npos )
		{
			message.replace( KeyStart, Key[i].size( ), Value[i] );
			KeyStart = message.find( Key[i] );
		}
	}

	return message;
}

string UTIL_RUSShort( string message )
{
	string Key[17] = { "А","В","Е","К","М","Н","О","Р","С","Т","Х","а","е","о","р","с","х" };
	string Value[17] = { "A","B","E","K","M","H","O","P","C","T","X","a","e","o","p","c","x" };
	string::size_type KeyStart = string::npos;

	for( uint32_t i = 0; i <= 16; i++ )
	{
		KeyStart = message.find( Key[i] );

		while( KeyStart != string::npos )
		{
			message.replace( KeyStart, 2, Value[i] );
			KeyStart = message.find( Key[i] );
		}
	}

	return message;
}

string UTIL_StringToLower( string message )
{
    QString str = QString::fromUtf8( message.c_str( ) );
    str = str.toLower( );
    message = str.toStdString( );
    return message;
}

string UTIL_Wc3ColorStringFix( string message )
{
    string::size_type MessageFind = message.find( "|c" );

    while( MessageFind != string::npos && MessageFind + 10 <= message.size( ) )
    {
        message = message.substr( 0, MessageFind ) + "<font color = #" + message.substr( MessageFind + 4, 6 ) + ">" + message.substr( MessageFind + 10 );
        MessageFind = message.find( "|c" );
    }

    MessageFind = message.find( "|C" );

    while( MessageFind != string::npos && MessageFind + 10 <= message.size( ) )
    {
        message = message.substr( 0, MessageFind ) + "<font color = #" + message.substr( MessageFind + 4, 6 ) + ">" + message.substr( MessageFind + 10 );
        MessageFind = message.find( "|C" );
    }

    MessageFind = message.find( "|r" );

    while( MessageFind != string::npos )
    {
        message = message.substr( 0, MessageFind ) + "</font>" + message.substr( MessageFind + 2 );
        MessageFind = message.find( "|r" );
    }

    MessageFind = message.find( "|R" );

    while( MessageFind != string::npos )
    {
        message = message.substr( 0, MessageFind ) + "</font>" + message.substr( MessageFind + 2 );
        MessageFind = message.find( "|R" );
    }

    return message;
}

string UTIL_MsgErrorCorrection( string message )
{
	UTIL_Replace( message, " нада", " надо" );
	UTIL_Replace( message, " дила", " дела" );
	UTIL_Replace( message, " шо ", " что " );
	UTIL_Replace( message, "пашел", "пошел" );
	UTIL_Replace( message, "пашол", "пошел" );
	UTIL_Replace( message, "пошол", "пошел" );
	UTIL_Replace( message, "прашел", "прошел" );
	UTIL_Replace( message, "прашол", "прошел" );
	UTIL_Replace( message, "прошол", "прошел" );
	UTIL_Replace( message, "будеш", "будешь" );
	UTIL_Replace( message, "будешьь", "будешь" );
	UTIL_Replace( message, "будиш", "будишь" );
	UTIL_Replace( message, "будишьь", "будишь" );
	UTIL_Replace( message, "видеш", "видиш" );
	UTIL_Replace( message, "видиш", "видишь" );
	UTIL_Replace( message, "видишьь", "видишь" );
	UTIL_Replace( message, "ненадо", "не надо" );
	UTIL_Replace( message, "непошел", "не пошел" );
	UTIL_Replace( message, "непрошел", "не прошел" );
	UTIL_Replace( message, "небудешь", "не будешь" );
	UTIL_Replace( message, "небудишь", "не будишь" );
	UTIL_Replace( message, "невидишь", "не видишь" );
	UTIL_Replace( message, "нинадо", "не надо" );
	UTIL_Replace( message, "нипошел", "не пошел" );
	UTIL_Replace( message, "нипрошел", "не прошел" );
	UTIL_Replace( message, "нибудешь", "не будешь" );
	UTIL_Replace( message, "нибудишь", "не будишь" );
	UTIL_Replace( message, "нивидишь", "не видишь" );
	UTIL_Replace( message, "извени", "извини" );
	UTIL_Replace( message, " нету", " нет" );
	UTIL_Replace( message, "знаеш", "знаешь" );
	UTIL_Replace( message, "знаиш", "знаешь" );
	UTIL_Replace( message, "знаешьь", "знаешь" );
	UTIL_Replace( message, "незнаешь", "не знаешь" );
	UTIL_Replace( message, "низнаешь", "не знаешь" );
	UTIL_Replace( message, "пажалуйста", "пожалуйста" );
	UTIL_Replace( message, "спосибо", "спасибо" );
	UTIL_Replace( message, "пачиму", "почему" );
	UTIL_Replace( message, "почиму", "почему" );
	UTIL_Replace( message, "пачему", "почему" );
	UTIL_Replace( message, "выйграл", "выиграл" );
	UTIL_Replace( message, "честна", "честно" );
	UTIL_Replace( message, "чесна", "честно" );
	UTIL_Replace( message, "чесно ", "честно " );
	return message;
}

string UTIL_SubStrFix( string message, uint32_t start, uint32_t leng )
{
	if( message.empty( ) )
		return string( );
	else if( leng == 0 )
		leng = message.size( );

	uint32_t c;
	uint32_t i = 0;
	uint32_t ix = message.length( );
	uint32_t min = 0;
	uint32_t max = string::npos;

	if( start == 0 )
		min = 0;

	while( i < ix )
	{
        c = static_cast<unsigned char>(message[i]);

		if( c <= 127 )
			i += 1;
		else if( ( c & 0xE0 ) == 0xC0 )
			i += 2;
		else if( ( c & 0xF0 ) == 0xE0 )
			i += 3;
//		else if( ( c & 0xF8 ) == 0xF0 )
//			i += 4;
		else
		{
			message.erase( i, 1 );
			ix = message.length( );

			if( start > 0 )
				start--;
		}

		if( i <= start )
			min = i;

		if( leng == string::npos || i <= min + leng )
			max = i;
		else
			break;
	}

	if( leng == string::npos || i < min + leng )
		max = i;

	if( max == string::npos || max == 0 )
		return string( );

	return message.substr( min, max - min );
}

string UTIL_GetUserName( string message )
{
	string UserName = message;
	string::size_type UserNameStart = UserName.find( " " );

	if( UserNameStart != string::npos )
	{
		UserName = UserName.substr( UserNameStart + 1 );

		if( UserName.find_first_not_of( " " ) != string::npos )
		{
            while( UserName.size( ) > 0 && UserName.front( ) == ' ' )
                UserName.erase( UserName.begin( ) );

			string::size_type UserNameEnd = UserName.find( " " );

			if( UserNameEnd != string::npos )
				UserName = UserName.substr( 0, UserNameEnd );
		}
		else
			UserName = message;
	}

	return UserName;
}

uint32_t UTIL_SizeStrFix( string message )
{
	uint32_t i = 0;
    uint32_t n = 0;
    QString qmessage = QString::fromUtf8( message.c_str( ) );

    while( i < qmessage.size( ) )
    {
        if( qmessage[i] != ' ' && qmessage[i] != '-' && qmessage[i] != ',' && qmessage[i] != '.' && qmessage[i] != ':' && qmessage[i] != '!' && qmessage[i] != '?' && qmessage[i] != '"' && qmessage[i] != '\'' && qmessage[i] != ';' && qmessage[i] != '\\' && qmessage[i] != '/' )
            n++;

        i++;
    }

    return n;
}

string UTIL_TriviaCloseLetter( string message )
{
    QString qmessage = QString::fromUtf8( message.c_str( ) );
	string nmessage = string( );
	uint32_t i = 0;

    while( i < qmessage.size( ) )
	{
        if( qmessage[i] == ' ' )
            nmessage += " ";
        else if( qmessage[i] == '-' )
            nmessage += "-";
        else if( qmessage[i] == ',' )
            nmessage += ",";
        else if( qmessage[i] == '.' )
            nmessage += ".";
        else if( qmessage[i] == ':' )
            nmessage += ":";
        else if( qmessage[i] == '!' )
            nmessage += "!";
        else if( qmessage[i] == '?' )
            nmessage += "?";
        else if( qmessage[i] == '"' )
            nmessage += "\"";
        else if( qmessage[i] == '\'' )
            nmessage += "'";
        else if( qmessage[i] == ';' )
            nmessage += ";";
        else if( qmessage[i] == '\\' )
            nmessage += "\\";
        else if( qmessage[i] == '/' )
            nmessage += "/";
		else
			nmessage += "*";

		i++;
	}

	return nmessage;
}

string UTIL_TriviaOpenLetter( string cmessage, string omessage )
{
    QString qcmessage = QString::fromUtf8( cmessage.c_str( ) );
    QString qomessage = QString::fromUtf8( omessage.c_str( ) );
    QString qnmessage = QString( );
	uint32_t i = 0;
	uint32_t n = 0;

    while( i < qcmessage.size( ) )
	{
        if( qcmessage[i] == '*' )
			n++;

		i++;
	}

    uint32_t randomletter = UTIL_GenerateRandomNumberUInt32( 0, n - 1 );;
	i = 0;
	n = 0;

    while( i < qomessage.size( ) )
	{
        if( qcmessage[i] == '*' )
		{
			if( n == randomletter )
                qnmessage += qomessage[i];
			else
                qnmessage += "*";

			n++;
		}
		else
            qnmessage += qcmessage[i];

		i++;
	}

    string nmessage = qnmessage.toStdString( );
	return nmessage;
}

string UTIL_TriviaNewAnswerFormat( string message )
{
    QString qmessage = QString::fromUtf8( message.c_str( ) );
    QString qnmessage = QString( );
	uint32_t i = 0;
	uint32_t n = 0;

    while( i < qmessage.size( ) )
	{
        if( i > 0 && ( n == 0 || qmessage[i] == '_' || qmessage[i] == '*' || ( n == 1 && ( qmessage[i] == '-' || qmessage[i] == ',' || qmessage[i] == '.' || qmessage[i] == ':' || qmessage[i] == '!' || qmessage[i] == '?' || qmessage[i] == '"' || qmessage[i] == '\'' || qmessage[i] == ";" || qmessage[i] == '\\' || qmessage[i] == '/' ) ) ) )
            qnmessage += " ";

        if( qmessage[i] == '*' || qmessage[i] == '_' )
            qnmessage += "_";
		else
            qnmessage += qmessage[i];

        if( qmessage[i] == '-' || qmessage[i] == ',' || qmessage[i] == '.' || qmessage[i] == ':' || qmessage[i] == '!' || qmessage[i] == '?' || qmessage[i] == '"' || qmessage[i] == '\'' || qmessage[i] == ';' || qmessage[i] == '\\' || qmessage[i] == '/' )
			n = 1;
        else if( qmessage[i] == '_' || qmessage[i] == '*' )
			n = 0;
		else
			n = 2;

		i++;
	}

    string nmessage = qnmessage.toStdString( );
    return nmessage;
}

bool UTIL_StringSearch( string fullstring, string keystring, uint32_t type )
{
    transform( fullstring.begin( ), fullstring.end( ), fullstring.begin( ), static_cast<int(*)(int)>(tolower) );
    transform( keystring.begin( ), keystring.end( ), keystring.begin( ), static_cast<int(*)(int)>(tolower) );

    if( type == 0 )
    {
        fullstring = " "+fullstring+" ";
        keystring = " "+keystring+" ";
    }
    else if( type == 1 )
    {
        fullstring = ", "+fullstring+", ";
        keystring = ", "+keystring+", ";
    }

    if( fullstring.find( keystring ) != string::npos )
        return true;
    else
        return false;
}

bool UTIL_SetVarInFile( string file, string var, string value )
{
    // read file

    string Line;
    vector<string> AllLines;
    ifstream in;
    int32_t VarsStart = 0;
    bool VarsStartFound = false;
    bool varfound = false;

#ifdef WIN32
    in.open( UTIL_UTF8ToCP1251( file.c_str( ) ) );
#else
    in.open( file );
#endif

    if( !in.fail( ) )
    {
        while( !in.eof( ) )
        {
            getline( in, Line );
            Line = UTIL_FixFileLine( Line );

            if( !VarsStartFound )
            {
                if( Line.find_first_not_of( " #" ) == string::npos )
                    VarsStart++;
                else if( Line.find( "#####" ) != string::npos )
                    VarsStart++;
                else if( Line.size( ) > 3 && Line.substr( 0, 2 ) == "# " && Line.substr( Line.size( ) - 2, 2 ) == " #" )
                    VarsStart++;
                else
                    VarsStartFound = true;
            }

            if( Line.substr( 0, var.size( ) ) == var )
            {
                if( !varfound )
                {
                    AllLines.push_back( var + " = " + value );
                    varfound = true;
                }
            }
            else
                AllLines.push_back( Line );
        }

        in.close( );
    }
    else
    {
        in.close( );
        return false;
    }

    if( !varfound )
        AllLines.insert( AllLines.begin( ) + VarsStart, var + " = " + value );

    while( AllLines.size( ) > 0 && AllLines.back( ).empty( ) )
        AllLines.erase( AllLines.begin( ) + static_cast<int32_t>( AllLines.size( ) - 1 ) );

    // writing new file

    ofstream newfile;

#ifdef WIN32
    newfile.open( UTIL_UTF8ToCP1251( file.c_str( ) ), ios::trunc );
#else
    newfile.open( file, ios::trunc );
#endif

    for( uint32_t n = 0; n < AllLines.size( ); n++ )
        newfile << AllLines[n] << endl;

    newfile.close( );
    return true;
}

bool UTIL_SetVarInFile( string file, string var, string value, string section )
{
    // read file

    string Line;
    vector<string> AllLines;
    ifstream in;
    int32_t VarsStart = 0;
    bool VarsStartFound = false;
    bool SectionFound = false;
    bool NextSectionFound = false;
    bool varfound = false;

#ifdef WIN32
    in.open( UTIL_UTF8ToCP1251( file.c_str( ) ) );
#else
    in.open( file );
#endif

    if( !in.fail( ) )
    {
        while( !in.eof( ) )
        {
            getline( in, Line );
            Line = UTIL_FixFileLine( Line );

            if( !SectionFound )
            {
                VarsStart++;

                if( Line.size( ) >= section.size( ) + 2 && Line.substr( 0, section.size( ) + 2 ) == "[" + section + "]" )
                    SectionFound = true;

                AllLines.push_back( Line );
            }
            else
            {
                if( !NextSectionFound && Line.front( ) == '[' )
                {
                    string::size_type endpos = Line.find_last_not_of( " " );

                    if( endpos != string::npos && Line[endpos] == ']' )
                        NextSectionFound = true;
                }

                if( !NextSectionFound )
                {
                    if( !VarsStartFound )
                    {
                        if( Line.find_first_not_of( " #" ) == string::npos )
                            VarsStart++;
                        else
                            VarsStartFound = true;
                    }

                    if( Line.substr( 0, var.size( ) ) == var )
                    {
                        if( !varfound )
                        {
                            AllLines.push_back( var + " = " + value );
                            varfound = true;
                        }
                    }
                    else
                        AllLines.push_back( Line );
                }
                else
                    AllLines.push_back( Line );
            }
        }

        in.close( );
    }
    else
    {
        in.close( );
        return false;
    }

    if( !varfound )
        AllLines.insert( AllLines.begin( ) + VarsStart, var + " = " + value );

    while( AllLines.size( ) > 0 && AllLines.back( ).empty( ) )
        AllLines.erase( AllLines.begin( ) + static_cast<int32_t>( AllLines.size( ) - 1 ) );

    // writing new file

    ofstream newfile;

#ifdef WIN32
    newfile.open( UTIL_UTF8ToCP1251( file.c_str( ) ), ios::trunc );
#else
    newfile.open( file, ios::trunc );
#endif

    for( uint32_t n = 0; n < AllLines.size( ); n++ )
        newfile << AllLines[n] << endl;

    newfile.close( );
    return true;
}


bool UTIL_DelVarInFile( string file, string var )
{
    // read file

    string Line;
    vector<string> AllLines;
    ifstream in;

#ifdef WIN32
    in.open( UTIL_UTF8ToCP1251( file.c_str( ) ) );
#else
    in.open( file );
#endif

    if( !in.fail( ) )
    {
        while( !in.eof( ) )
        {
            getline( in, Line );
            Line = UTIL_FixFileLine( Line );

            if( Line.substr( 0, var.size( ) ) != var )
                AllLines.push_back( Line );
        }

        in.close( );
    }
    else
    {
        in.close( );
        return false;
    }

    while( !AllLines.empty( ) && AllLines.back( ).empty( ) )
        AllLines.erase( AllLines.end( ) - 1 );

    // writing new file

    ofstream newfile;

#ifdef WIN32
    newfile.open( UTIL_UTF8ToCP1251( file.c_str( ) ), ios::trunc );
#else
    newfile.open( file, ios::trunc );
#endif

    for( uint32_t n = 0; n < AllLines.size( ); n++ )
        newfile << AllLines[n] << endl;

    newfile.close( );
    return true;
}

uint32_t UTIL_CheckVarInFile( string file, vector<string> vars )
{
    // эта функция проверяет конфигурационный файл на наличие заданных кваров
    // функция принимает полный путь/название конфигурационного файла и список кваров, наличие которых нужно проверить
    // функция возвращает количество найденных совпадений

    string Line;
    ifstream in;
    uint32_t cvarsfoundcount = 0;

#ifdef WIN32
    in.open( UTIL_UTF8ToCP1251( file.c_str( ) ) );
#else
    in.open( file );
#endif

    if( !in.fail( ) )
    {
        while( !in.eof( ) )
        {
            getline( in, Line );
            Line = UTIL_FixFileLine( Line );

            for( uint32_t i = 0; i < vars.size( ); i++ )
            {
                if( Line.substr( 0, vars[i].size( ) ) == vars[i] )
                {
                    cvarsfoundcount++;
                    break;
                }
            }
        }
    }

    in.close( );
    return cvarsfoundcount;
}

bool UTIL_ToBool( string &s )
{
    if( s == "0" )
        return false;
    else if( s == "1" )
        return true;
    else
    {
        CONSOLE_Print( "[DEBUG] Converting string \"" + s + "\" to bool as \"true\"" );
        return true;
    }
}

uint16_t UTIL_ToUInt16( string &s )
{
	uint16_t result;
	stringstream SS;
	SS << s;
    SS >> result;

	if( SS.fail( ) )
		return 0;
	else
		return result;
}

uint32_t UTIL_ToUInt32( string &s )
{
	uint32_t result;
	stringstream SS;
	SS << s;
    SS >> result;

	if( SS.fail( ) )
		return 0;
	else
		return result;
}

uint32_t UTIL_ToUInt32CopyValue( string s )
{
    uint32_t result;
    stringstream SS;
    SS << s;
    SS >> result;

    if( SS.fail( ) )
        return 0;
    else
        return result;
}

uint32_t UTIL_QSToUInt32( QString qs )
{
    string s = qs.toStdString( );
    uint32_t result;
    stringstream SS;
    SS << s;
    SS >> result;

    if( SS.fail( ) )
        return 0;
    else
        return result;
}

uint64_t UTIL_ToUInt64( string &s )
{
	uint64_t result;
	stringstream SS;
	SS << s;
    SS >> result;

	if( SS.fail( ) )
		return 0;
	else
		return result;
}

int16_t UTIL_ToInt16( string &s )
{
	int16_t result;
	stringstream SS;
	SS << s;
    SS >> result;

	if( SS.fail( ) )
		return 0;
	else
		return result;
}

int32_t UTIL_ToInt32( string &s )
{
	int32_t result;
	stringstream SS;
	SS << s;
    SS >> result;

	if( SS.fail( ) )
		return 0;
	else
		return result;
}

int32_t UTIL_ToInt32CopyValue( string s )
{
    int32_t result;
    stringstream SS;
    SS << s;
    SS >> result;

    if( SS.fail( ) )
        return 0;
    else
        return result;
}

int32_t UTIL_QSToInt32( QString qs )
{
    string s = qs.toStdString( );
    int32_t result;
    stringstream SS;
    SS << s;
    SS >> result;

    if( SS.fail( ) )
        return 0;
    else
        return result;
}

int64_t UTIL_ToInt64( string &s )
{
	int64_t result;
	stringstream SS;
	SS << s;
    SS >> result;

	if( SS.fail( ) )
		return 0;
	else
		return result;
}

double UTIL_ToDouble( string &s )
{
	double result;
	stringstream SS;
	SS << s;
    SS >> result;

	if( SS.fail( ) )
		return 0;
	else
		return result;
}

unsigned long UTIL_ToULong( int i )
{
    unsigned long result = (unsigned long)i;

	if( i < 0 )
        result += 256;

	return result;
}

string UTIL_MSToString( uint32_t ms )
{
	string MinString = UTIL_ToString( ( ms / 1000 ) / 60 );
	string SecString = UTIL_ToString( ( ms / 1000 ) % 60 );

	if( MinString.size( ) == 1 )
		MinString.insert( 0, "0" );

	if( SecString.size( ) == 1 )
		SecString.insert( 0, "0" );

	return MinString + "m" + SecString + "s";
}

bool UTIL_FileExists( string file )
{
    std::wstring Wfile = std::wstring_convert<std::codecvt_utf8<wchar_t>>( ).from_bytes(file);
    return std::filesystem::exists( Wfile );
}

int64_t UTIL_FileSize( string file, int64_t numberifabsent )
{
#ifdef WIN32
    file = UTIL_UTF8ToCP1251( file.c_str( ) );
#endif

	struct stat fileinfo;

	if( stat( file.c_str( ), &fileinfo ) == 0 )
		return fileinfo.st_size;
	else
		return numberifabsent;
}

string UTIL_FileTime( string file )
{
#ifdef WIN32
    file = UTIL_UTF8ToCP1251( file.c_str( ) );
#endif

	WIN32_FILE_ATTRIBUTE_DATA ad;
	wstring wfile( file.begin( ), file.end( ) );
	LPCWSTR lpcwfile = wfile.c_str( );
	string FileTime = string( );

	if( GetFileAttributesEx( lpcwfile, GetFileExInfoStandard, &ad ) == TRUE )
	{
		char str[32];
		FILETIME ftWrite;
		SYSTEMTIME stUTC;
		ftWrite = ad.ftLastWriteTime;
		FileTimeToSystemTime( &ftWrite, &stUTC );
		sprintf( str, "%02d/%02d/%d  %02d:%02d", stUTC.wDay, stUTC.wMonth, stUTC.wYear, stUTC.wHour, stUTC.wMinute );
		FileTime = string( str );
	}

	return FileTime;
}

bool UTIL_FileRename( string oldfilename, string newfilename )
{
    std::wstring Woldfilename = std::wstring_convert<std::codecvt_utf8<wchar_t>>().from_bytes(oldfilename);
    std::wstring Wnewfilename = std::wstring_convert<std::codecvt_utf8<wchar_t>>().from_bytes(newfilename);

    if( _wrename( Woldfilename.c_str( ), Wnewfilename.c_str( ) ) == 0 )
		return true;

	return false;
}

bool UTIL_FileDelete( string filename )
{
    std::wstring Wfilename = std::wstring_convert<std::codecvt_utf8<wchar_t>>().from_bytes(filename);

    if( _wremove( Wfilename.c_str( ) ) == 0 )
        return true;

    return false;
}

string UTIL_FileRead( string file, uint32_t start, uint32_t length )
{
    ifstream IS;
    std::wstring Wfile = std::wstring_convert<std::codecvt_utf8<wchar_t>>().from_bytes(file);
    IS.open( Wfile.c_str( ), ios::binary );

	if( IS.fail( ) )
	{
		CONSOLE_Print( "[UTIL] warning - unable to read file part [" + file + "]" );
		return string( );
	}

	// get length of file

	IS.seekg( 0, ios::end );
	uint32_t FileLength = (uint32_t)IS.tellg( );

	if( start > FileLength )
	{
		IS.close( );
		return string( );
	}

	IS.seekg( start, ios::beg );

	// read data

	char *Buffer = new char[length];
	IS.read( Buffer, length );
	string BufferString = string( Buffer, (unsigned int)IS.gcount( ) );
	IS.close( );
	delete[] Buffer;
	return BufferString;
}

string UTIL_FileRead( string file )
{
	ifstream IS;
    std::wstring Wfile = std::wstring_convert<std::codecvt_utf8<wchar_t>>().from_bytes(file);
    IS.open( Wfile.c_str( ), ios::binary );

	if( IS.fail( ) )
	{
		CONSOLE_Print( "[UTIL] warning - unable to read file [" + file + "]" );
		return string( );
	}

	// get length of file

	IS.seekg( 0, ios::end );
	uint32_t FileLength = (uint32_t)IS.tellg( );
	IS.seekg( 0, ios::beg );

	// read data

	char *Buffer = new char[FileLength];
	IS.read( Buffer, FileLength );
	string BufferString = string( Buffer, (unsigned int)IS.gcount( ) );
	IS.close( );
	delete[] Buffer;

	if( BufferString.size( ) == FileLength )
		return BufferString;
	else
		return string( );
}

bool UTIL_FileWrite( string file, unsigned char *data, uint32_t length )
{
	ofstream OS;

#ifdef WIN32
    OS.open( UTIL_UTF8ToCP1251( file.c_str( ) ), ios::binary );
#else
    OS.open( file, ios::binary );
#endif

	if( OS.fail( ) )
	{
		CONSOLE_Print( "[UTIL] warning - unable to write file [" + file + "]" );
		return false;
	}

	// write data

	OS.write( (const char *)data, length );
	OS.close( );
	return true;
}

string UTIL_FileSafeName( string fileName )
{
	string::size_type BadStart = fileName.find_first_of( "\\/:*?<>|" );

	while( BadStart != string::npos )
	{
		fileName.replace( BadStart, 1, 1, '_' );
		BadStart = fileName.find_first_of( "\\/:*?<>|" );
	}

	return fileName;
}

string UTIL_FixPath( string Path )
{
#ifdef WIN32
    while( Path.find( "/" ) != string::npos )
        UTIL_Replace( Path, "/", "\\" );

    if( !Path.empty( ) && Path.substr( Path.size( ) - 1 ) != "\\" )
        Path += "\\";
#else
    while( Path.find( "\\" ) != string::npos )
        UTIL_Replace( Path, "\\", "/" );

    if( !Path.empty( ) && Path.substr( Path.size( ) - 1 ) != "/" )
        Path += "/";
#endif

    return Path;
}

string UTIL_FixFilePath( string Path )
{
#ifdef WIN32
    while( Path.find( "/" ) != string::npos )
        UTIL_Replace( Path, "/", "\\" );

    while( !Path.empty( ) && Path.back( ) == '\\' )
        Path.erase( Path.end( ) - 1 );
#else
    while( Path.find( "\\" ) != string::npos )
        UTIL_Replace( Path, "\\", "/" );

    while( !Path.empty( ) && Path.back( ) == '/' )
        Path.erase( Path.end( ) - 1 );
#endif

    return Path;
}

string UTIL_GetFileNameFromPath( string Path )
{
    string FileName = Path;
    string::size_type FileNameStart = FileName.rfind( "\\" );

    if( FileNameStart != string::npos )
        FileName = FileName.substr( FileNameStart + 1 );

    FileNameStart = FileName.rfind( "/" );

    if( FileNameStart != string::npos )
        FileName = FileName.substr( FileNameStart + 1 );

    return FileName;
}

QString UTIL_GetFileNameFromPath( QString Path )
{
    return QString::fromUtf8( UTIL_GetFileNameFromPath( Path.toStdString( ) ).c_str( ) );
}

BYTEARRAY UTIL_EncodeStatString( BYTEARRAY &data )
{
	unsigned char Mask = 1;
	BYTEARRAY Result;

	for( uint32_t i = 0; i < data.size( ); i++ )
	{
		if( ( data[i] % 2 ) == 0 )
			Result.push_back( data[i] + 1 );
		else
		{
			Result.push_back( data[i] );
			Mask |= 1 << ( ( i % 7 ) + 1 );
		}

		if( i % 7 == 6 || i == data.size( ) - 1 )
		{
			Result.insert( Result.end( ) - 1 - ( i % 7 ), Mask );
			Mask = 1;
		}
	}

	return Result;
}

BYTEARRAY UTIL_DecodeStatString( BYTEARRAY &data )
{
	unsigned char Mask = '\0';
	BYTEARRAY Result;

	for( uint32_t i = 0; i < data.size( ); i++ )
	{
		if( ( i % 8 ) == 0 )
			Mask = data[i];
		else
		{
			if( ( Mask & ( 1 << ( i % 8 ) ) ) == 0 )
				Result.push_back( data[i] - 1 );
			else
				Result.push_back( data[i] );
		}
	}

	return Result;
}

bool UTIL_IsLanIP( BYTEARRAY ip )
{
	if( ip.size( ) != 4 )
		return false;

	// thanks to LuCasn for this function

	// 127.0.0.1
	if( ip[0] == 127 && ip[1] == 0 && ip[2] == 0 && ip[3] == 1 )
		return true;

	// 10.x.x.x
	if( ip[0] == 10 )
		return true;

	// 172.16.0.0-172.31.255.255
	if( ip[0] == 172 && ip[1] >= 16 && ip[1] <= 31 )
		return true;

	// 192.168.x.x
	if( ip[0] == 192 && ip[1] == 168 )
		return true;

	// RFC 3330 and RFC 3927 automatic address range
	if( ip[0] == 169 && ip[1] == 254 )
		return true;

	return false;
}

bool UTIL_IsLocalIP( BYTEARRAY ip, vector<BYTEARRAY> &localIPs )
{
	if( ip.size( ) != 4 )
		return false;

	for( vector<BYTEARRAY> ::iterator i = localIPs.begin( ); i != localIPs.end( ); i++ )
	{
		if( ( *i ).size( ) != 4 )
			continue;

		if( ip[0] == ( *i )[0] && ip[1] == ( *i )[1] && ip[2] == ( *i )[2] && ip[3] == ( *i )[3] )
			return true;
	}

	return false;
}

void UTIL_Replace( string &Text, string Key, string Value )
{
	// don't allow any infinite loops

	if( Value.find( Key ) != string::npos )
		return;

	string::size_type KeyStart = Text.find( Key );

	while( KeyStart != string::npos )
	{
		Text.replace( KeyStart, Key.size( ), Value );
		KeyStart = Text.find( Key );
	}
}

void UTIL_Replace2( string &Text, string Key, string Value )
{
    transform( Key.begin( ), Key.end( ), Key.begin( ), static_cast<int(*)(int)>(tolower) );
    string ValueLower = Value;
    transform( ValueLower.begin( ), ValueLower.end( ), ValueLower.begin( ), static_cast<int(*)(int)>(tolower) );

    // don't allow any infinite loops

    if( ValueLower.find( Key ) != string::npos )
        return;

    string TextLower = Text;
    transform( TextLower.begin( ), TextLower.end( ), TextLower.begin( ), static_cast<int(*)(int)>(tolower) );
    string::size_type KeyStart = TextLower.find( Key );

    while( KeyStart != string::npos )
    {
        Text.replace( KeyStart, Key.size( ), Value );
        TextLower.replace( KeyStart, Key.size( ), Value );
        KeyStart = TextLower.find( Key );
    }
}

vector<string> UTIL_Tokenize( string s, char delim )
{
	vector<string> Tokens;
	string Token;

	for( string::iterator i = s.begin( ); i != s.end( ); i++ )
	{
		if( *i == delim )
		{
			if( Token.empty( ) )
				continue;

			Tokens.push_back( Token );
			Token.clear( );
		}
		else
			Token += *i;
	}

	if( !Token.empty( ) )
		Tokens.push_back( Token );

	return Tokens;
}

uint32_t UTIL_Factorial( uint32_t x )
{
	uint32_t Factorial = 1;

	for( uint32_t i = 2; i <= x; i++ )
		Factorial *= i;

	return Factorial;
}


string UTIL_ToBinaryString( uint32_t data )
{
	string Result;
	uint32_t mask = 1;

	for( unsigned int i = 1; i <= 8; i++ )
	{
		if( i > 1 )
			Result += " ";
		if( data & mask )
			Result += "1";
		else
			Result += "0";
		if( mask == 1 )
			mask = 2;
		else
			mask = mask * 2;
	}

	return Result;
}

char UTIL_FromHex( char ch )
{
    return isdigit(ch) ? ch - '0' : tolower(ch) - 'a' + 10;
}

string UTIL_GetMapNameFromUrl( string url )
{
    string::size_type MapNameStart = url.rfind( "/" );
    string MapName;

    if( MapNameStart != string::npos )
        MapName = url.substr( MapNameStart + 1 );
    else if( url.size( ) <= 39 )
        MapName = url;
    else
        MapName = UTIL_SubStrFix( url, url.size( ) - 39, 0 );

    MapName = UTIL_UrlDecode( MapName );
    return MapName;
}

string UTIL_UrlFix( string url )
{
    if( url.size( ) >= 8 && url.substr( 0, 8 ) == "https://" )
        url = url.substr( 8 );
    else if( url.size( ) >= 7 && url.substr( 0, 7 ) == "http://" )
        url = url.substr( 7 );

    if( url.size( ) >= 4 && url.substr( 0, 4 ) == "www." )
        url = url.substr( 4 );

    string::size_type urlend = url.find( " " );

    if( urlend != string::npos )
        url = url.substr( 0, urlend );

    while( !url.empty( ) && url.back( ) == '/' )
        url.erase( url.end( ) - 1 );

    return url;
}

string UTIL_UrlDecode( string text, bool url )
{
    char h;
    ostringstream escaped;
    escaped.fill('0');

    for( auto i = text.begin( ), n = text.end( ); i != n; ++i )
    {
        string::value_type c = ( *i );

        if( c == '%' )
        {
            if( i + 1 == n || i + 2 == n )
                return escaped.str( );
            else if( i[1] && i[2] )
            {
                h = UTIL_FromHex( i[1] ) << 4 | UTIL_FromHex( i[2] );
                escaped << h;
                i += 2;
            }
        }
        else if( c == '+' && url )
            escaped << ' ';
        else
            escaped << c;
    }

    return escaped.str( );
}

string UTIL_CP1251ToUTF8( const char *str )
{
    string res;
    WCHAR *ures = nullptr;
    char *cres = nullptr;
    int result_u = MultiByteToWideChar( 1251, 0, str, -1, 0, 0 );

    if( result_u != 0 )
    {
        ures = new WCHAR[(unsigned int)result_u];

        if( MultiByteToWideChar( 1251, 0, str, -1, ures, result_u ) )
        {
            int result_c = WideCharToMultiByte( CP_UTF8, 0, ures, -1, 0, 0, 0, 0 );

            if( result_c != 0 )
            {
                cres = new char[(unsigned int)result_c];

                if( WideCharToMultiByte( CP_UTF8, 0, ures, -1, cres, result_c, 0, 0 ) )
                    res = cres;
            }
        }
    }

    delete[] ures;
    delete[] cres;
    return res;
}

string UTIL_FixFileLine( string line, bool removespace )
{
    // remove spaces if necessary

    if( removespace )
        line.erase( remove( line.begin( ), line.end( ), ' ' ), line.end( ) );

    // remove newlines and partial newlines to help fix issues with Windows formatted files on Linux systems

    line.erase( remove( line.begin( ), line.end( ), '\r' ), line.end( ) );
    line.erase( remove( line.begin( ), line.end( ), '\n' ), line.end( ) );

    return line;
}

uint32_t UTIL_GenerateRandomNumberUInt32( const uint32_t min, const uint32_t max )
{
    std::random_device rand_dev;
    std::mt19937 generator( rand_dev( ) );
    std::uniform_int_distribution<uint32_t> distr( min, max );
    return distr(generator);
}

uint64_t UTIL_GenerateRandomNumberUInt64( const uint64_t min, const uint64_t max )
{
    std::random_device rand_dev;
    std::mt19937 generator( rand_dev( ) );
    std::uniform_int_distribution<uint64_t> distr( min, max );
    return distr(generator);
}

string::size_type UTIL_GetPosOfLastLetter( string message )
{
    if( message.empty( ) )
        return string::npos;

    for( int32_t i = message.size( ) - 1; i >= 0; i-- )
    {
        if( message[i] == 'q' || message[i] == 'w' || message[i] == 'e' || message[i] == 'r' || message[i] == 't' || message[i] == 'y' || message[i] == 'u' || message[i] == 'i' || message[i] == 'o' || message[i] == 'p' || message[i] == 'a' || message[i] == 's' || message[i] == 'd' || message[i] == 'f' || message[i] == 'g' || message[i] == 'h' || message[i] == 'j' || message[i] == 'k' || message[i] == 'l' || message[i] == 'z' || message[i] == 'x' || message[i] == 'c' || message[i] == 'v' || message[i] == 'b' || message[i] == 'n' || message[i] == 'm' )
            return i;
    }

    return string::npos;
}

QImage UTIL_LoadTga( BYTEARRAY data, bool &success )
{
    QImage img;
    success = false;

    if( data.size( ) < 18 )
        return img;

    unsigned char idLength = data[0];
    unsigned char colorMapType = data[1];
    unsigned char imageType =  data[2];
//    uint16_t colorMapIndex = UTIL_ByteArrayToUInt16( BYTEARRAY( data.begin( ) + 3, data.begin( ) + 5 ), false );
    uint16_t colorMapLength = UTIL_ByteArrayToUInt16( BYTEARRAY( data.begin( ) + 5, data.begin( ) + 7 ), false );
    unsigned char colorMapDepth = data[7];
//    uint16_t offsetX = UTIL_ByteArrayToUInt16( BYTEARRAY( data.begin( ) + 8, data.begin( ) + 10 ), false );
//    uint16_t offsetY = UTIL_ByteArrayToUInt16( BYTEARRAY( data.begin( ) + 10, data.begin( ) + 12 ), false );
    uint16_t width = UTIL_ByteArrayToUInt16( BYTEARRAY( data.begin( ) + 12, data.begin( ) + 14 ), false );
    uint16_t height = UTIL_ByteArrayToUInt16( BYTEARRAY( data.begin( ) + 14, data.begin( ) + 16 ), false );
    unsigned char pixelDepth = data[16];
    unsigned char flags = data[17];
    bool hasEncoding = ( imageType == 9 || imageType == 10 || imageType == 11 );
    bool hasColorMap = ( imageType == 9 || imageType == 1 );
    bool isGreyColor = ( imageType == 11 || imageType == 3 );
    unsigned char bytesPerPixel = pixelDepth >> 3;
    unsigned char bytesPerPixelPalette = colorMapDepth >> 3;
    unsigned char origin = ( flags & 0x30 ) >> 0x04;
    unsigned char alphaBits = flags & 0x08;

    // What the need of a file without data ?
    if( imageType == 0 )
        return img;

    // Indexed type
    if( hasColorMap )
    {
        if( colorMapLength > 256 || colorMapType != 1 )
            return img;
        else if( colorMapDepth != 16 && colorMapDepth != 24 && colorMapDepth != 32 )
            return img;
    }
    else if( colorMapType )
        return img;

    // Check image size
    if( width <= 0 || height <= 0 )
        return img;

    // Check pixel size
    if( pixelDepth != 8 && pixelDepth != 16 && pixelDepth != 24 && pixelDepth != 32 )
        return img;

    // Check alpha size
    if( alphaBits != 0 && alphaBits != 1 && alphaBits != 8 )
        return img;

    uint32_t offset = 18;

    // Move to data
    offset += idLength;

    if( offset >= data.size( ) )
        return img;

    BYTEARRAY palette;

    // Read palette
    if( hasColorMap )
    {
        uint32_t colorMapSize = colorMapLength * bytesPerPixelPalette;

        if( colorMapSize + offset >= data.size( ) )
            return img;

        palette = BYTEARRAY( data.begin( ) + offset, data.begin( ) + offset + colorMapSize );
        offset += colorMapSize;
    }

    uint32_t imageSize = width * height;
    uint32_t pixelTotal = imageSize * bytesPerPixel;
    BYTEARRAY imageData;

    if( hasEncoding ) // RLE encoded
    {
        uint32_t RLELength = data.size( ) - offset - 26;
        BYTEARRAY RLEData = BYTEARRAY( data.begin( ) + offset, data.begin( ) + offset + RLELength );

        // decode...

        uint32_t pos = 0; // offset for imageData
        uint32_t c;
        uint32_t count;
        uint32_t i;
        uint32_t offsetRLE = 0; // offset in RLEData
        /*
        BYTEARRAY pixels( bytesPerPixel );
        imageData = BYTEARRAY( pixelTotal );

        while( pos < pixelTotal )
        {
            c = RLEData[offsetRLE++]; // current byte to check
            count = ( c & 0x7f ) + 1; // repetition count of pixels, the lower 7 bits + 1

            if( c & 0x80 ) // RLE packet, if highest bit is set to 1.
            {
                // Copy pixel values to be repeated to tmp array
                for( i = 0; i < bytesPerPixel; ++i )
                    pixels[i] = RLEData[offsetRLE++];

                // Copy pixel values * count to imageData
                for( i = 0; i < count; ++i )
                {
                    std::copy( pixels.begin( ), pixels.end( ), imageData.begin( ) + pos );
                    pos += bytesPerPixel;
                }
            }
            else // Raw packet (Non-Run-Length Encoded)
            {
                count *= bytesPerPixel;

                for( i = 0; i < count; ++i )
                    imageData[pos++] = RLEData[offsetRLE++];
            }
        }*/

        BYTEARRAY pixels;
//        imageData = BYTEARRAY( pixelTotal );

        while( pos < pixelTotal )
        {
            c = RLEData[offsetRLE]; // current byte to check

            if( RLEData.size( ) != offsetRLE + 1 )
                offsetRLE++;

            count = ( c & 0x7f ) + 1; // repetition count of pixels, the lower 7 bits + 1

            if( c & 0x80 ) // RLE packet, if highest bit is set to 1.
            {
                // Copy pixel values to be repeated to tmp array
                for( i = 0; i < bytesPerPixel; ++i )
                {
                    pixels.push_back( RLEData[offsetRLE] );

                    if( RLEData.size( ) != offsetRLE + 1 )
                        offsetRLE++;
                }

                // Copy pixel values * count to imageData
                for( i = 0; i < count; ++i )
                {
                    UTIL_AppendByteArrayFast( imageData, pixels );
                    pos += bytesPerPixel;
                }

                pixels.clear( );
            }
            else // Raw packet (Non-Run-Length Encoded)
            {
                count *= bytesPerPixel;

                for( i = 0; i < count; ++i )
                {
                    imageData.push_back( RLEData[offsetRLE] );
                    pos++;

                    if( RLEData.size( ) != offsetRLE + 1 )
                        offsetRLE++;
                }
            }
        }

        if( pos > pixelTotal )
            return img;
    }
    else // RAW pixels
        imageData = BYTEARRAY( data.begin( ) + offset, data.begin( ) + offset + ( hasColorMap ? imageSize : pixelTotal ) );

    // read footer...

    offset = data.size( ) - 26;
    string signature = "TRUEVISION-XFILE.";
    bool hasFooter = false;
    unsigned char attributeType = 0;
    uint32_t extensionOffset = 0;
    uint32_t developerOffset = 0;
    bool hasExtensionArea = false;
    bool hasDeveloperArea = false;
    bool usesAlpha = false;

    if( data.size( ) >= offset + 9 + signature.size( ) )
    {
        BYTEARRAY Realsignature = UTIL_ExtractCString( data, offset + 8 );

        if( string( Realsignature.begin( ), Realsignature.end( ) ) == signature )
        {
            hasFooter = true;
            extensionOffset = UTIL_ByteArrayToUInt32( BYTEARRAY( data.begin( ) + offset, data.begin( ) + offset + 4 ), false );
            developerOffset = UTIL_ByteArrayToUInt32( BYTEARRAY( data.begin( ) + offset + 4, data.begin( ) + offset + 8 ), false );
            hasExtensionArea = extensionOffset != 0;
            hasDeveloperArea = developerOffset != 0;

            if( hasExtensionArea && data.size( ) > extensionOffset + 494 )
                attributeType = data[extensionOffset + 494];
            else
                hasExtensionArea = false;
        }
    }

    if( ( alphaBits != 0 ) || ( hasExtensionArea && ( attributeType == 3 || attributeType == 4 ) ) )
        usesAlpha = true;

    uint32_t x_start;
    uint32_t x_step;
    uint32_t x_end;
    uint32_t y_start;
    uint32_t y_step;
    uint32_t y_end;
    BYTEARRAY getImageData;

    if( origin == 0x02 || origin == 0x03 )
    {
        y_start = 0;
        y_step = 1;
        y_end = height;
    }
    else
    {
        y_start = height - 1;
        y_step = -1;
        y_end = -1;
    }

    if( origin == 0x02 || origin == 0x00 )
    {
        x_start = 0;
        x_step = 1;
        x_end = width;
    }
    else
    {
        x_start = width - 1;
        x_step = -1;
        x_end = -1;
    }

    // TODO: use this.header.offsetX and this.header.offsetY ?

    BYTEARRAY finalData( width * height * 4 );

    if( pixelDepth == 8 )
    {
        if( isGreyColor )
        {
            unsigned char color;
            uint32_t i;
            uint32_t x;
            uint32_t y;

            for( i = 0, y = y_start; y != y_end; y += y_step )
            {
                for( x = x_start; x != x_end; x += x_step, i++ )
                {
                    color = imageData[i];
                    offset = ( x + width * y ) * 4;
                    finalData[offset] = color;		// red
                    finalData[offset + 1] = color;	// green
                    finalData[offset + 2] = color;	// blue
                    finalData[offset + 3] = 255;	// alpha
                }
            }

            success = true;
            img = QImage( width, height, QImage::Format_RGBA8888 );

            for( unsigned int x2 = 0; x2 < width; x2++ )
            {
                for( unsigned int y2 = 0; y2 < height; y2++ )
                {
                    int valr = finalData[y2 * width * 4 + x2 * 4];
                    int valg = finalData[y2 * width * 4 + x2 * 4 + 1];
                    int valb = finalData[y2 * width * 4 + x2 * 4 + 2];
                    int vala = finalData[y2 * width * 4 + x2 * 4 + 3];

                    QColor value( valr, valg, valb, vala );
                    img.setPixelColor( x2, y2, value );
                }
            }

            return img;
        }
        else
        {
            unsigned char color;
            uint32_t index;
            uint32_t i;
            uint32_t x;
            uint32_t y;

            for( i = 0, y = y_start; y != y_end; y += y_step )
            {
                for( x = x_start; x != x_end; x += x_step, i++ )
                {
                    offset = (x + width * y) * 4;
                    index = imageData[i] * bytesPerPixelPalette;

                    if( bytesPerPixelPalette == 4 )
                    {
                        finalData[offset] = palette[index + 2]; 			// red
                        finalData[offset + 1] = palette[index + 1];			// green
                        finalData[offset + 2] = palette[index]; 			// blue
                        finalData[offset + 3] = palette[index + 3];			// alpha
                    }
                    else if( bytesPerPixelPalette == 3 )
                    {
                        finalData[offset] = palette[index + 2];				// red
                        finalData[offset + 1] = palette[index + 1];			// green
                        finalData[offset + 2] = palette[index];				// blue
                        finalData[offset + 3] = 255; 						// alpha
                    }
                    else if( bytesPerPixelPalette == 2 )
                    {
                        color = palette[index] | (palette[index + 1] << 8);
                        finalData[offset] = (color & 0x7C00) >> 7; 			// red
                        finalData[offset + 1] = (color & 0x03E0) >> 2; 		// green
                        finalData[offset + 2] = (color & 0x001F) << 3; 		// blue
                        finalData[offset + 3] = (color & 0x8000) ? 0 : 255; // overlay 0 = opaque and 1 = transparent Discussion at: https://bugzilla.gnome.org/show_bug.cgi?id=683381
                    }
                }
            }

            success = true;
            img = QImage( width, height, QImage::Format_RGBA8888 );

            for( unsigned int x2 = 0; x2 < width; x2++ )
            {
                for( unsigned int y2 = 0; y2 < height; y2++ )
                {
                    int valr = finalData[y2 * width * 4 + x2 * 4];
                    int valg = finalData[y2 * width * 4 + x2 * 4 + 1];
                    int valb = finalData[y2 * width * 4 + x2 * 4 + 2];
                    int vala = finalData[y2 * width * 4 + x2 * 4 + 3];

                    QColor value( valr, valg, valb, vala );
                    img.setPixelColor( x2, y2, value );
                }
            }

            return img;
        }
    }
    else if( pixelDepth == 16 )
    {
        if( isGreyColor )
        {
            unsigned char color;
            uint32_t i;
            uint32_t x;
            uint32_t y;

            for( i = 0, y = y_start; y != y_end; y += y_step )
            {
                for( x = x_start; x != x_end; x += x_step, i += 2 )
                {
                    color = imageData[i];
                    offset = (x + width * y) * 4;
                    finalData[offset] = color;
                    finalData[offset + 1] = color;
                    finalData[offset + 2] = color;
                    finalData[offset + 3] = imageData[i + 1];
                }
            }

            success = true;
            img = QImage( width, height, QImage::Format_RGBA8888 );

            for( unsigned int x2 = 0; x2 < width; x2++ )
            {
                for( unsigned int y2 = 0; y2 < height; y2++ )
                {
                    int valr = finalData[y2 * width * 4 + x2 * 4];
                    int valg = finalData[y2 * width * 4 + x2 * 4 + 1];
                    int valb = finalData[y2 * width * 4 + x2 * 4 + 2];
                    int vala = finalData[y2 * width * 4 + x2 * 4 + 3];

                    QColor value( valr, valg, valb, vala );
                    img.setPixelColor( x2, y2, value );
                }
            }

            return img;
        }
        else
        {
            unsigned char color;
            uint32_t i;
            uint32_t x;
            uint32_t y;

            for( i = 0, y = y_start; y != y_end; y += y_step )
            {
                for( x = x_start; x != x_end; x += x_step, i += 2 )
                {
                    color = imageData[i] | (imageData[i + 1] << 8);
                    offset = (x + width * y) * 4;
                    finalData[offset] = (color & 0x7C00) >> 7;          // red
                    finalData[offset + 1] = (color & 0x03E0) >> 2;		// green
                    finalData[offset + 2] = (color & 0x001F) << 3;		// blue
                    finalData[offset + 3] = (color & 0x8000) ? 0 : 255;	// overlay 0 = opaque and 1 = transparent Discussion at: https://bugzilla.gnome.org/show_bug.cgi?id=683381
                }
            }

            success = true;
            img = QImage( width, height, QImage::Format_RGBA8888 );

            for( unsigned int x2 = 0; x2 < width; x2++ )
            {
                for( unsigned int y2 = 0; y2 < height; y2++ )
                {
                    int valr = finalData[y2 * width * 4 + x2 * 4];
                    int valg = finalData[y2 * width * 4 + x2 * 4 + 1];
                    int valb = finalData[y2 * width * 4 + x2 * 4 + 2];
                    int vala = finalData[y2 * width * 4 + x2 * 4 + 3];

                    QColor value( valr, valg, valb, vala );
                    img.setPixelColor( x2, y2, value );
                }
            }

            return img;
        }
    }
    else if( pixelDepth == 24 )
    {
        uint32_t i;
        uint32_t x;
        uint32_t y;

        for( i = 0, y = y_start; y != y_end; y += y_step )
        {
            for( x = x_start; x != x_end; x += x_step, i += bytesPerPixel )
            {
                offset = (x + width * y) * 4;
                finalData[offset + 3] = 255;				// alpha
                finalData[offset + 2] = imageData[i];		// blue
                finalData[offset + 1] = imageData[i + 1];	// green
                finalData[offset] = imageData[i + 2];		// red
            }
        }

        success = true;
        img = QImage( width, height, QImage::Format_RGBA8888 );

        for( unsigned int x2 = 0; x2 < width; x2++ )
        {
            for( unsigned int y2 = 0; y2 < height; y2++ )
            {
                int valr = finalData[y2 * width * 4 + x2 * 4];
                int valg = finalData[y2 * width * 4 + x2 * 4 + 1];
                int valb = finalData[y2 * width * 4 + x2 * 4 + 2];
                int vala = finalData[y2 * width * 4 + x2 * 4 + 3];

                QColor value( valr, valg, valb, vala );
                img.setPixelColor( x2, y2, value );
            }
        }

        return img;
    }
    else if( pixelDepth == 32 )
    {
        if( hasExtensionArea )
        {
            if( attributeType == 3 )
            {
                // straight alpha

                uint32_t i;
                uint32_t x;
                uint32_t y;

                for( i = 0, y = y_start; y != y_end; y += y_step )
                {
                    for( x = x_start; x != x_end; x += x_step, i += 4 )
                    {
                        offset = (x + width * y) * 4;
                        finalData[offset + 2] = imageData[i]; 		// blue
                        finalData[offset + 1] = imageData[i + 1];	// green
                        finalData[offset] = imageData[i + 2];		// red
                        finalData[offset + 3] = imageData[i + 3];	// alpha
                    }
                }

                success = true;
                img = QImage( width, height, QImage::Format_RGBA8888 );

                for( unsigned int x2 = 0; x2 < width; x2++ )
                {
                    for( unsigned int y2 = 0; y2 < height; y2++ )
                    {
                        int valr = finalData[y2 * width * 4 + x2 * 4];
                        int valg = finalData[y2 * width * 4 + x2 * 4 + 1];
                        int valb = finalData[y2 * width * 4 + x2 * 4 + 2];
                        int vala = finalData[y2 * width * 4 + x2 * 4 + 3];

                        QColor value( valr, valg, valb, vala );
                        img.setPixelColor( x2, y2, value );
                    }
                }

                return img;
            }
            else if( attributeType == 4 )
            {
                // pre multiplied alpha

                uint32_t i;
                uint32_t x;
                uint32_t y;
                uint32_t alpha;

                for( i = 0, y = y_start; y != y_end; y += y_step )
                {
                    for( x = x_start; x != x_end; x += x_step, i += 4 )
                    {
                        offset = (x + width * y) * 4;
                        alpha = imageData[i + 3] * 255; 					// TODO needs testing
                        finalData[offset + 2] = imageData[i] / alpha; 		// blue
                        finalData[offset + 1] = imageData[i + 1] / alpha;	// green
                        finalData[offset] = imageData[i + 2] / alpha; 		// red
                        finalData[offset + 3] = imageData[i + 3]; 			// alpha
                    }
                }

                success = true;
                img = QImage( width, height, QImage::Format_RGBA8888 );

                for( unsigned int x2 = 0; x2 < width; x2++ )
                {
                    for( unsigned int y2 = 0; y2 < height; y2++ )
                    {
                        int valr = finalData[y2 * width * 4 + x2 * 4];
                        int valg = finalData[y2 * width * 4 + x2 * 4 + 1];
                        int valb = finalData[y2 * width * 4 + x2 * 4 + 2];
                        int vala = finalData[y2 * width * 4 + x2 * 4 + 3];

                        QColor value( valr, valg, valb, vala );
                        img.setPixelColor( x2, y2, value );
                    }
                }

                return img;
            }
            else
            {
                // ignore alpha values if attributeType set to 0, 1, 2

                uint32_t i;
                uint32_t x;
                uint32_t y;

                for( i = 0, y = y_start; y != y_end; y += y_step )
                {
                    for( x = x_start; x != x_end; x += x_step, i += bytesPerPixel )
                    {
                        offset = (x + width * y) * 4;
                        finalData[offset + 3] = 255;				// alpha
                        finalData[offset + 2] = imageData[i];		// blue
                        finalData[offset + 1] = imageData[i + 1];	// green
                        finalData[offset] = imageData[i + 2];		// red
                    }
                }

                success = true;
                img = QImage( width, height, QImage::Format_RGBA8888 );

                for( unsigned int x2 = 0; x2 < width; x2++ )
                {
                    for( unsigned int y2 = 0; y2 < height; y2++ )
                    {
                        int valr = finalData[y2 * width * 4 + x2 * 4];
                        int valg = finalData[y2 * width * 4 + x2 * 4 + 1];
                        int valb = finalData[y2 * width * 4 + x2 * 4 + 2];
                        int vala = finalData[y2 * width * 4 + x2 * 4 + 3];

                        QColor value( valr, valg, valb, vala );
                        img.setPixelColor( x2, y2, value );
                    }
                }

                return img;
            }
        }
        else
        {
            if( alphaBits != 0 )
            {
                uint32_t i;
                uint32_t x;
                uint32_t y;

                for( i = 0, y = y_start; y != y_end; y += y_step )
                {
                    for( x = x_start; x != x_end; x += x_step, i += 4 )
                    {
                        offset = (x + width * y) * 4;
                        finalData[offset + 2] = imageData[i]; 		// blue
                        finalData[offset + 1] = imageData[i + 1];	// green
                        finalData[offset] = imageData[i + 2];		// red
                        finalData[offset + 3] = imageData[i + 3];	// alpha
                    }
                }

                success = true;
                img = QImage( width, height, QImage::Format_RGBA8888 );

                for( unsigned int x2 = 0; x2 < width; x2++ )
                {
                    for( unsigned int y2 = 0; y2 < height; y2++ )
                    {
                        int valr = finalData[y2 * width * 4 + x2 * 4];
                        int valg = finalData[y2 * width * 4 + x2 * 4 + 1];
                        int valb = finalData[y2 * width * 4 + x2 * 4 + 2];
                        int vala = finalData[y2 * width * 4 + x2 * 4 + 3];

                        QColor value( valr, valg, valb, vala );
                        img.setPixelColor( x2, y2, value );
                    }
                }

                return img;
            }
            else
            {
                // 32 bits Depth, but alpha Bits set to 0

                uint32_t i;
                uint32_t x;
                uint32_t y;

                for( i = 0, y = y_start; y != y_end; y += y_step )
                {
                    for( x = x_start; x != x_end; x += x_step, i += bytesPerPixel )
                    {
                        offset = (x + width * y) * 4;
                        finalData[offset + 3] = 255;				// alpha
                        finalData[offset + 2] = imageData[i];		// blue
                        finalData[offset + 1] = imageData[i + 1];	// green
                        finalData[offset] = imageData[i + 2];		// red
                    }
                }

                success = true;
                img = QImage( width, height, QImage::Format_RGBA8888 );

                for( unsigned int x2 = 0; x2 < width; x2++ )
                {
                    for( unsigned int y2 = 0; y2 < height; y2++ )
                    {
                        int valr = finalData[y2 * width * 4 + x2 * 4];
                        int valg = finalData[y2 * width * 4 + x2 * 4 + 1];
                        int valb = finalData[y2 * width * 4 + x2 * 4 + 2];
                        int vala = finalData[y2 * width * 4 + x2 * 4 + 3];

                        QColor value( valr, valg, valb, vala );
                        img.setPixelColor( x2, y2, value );
                    }
                }

                return img;
            }
        }
    }
    else
    {
        return img;
    }

    return img;
}

bool UTIL_CompareGameNames( string newgamename, string oldgamename )
{
    if( newgamename.size( ) != oldgamename.size( ) )
        return false;

    if( newgamename == oldgamename )
        return true;

    while( !newgamename.empty( ) && newgamename.front( ) == ' ' )
        newgamename.erase( newgamename.begin( ) );

    while( !oldgamename.empty( ) && oldgamename.front( ) == ' ' )
        oldgamename.erase( oldgamename.begin( ) );

    string::size_type newgamenameend = newgamename.find( " " );
    string::size_type oldgamenameend = oldgamename.find( " " );

    if( newgamenameend != oldgamenameend )
        return false;

    if( newgamenameend != string::npos )
        newgamename = newgamename.substr( 0, newgamenameend );

    if( oldgamenameend != string::npos )
        oldgamename = oldgamename.substr( 0, oldgamenameend );

    if( newgamename != oldgamename || newgamename.empty( ) || oldgamename.empty( ) )
        return false;

    return true;
}
