#ifndef EUDAQ_INCLUDED_FileWriter
#define EUDAQ_INCLUDED_FileWriter

#include "eudaq/DetectorEvent.hh"
#include "eudaq/Configuration.hh"
#include <vector>
#include <string>

namespace eudaq {

  class DLLEXPORT FileWriter {
    public:
      explicit FileWriter(Configuration *config);
      FileWriter();
      void SetConfig(Configuration *config) {m_config=config;}
      virtual void Configure();
      virtual void StartRun(unsigned runnumber) = 0;
      virtual void WriteEvent(const DetectorEvent &) = 0;
      virtual void Run() {};
      virtual std::vector<float> GetBlackOffsets() { return std::vector<float>(0); }
      virtual std::vector<float> GetLeve1Offsets() { return std::vector<float>(0); }
      virtual std::vector<float> GetAlphas() { return std::vector<float>(0); }
      virtual uint64_t FileBytes() const = 0;
      void SetFilePattern(const std::string & p) { m_filepattern = p; }
      // method to get max event number: DA
      virtual uint64_t GetMaxEventNumber();
      virtual std::string GetStats(const DetectorEvent &) { return ""; }
      virtual void PrintResults() {};
      virtual void SaveResults() {};
      virtual void SetTU(bool) {};
      virtual ~FileWriter() = default;
    protected:
      std::string m_filepattern;
      Configuration* m_config;
  };


  class DLLEXPORT FileWriterFactory {
    public:
      static FileWriter * Create(const std::string & name, Configuration *config, const std::string & params = "");
      template <typename T>
        static void Register(const std::string & name) {
          do_register(name, filewriterfactory<T>);
        }
      typedef FileWriter * (*factoryfunc)(const std::string &);
      static std::vector<std::string> GetTypes();
    private:
      template <typename T>
        static FileWriter * filewriterfactory(const std::string & params) {
          return new T(params);
        }
      static void do_register(const std::string & name, factoryfunc);
  };

  template <typename T>
    class RegisterFileWriter {
      public:
        RegisterFileWriter(const std::string & name) {
          FileWriterFactory::Register<T>(name);
        }
    };

}

#endif // EUDAQ_INCLUDED_FileWriter
