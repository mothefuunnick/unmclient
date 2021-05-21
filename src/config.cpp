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
#include "config.h"
#include "util.h"

using namespace std;

//
// CConfig
//

CConfig::CConfig( )
{

}

CConfig::~CConfig( )
{

}

bool CConfig::Read( string file, uint32_t flag )
{
    if( flag == 1 )
	{
		ifstream ind;
        string defaultcfg;

#ifdef WIN32
        defaultcfg = "text_files\\default.cfg";
        ind.open( UTIL_UTF8ToCP1251( defaultcfg.c_str( ) ) );
#else
        defaultcfg = "text_files/default.cfg";
        ind.open( defaultcfg );
#endif

		if( ind.fail( ) )
            CONSOLE_Print( "[CONFIG] warning - unable to read file [" + defaultcfg + "]" );
		else
		{
            CONSOLE_Print( "[CONFIG] loading file [" + defaultcfg + "]" );
			string Line;

			while( !ind.eof( ) )
			{
				getline( ind, Line );
                Line = UTIL_FixFileLine( Line );

				// ignore blank lines and comments

				if( Line.empty( ) || Line[0] == '#' )
					continue;

				string::size_type Split = Line.find( "=" );

				if( Split == string::npos )
					continue;

				string::size_type KeyStart = Line.find_first_not_of( " " );
				string::size_type KeyEnd = Line.find( " ", KeyStart );
				string::size_type ValueStart = Line.find_first_not_of( " ", Split + 1 );
				string::size_type ValueEnd = Line.size( );

				if( ValueStart != string::npos )
					m_CFG[Line.substr( KeyStart, KeyEnd - KeyStart )] = Line.substr( ValueStart, ValueEnd - ValueStart );
			}
		}

		ind.close( );
	}

	ifstream in;

#ifdef WIN32
    in.open( UTIL_UTF8ToCP1251( file.c_str( ) ) );
#else
    in.open( file );
#endif

	if( in.fail( ) )
	{
        if( flag == 2 )
            CONSOLE_Print( "[UNM] warning - unable to read file [" + file + "]" );
		else
            CONSOLE_Print( "[CONFIG] warning - unable to read file [" + file + "]" );

        in.close( );
        return false;
	}
	else
	{
        if( flag == 2 )
            CONSOLE_Print( "[UNM] loading file [" + file + "]" );
		else
            CONSOLE_Print( "[CONFIG] loading file [" + file + "]" );

		string Line;

		while( !in.eof( ) )
		{
			getline( in, Line );
            Line = UTIL_FixFileLine( Line );

			// ignore blank lines and comments

			if( Line.empty( ) || Line[0] == '#' )
				continue;

			string::size_type Split = Line.find( "=" );

			if( Split == string::npos )
				continue;

			string::size_type KeyStart = Line.find_first_not_of( " " );
			string::size_type KeyEnd = Line.find( " ", KeyStart );
			string::size_type ValueStart = Line.find_first_not_of( " ", Split + 1 );
			string::size_type ValueEnd = Line.size( );

			if( ValueStart != string::npos )
				m_CFG[Line.substr( KeyStart, KeyEnd - KeyStart )] = Line.substr( ValueStart, ValueEnd - ValueStart );
		}
	}

	in.close( );
    return true;
}

bool CConfig::Exists( string key )
{
	return m_CFG.find( key ) != m_CFG.end( );
}

int32_t CConfig::GetInt( string key, int32_t x )
{
	if( m_CFG.find( key ) == m_CFG.end( ) )
		return x;
	else
		return atoi( m_CFG[key].c_str( ) );
}

uint32_t CConfig::GetUInt( string key, uint32_t x, uint32_t max )
{
	if( m_CFG.find( key ) == m_CFG.end( ) )
		return x;
	else
    {
        if( max == 0 )
            return strtoul( m_CFG[key].c_str( ), nullptr, 0 );
        else
        {
            uint32_t returnvalue = strtoul( m_CFG[key].c_str( ), nullptr, 0 );

            if( returnvalue > max )
                return x;
            else
                return returnvalue;
        }
    }
}

uint32_t CConfig::GetUInt( string key, uint32_t x, uint32_t min, uint32_t max )
{
    if( m_CFG.find( key ) == m_CFG.end( ) )
        return x;
    else
    {
        uint32_t returnvalue = strtoul( m_CFG[key].c_str( ), nullptr, 0 );

        if( returnvalue <= min )
            return min;
        else if( returnvalue >= max )
            return max;
        else
            return returnvalue;
    }
}

uint16_t CConfig::GetUInt16( string key, uint16_t x )
{
	if( m_CFG.find( key ) == m_CFG.end( ) )
		return x;
	else
	{
		string ValidString = m_CFG[key];

        while( !ValidString.empty( ) && ValidString.back( ) == ' ' )
            ValidString.erase( ValidString.end( ) - 1 );
        while( !ValidString.empty( ) && ValidString.front( ) == ' ' )
            ValidString.erase( ValidString.begin( ) );

		string::size_type ValidStringEnd = ValidString.find( " " );

		if( ValidStringEnd != string::npos )
			ValidString = ValidString.substr( 0, ValidStringEnd );

		if( ValidString.empty( ) )
			return x;

        char * temp;
        uint32_t ValidUInt = strtoul( ValidString.c_str( ), &temp, 0 );

        if( *temp != '\0' || ValidUInt > 65535 )
            return x;
        else
            return static_cast<uint16_t>(ValidUInt);
	}
}

uint64_t CConfig::GetUInt64( string key, uint64_t x )
{
	if( m_CFG.find( key ) == m_CFG.end( ) )
		return x;
	else
		return strtoull( m_CFG[key].c_str( ), nullptr, 0 );
}

int64_t CConfig::GetInt64( string key, int64_t x )
{
	if( m_CFG.find( key ) == m_CFG.end( ) )
		return x;
	else
		return atoll( m_CFG[key].c_str( ) );
}

string CConfig::GetString( string key, string x )
{
	if( m_CFG.find( key ) == m_CFG.end( ) )
		return x;
	else
		return m_CFG[key];
}

string CConfig::GetValidString( string key, string x )
{
	if( m_CFG.find( key ) == m_CFG.end( ) )
		return x;
	else
	{
		string ValidString = m_CFG[key];

        while( ValidString.back( ) == ' ' )
            ValidString.erase( ValidString.end( ) - 1 );

        while( ValidString.front( ) == ' ' )
            ValidString.erase( ValidString.begin( ) );

		string::size_type ValidStringEnd = ValidString.find( " " );

		if( ValidStringEnd != string::npos )
			ValidString = ValidString.substr( 0, ValidStringEnd );

		if( ValidString.empty( ) )
			return x;
		else
			return ValidString;
	}
}

void CConfig::Set( string key, string x )
{
	m_CFG[key] = x;
}
