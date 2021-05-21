#ifndef CONFIGSECTION_H
#define CONFIGSECTION_H

//
// CConfigSection
//

class CConfigSection
{
private:
    std::map<std::string, std::map<std::string, std::string>> m_CFG;

public:
	CConfigSection( );
	~CConfigSection( );
    bool Read( std::string file, bool first = true );
    bool Exists( std::string section, std::string key );
    int32_t GetInt( std::string section, std::string key, int32_t x );
    uint32_t GetUInt( std::string section, std::string key, uint32_t x, uint32_t max = 0 );
    uint32_t GetUInt( std::string section, std::string key, uint32_t x, uint32_t min, uint32_t max );
    uint16_t GetUInt16( std::string section, std::string key, uint16_t x );
    uint64_t GetUInt64( std::string section, std::string key, uint64_t x );
    int64_t GetInt64( std::string section, std::string key, int64_t x );
    std::string GetString( std::string section, std::string key, std::string x );
    std::string GetValidString( std::string section, std::string key, std::string x );
    void Set( std::string section, std::string key, std::string x );
};

#endif
