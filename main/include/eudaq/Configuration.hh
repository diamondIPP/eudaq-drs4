#ifndef EUDAQ_INCLUDED_Configuration
#define EUDAQ_INCLUDED_Configuration

#include "eudaq/Utils.hh"
#include "eudaq/Exception.hh"
#include "eudaq/Platform.hh"
#include <iostream>
#include <string>
#include <map>


namespace eudaq {

  class DLLEXPORT Configuration {
    public:
      Configuration(const std::string & config = "", const std::string & section = "");
      Configuration(std::istream & conffile, const std::string & section = "");
      Configuration(const std::string &, const std::string &, bool);
      Configuration(const Configuration & other);
      void Save(std::ostream & file) const;
      void Save(const std::string&) const;
      void Save() const;
      void Load(std::istream & file, const std::string & section);
      void Read(const std::string &, const std::string & section = "");
      bool SetSection(const std::string & section) const;
      bool SetSection(const std::string & section);
      unsigned NSections(){return m_config.size();}
      unsigned NKeys(){ return m_config[m_section].size();}
      unsigned NKeys(std::string section){return m_config[section].size();}
      std::vector<std::string> GetSections() const;
      std::vector<std::string> GetKeys(){return GetKeys(m_section);};
      std::vector<std::string> GetKeys(std::string section);
      std::string GetSection() const {return m_section;}
      std::string operator [] (const std::string & key) const { return GetString(key); }
      std::string Get(const std::string & key, const std::string & def) const;
      double Get(const std::string & key, double def) const;
      int64_t Get(const std::string & key, int64_t def) const;
	  uint64_t Get(const std::string & key,uint64_t def) const;
	  template <typename T>
	  T Get(const std::string &key, T def) const {
	      std::string str = Get(key,to_string(def));
		  return eudaq::from_string(str,  def);
	  }
      int Get(const std::string & key, int def) const;
      template <typename T>
        T Get(const std::string & key, const std::string fallback, const T & def) const {
          return Get(key, Get(fallback, def));
        }
		std::string Get(const std::string & key, const char * def) const{
			std::string ret(Get(key,std::string(def)));
			return ret;
		}
      std::string Get(const std::string & key, const std::string fallback, std::string def) const {
        return Get(key, Get(fallback, def));
      }
      //std::string Get(const std::string & key, const std::string & def = "");
      template <typename T>
        void Set(const std::string & key, const T & val);
      std::string Name() const;
      Configuration & operator = (const Configuration & other);
      void Print(std::ostream & out) const;
      void Print() const;
      void PrintSectionNames(std::ostream& out) const;
      void PrintSectionNames() const;
      void PrintKeys(std::ostream& out, const std::string section) const;
      void PrintKeys(std::ostream& out) const;
      void PrintKeys(const std::string section) const;
      void PrintKeys() const;
    private:
      std::string GetString(const std::string & key) const;
      void SetString(const std::string & key, const std::string & val);
      typedef std::map<std::string, std::string> section_t;
      typedef std::map<std::string, section_t> map_t;
      map_t m_config;
      mutable std::string m_section;
      mutable section_t * m_cur;
      std::string m_file_name;
  };

  inline std::ostream & operator << (std::ostream & os, const Configuration & c) {
    c.Save(os);
    return os;
  }

  template <typename T>
    inline void Configuration::Set(const std::string & key, const T & val) {
      SetString(key, to_string(val));
    }

}

#endif // EUDAQ_INCLUDED_Configuration
