#include "unm.h"
#include "configsection.h"
#include "util.h"

using namespace std;

//
// CConfigSection
//

CConfigSection::CConfigSection( )
{

}

CConfigSection::~CConfigSection( )
{

}

bool CConfigSection::Read( string file, bool first )
{
	ifstream in;
	string SectionName = string( );

#ifdef WIN32
    in.open( UTIL_UTF8ToCP1251( file.c_str( ) ) );
#else
    in.open( file );
#endif

	if( in.fail( ) )
	{
        CONSOLE_Print( "[CONFIG] warning - unable to read file [" + file + "]" );
        in.close( );
        return false;
	}
	else
	{

        CONSOLE_Print( "[CONFIG] loading file [" + file + "]" );
		string Line;

		while( !in.eof( ) )
		{
			getline( in, Line );
            Line = UTIL_FixFileLine( Line );

			// ignore blank lines and comments

			if( Line.empty( ) || Line[0] == '#' )
				continue;
				
			if( Line.front( ) == '[' && Line.size( ) > 1 )
            {
                string::size_type endpos = Line.find_last_not_of( " " );

                if( endpos != string::npos && Line[endpos] == ']' )
				{
                    SectionName = Line.substr( 1, endpos - 1 );
					continue;
				}
            }

			string::size_type Split = Line.find( "=" );

			if( Split == string::npos )
				continue;

			string::size_type KeyStart = Line.find_first_not_of( " " );
			string::size_type KeyEnd = Line.find( " ", KeyStart );
			string::size_type ValueStart = Line.find_first_not_of( " ", Split + 1 );
			string::size_type ValueEnd = Line.size( );

            if( ValueStart != string::npos && ( first || ( SectionName != "UNM_KEYBINDS" && SectionName != "UNM_SMARTCAST" && SectionName != "KEYBINDS" && SectionName != "SMARTCAST" && ( SectionName != "type1" || Line.substr( KeyStart, KeyEnd - KeyStart ) != "type1" ) ) ) )
				m_CFG[SectionName][Line.substr( KeyStart, KeyEnd - KeyStart )] = Line.substr( ValueStart, ValueEnd - ValueStart );
		}
	}

	in.close( );
    return true;
}

bool CConfigSection::Exists( string section, string key )
{
	return m_CFG[section].find( key ) != m_CFG[section].end( );
}

int32_t CConfigSection::GetInt( string section, string key, int32_t x )
{
	if( m_CFG[section].find( key ) == m_CFG[section].end( ) )
		return x;
	else
		return atoi( m_CFG[section][key].c_str( ) );
}

uint32_t CConfigSection::GetUInt( string section, string key, uint32_t x, uint32_t max )
{
	if( m_CFG[section].find( key ) == m_CFG[section].end( ) )
		return x;
	else
    {
        if( max == 0 )
            return strtoul( m_CFG[section][key].c_str( ), nullptr, 0 );
        else
        {
            uint32_t returnvalue = strtoul( m_CFG[section][key].c_str( ), nullptr, 0 );

            if( returnvalue > max )
                return x;
            else
                return returnvalue;
        }
    }
}

uint32_t CConfigSection::GetUInt( string section, string key, uint32_t x, uint32_t min, uint32_t max )
{
    if( m_CFG[section].find( key ) == m_CFG[section].end( ) )
        return x;
    else
    {
        uint32_t returnvalue = strtoul( m_CFG[section][key].c_str( ), nullptr, 0 );

        if( returnvalue <= min )
            return min;
        else if( returnvalue >= max )
            return max;
        else
            return returnvalue;
    }
}

uint16_t CConfigSection::GetUInt16( string section, string key, uint16_t x )
{
	if( m_CFG[section].find( key ) == m_CFG[section].end( ) )
		return x;
	else
	{
		string ValidString = m_CFG[section][key];

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

uint64_t CConfigSection::GetUInt64( string section, string key, uint64_t x )
{
	if( m_CFG[section].find( key ) == m_CFG[section].end( ) )
		return x;
	else
		return strtoull( m_CFG[section][key].c_str( ), nullptr, 0 );
}

int64_t CConfigSection::GetInt64( string section, string key, int64_t x )
{
	if( m_CFG[section].find( key ) == m_CFG[section].end( ) )
		return x;
	else
		return atoll( m_CFG[section][key].c_str( ) );
}

string CConfigSection::GetString( string section, string key, string x )
{
	if( m_CFG[section].find( key ) == m_CFG[section].end( ) )
		return x;
	else
		return m_CFG[section][key];
}

string CConfigSection::GetValidString( string section, string key, string x )
{
	if( m_CFG[section].find( key ) == m_CFG[section].end( ) )
		return x;
	else
	{
		string ValidString = m_CFG[section][key];

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

void CConfigSection::Set( string section, string key, string x )
{
	m_CFG[section][key] = x;
}
