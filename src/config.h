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

#ifndef CONFIG_H
#define CONFIG_H

//
// CConfig
//

class CConfig
{
private:
    std::map<std::string, std::string> m_CFG;

public:
	CConfig( );
	~CConfig( );
    bool Read( std::string file, uint32_t flag = 0 );
	bool Exists( std::string key );
	int32_t GetInt( std::string key, int32_t x );
    uint32_t GetUInt( std::string key, uint32_t x, uint32_t max = 0 );
    uint32_t GetUInt( std::string key, uint32_t x, uint32_t min, uint32_t max );
    uint16_t GetUInt16( std::string key, uint16_t x );
	uint64_t GetUInt64( std::string key, uint64_t x );
	int64_t GetInt64( std::string key, int64_t x );
	std::string GetString( std::string key, std::string x );
	std::string GetValidString( std::string key, std::string x );
	void Set( std::string key, std::string x );
};

#endif
