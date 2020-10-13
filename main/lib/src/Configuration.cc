#include "eudaq/Configuration.hh"
#include "eudaq/Platform.hh"

#include <fstream>
#include <iostream>
#include <cstdlib>

namespace eudaq {

  Configuration::Configuration(const std::string & config, const std::string & section)
    : m_cur(&m_config[""]) {
      std::istringstream confstr(config);
      Load(confstr, section);
    }

  Configuration::Configuration(std::istream & conffile, const std::string & section)
    : m_cur(&m_config[""]) {
      Load(conffile, section);
    }

  Configuration::Configuration(const Configuration & other)
    : m_config(other.m_config)
  {
    SetSection(other.m_section);
  }

  Configuration::Configuration(const std::string & file_name, const std::string & section, bool /*unused*/): m_cur(&m_config[""]), m_file_name(file_name) {

    if (!file_name.empty()){
      std::cout << "Reading config file: " << file_name << std::endl;
      std::ifstream file(file_name.c_str());
      if (file.is_open()) {
        Load(file, section);
        std::string name = file_name.substr(0, file_name.find('.'));
        Set("Name", name);
      } else {
        std::cout << "Unable to open file '" << file_name << "'" << std::endl;
      }
      file.close();
    }
  }

  std::string Configuration::Name() const {
    map_t::const_iterator it = m_config.find("");
    if (it == m_config.end()) return "";
    section_t::const_iterator it2 = it->second.find("Name");
    if (it2 == it->second.end()) return "";
    return it2->second;
  }

  void Configuration::Save(std::ostream & stream) const {
    for (map_t::const_iterator i = m_config.begin(); i != m_config.end(); ++i) {
      if (i->first != "") {
        stream << "[" << i->first << "]\n";
      }
      for (section_t::const_iterator j = i->second.begin(); j != i->second.end(); ++j) {
        stream << j->first << " = " << j->second << "\n";
      }
      stream << "\n";
    }
  }

  void Configuration::Save(const std::string & file_name) const {

    std::ofstream file(file_name);
    Save(file);
    file.close();
  }

  void Configuration::Save() const {

    std::ofstream file(m_file_name);
    Save(file);
    file.close();
  }



  Configuration & Configuration::operator = (const Configuration & other) {
    m_config = other.m_config;
    SetSection(other.m_section);
    return *this;
  }

  void Configuration::Load(std::istream & stream, const std::string & section) {
    map_t config;
    section_t * cur_sec = &config[""];
    for (;;) {
      std::string line;
      if (stream.eof()) break;
      std::getline(stream, line);
      size_t equals = line.find('=');
      if (equals == std::string::npos) {
        line = trim(line);
        if (line == "" || line[0] == ';' || line[0] == '#') continue;
        if (line[0] == '[' && line[line.length()-1] == ']') {
          line = std::string(line, 1, line.length()-2);
          // TODO: check name is alphanumeric?
          //std::cerr << "Section " << line << std::endl;
          cur_sec = &config[line];
        }
      } else {
        std::string key = trim(std::string(line, 0, equals));
        // TODO: check key does not already exist
        // handle lines like: blah = "foo said ""bar""; ok." # not "baz"
        line = trim(std::string(line, equals+1));
        if ((line[0] == '\'' && line[line.length()-1] == '\'') ||
            (line[0] == '\"' && line[line.length()-1] == '\"')) {
          line = std::string(line, 1, line.length()-2);
        } else {
          size_t i = line.find_first_of(";#");
          if (i != std::string::npos) line = trim(std::string(line, 0, i));
        }
        //std::cerr << "Key " << key << " = " << line << std::endl;
        (*cur_sec)[key] = line;
      }
    }
    m_config = config;
    SetSection(section);
  }

  void Configuration::Read(const std::string & name, const std::string & section) {

    std::ifstream file(name);
    std::string line;
    std::string cur_section;
    while (std::getline(file, line)){
      line = trim(line);
      if (line.empty() or line.at(0) == ';' or line.at(0) == '#') { continue; }
      size_t equal_pos = line.find('=');
      if (equal_pos == std::string::npos) {
        if (line.front() == '[' and line.back() == ']'){
          cur_section = trim(line, "[]"); }
      } else {
        m_config.at(cur_section).at(trim(line.substr(0, equal_pos))) = trim(line.substr(equal_pos + 1)); }
    }
    SetSection(section);
  }

  bool Configuration::SetSection(const std::string & section) const {
    map_t::const_iterator i = m_config.find(section);
    if (i == m_config.end()) return false;
    m_section = section;
    m_cur = const_cast<section_t*>(&i->second);
    return true;
  }

  bool Configuration::SetSection(const std::string & section) {
    m_section = section;
    m_cur = &m_config[section];
    return true;
  }

  std::string Configuration::Get(const std::string & key, const std::string & def) const {
    try {
      return GetString(key);
    } catch (const Exception &) {
      // ignore: return default
    }
    return def;
  }

  double Configuration::Get(const std::string & key, double def) const {
    try {
      return from_string(GetString(key), def);
    } catch (const Exception &) {
      // ignore: return default
    }
    return def;
  }

  int64_t Configuration::Get(const std::string & key, int64_t def) const {
    try {
      std::string s = GetString(key);
#if EUDAQ_PLATFORM_IS(CYGWIN) || EUDAQ_PLATFORM_IS(WIN32)
      // Windows doesn't have strtoll, so just use stoll for now
      return std::stoll(s);
#else
      return std::strtoll(s.c_str(), 0, 0);
#endif
    } catch (const Exception &) {
      // ignore: return default
    }
    return def;
  }
uint64_t Configuration::Get(const std::string & key, uint64_t def) const {
	  try {
		  std::string s = GetString(key);
#if EUDAQ_PLATFORM_IS(CYGWIN) || EUDAQ_PLATFORM_IS(WIN32)
		  // Windows doesn't have strtull, so just use stoull for now
		  return std::stoull(s);
		  
#else
		  return std::strtoull(s.c_str(), 0, 0);
#endif
	  } catch (const Exception &) {
		  // ignore: return default
	  }
	  return def;
  }



  int Configuration::Get(const std::string & key, int def) const {
    try {
      std::string s = GetString(key);
      return std::strtol(s.c_str(), 0, 0);
    } catch (const Exception &) {
      // ignore: return default
    }
    return def;
  }

  void Configuration::Print(std::ostream& out) const{
     out<<"ConfigurationFile:"<<std::endl;
     out<<" Sections: ";
     for (auto& i: this->m_config)
         out<<i.first<<" ";
     out<<std::endl;
     out << " CurrentSection: '" << this->GetSection() << "'" << std::endl;
     for (section_t::iterator it = m_cur->begin(); it!=m_cur->end(); ++it){
         out <<"  "<< it->first << " : " << it->second << std::endl;
    }
  }


  void Configuration::Print() const 
  {
    Print(std::cout);
  }

  std::string Configuration::GetString(const std::string & key) const {
    section_t::const_iterator i = m_cur->find(key);
    if (i != m_cur->end()) {
      return i->second;
    }
    throw Exception("Configuration: key not found");
  }

  std::vector<std::string> Configuration::GetSections() const {
      std::vector<std::string> sections;
      for (auto& i: this->m_config){
          sections.push_back(i.first);
      }
      return sections;
  }

  void Configuration::SetString(const std::string & key, const std::string & val) {
    (*m_cur)[key] = val;
  }

  void Configuration::PrintSectionNames() const {
      PrintSectionNames(std::cout);
  }

  void Configuration::PrintSectionNames(std::ostream& out) const {
      out<<"SectionNames:";
      for (map_t::const_iterator it = m_config.begin(); it!=m_config.end();it++)
          out<<" "<<it->first;
      out<<std::endl;
  }
  void Configuration::PrintKeys(std::ostream& out) const{
      return PrintKeys(out,m_section);
  }
  void Configuration::PrintKeys(std::ostream& out, const std::string section) const {
              out<<" Keys in "<<section<<": ";
      section_t sec = (m_config.at(section));
      for (section_t::const_iterator it = sec.begin(); it!=sec.end();it++)
          out<<" \""<<it->first<<"\"/"<<it->second;
      out<<std::endl;
  }

  void Configuration::PrintKeys(const std::string section) const {
        PrintKeys(std::cout,section);
    }

  void Configuration::PrintKeys() const {
      PrintKeys(m_section);
  }
  std::vector<std::string> Configuration::GetKeys(std::string section) {
      std::vector<std::string> keys;
      section_t sec = (m_config.at(section));
      for (section_t::const_iterator it = sec.begin(); it!=sec.end();it++)
          keys.push_back(it->first);
      return keys;

  }

}
