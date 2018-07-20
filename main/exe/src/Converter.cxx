#include <eudaq/PluginManager.hh>
#include "eudaq/FileReader.hh"
#include "eudaq/FileWriter.hh"
#include "eudaq/OptionParser.hh"
#include "eudaq/Logger.hh"
#include "eudaq/MultiFileReader.hh"

using namespace eudaq;
unsigned dbg = 0;

int main(int, char ** argv) {
  std::clock_t    start;
  start = std::clock();

  eudaq::OptionParser op("EUDAQ File Converter", "1.0", "", 1);
  eudaq::Option<std::string> type(op, "t", "type", "native", "name", "Output file type");
  eudaq::Option<std::string> events(op, "e", "events", "", "numbers", "Event numbers to convert (eg. '1-10,99' default is all)");
  eudaq::Option<std::string> ipat(op, "i", "inpattern", "../data/run$6R.raw", "string", "Input filename pattern");
  eudaq::Option<std::string> opat(op, "o", "outpattern", "test$6R$X", "string", "Output filename pattern");
  eudaq::OptionFlag async(op, "a", "nosync", "Disables Synchronisation with TLU events");
  eudaq::Option<size_t> syncEvents(op, "n" ,"syncevents",1000,"size_t","Number of events that need to be synchronous before they are used");
  eudaq::Option<uint64_t> syncDelay(op, "d" ,"longDelay",20,"uint64_t","us time long time delay");
  eudaq::Option<std::string> level(op, "l", "log-level", "INFO", "level", "The minimum level for displaying log messages locally");
  eudaq::Option<std::string> configFileName(op,"c","config", "", "string","Configuration filename");
  op.ExtraHelpText("Available output types are: " + to_string(eudaq::FileWriterFactory::GetTypes(), ", "));

  try {
    op.Parse(argv);
    EUDAQ_LOG_LEVEL(level.Value());
    std::vector<unsigned> numbers = parsenumbers(events.Value());
    std::sort(numbers.begin(),numbers.end());
    eudaq::multiFileReader reader(!async.Value());
    for (size_t i = 0; i < op.NumArgs(); ++i) {
      reader.addFileReader(op.GetArg(i), ipat.Value());
	}
    std::stringstream message;
    message << "STARTING EUDAQ " << to_string(type.Value()) << " CONVERTER";
    print_banner(message.str());
    Configuration config("");
    if (configFileName.Value() != ""){
        std::cout << "Read config file: "<<configFileName.Value()<<std::endl;
        std::ifstream file(configFileName.Value().c_str());
        if (file.is_open()) {
          config.Load(file,"");
          std::string name = configFileName.Value().substr(0, configFileName.Value().find("."));
          config.Set("Name",name);
        } else {
          std::cout<<"Unable to open file '" << configFileName.Value() << "'" << std::endl;
        }
    }
      std::shared_ptr<eudaq::FileWriter> writer(FileWriterFactory::Create(type.Value(),&config));
      writer->setTU(reader.hasTUEvent());
      writer->SetConfig(&config);
      writer->SetFilePattern(opat.Value());
      writer->StartRun(reader.RunNumber());
      ProgressBar bla(uint32_t(writer->GetMaxEventNumber()));
	  uint32_t event_nr=0;
      do {
		  if (!numbers.empty()&&reader.GetDetectorEvent().GetEventNumber()>numbers.back())
		  {
			break;
		  }else if (reader.GetDetectorEvent().IsBORE() || reader.GetDetectorEvent().IsEORE() || numbers.empty() ||
				std::find(numbers.begin(), numbers.end(), reader.GetDetectorEvent().GetEventNumber()) != numbers.end()) {
        writer->WriteEvent(reader.GetDetectorEvent());
        if(dbg>0)std::cout<< "writing one more event" << std::endl;
        ++event_nr;
        if (writer->GetMaxEventNumber()){
          if (event_nr == writer->GetMaxEventNumber() + 1)
            writer->GetStats(reader.GetDetectorEvent());
          bla.update(event_nr);
        }
        else
          if (event_nr % 1000 == 0) std::cout<<"\rProcessing event: "<< std::setfill('0') << std::setw(7) << event_nr << " " << std::flush;
      }
      } while (reader.NextEvent() && (writer->GetMaxEventNumber() <= 0 || event_nr <= writer->GetMaxEventNumber()));// Added " && (writer->GetMaxEventNumber() <= 0 || event_nr <= writer->GetMaxEventNumber())" to prevent looping over all events when desired: DA
      if(dbg>0)std::cout<< "no more events to read" << std::endl;
    
  } catch (...) {
	    std::cout << "Time: " << (std::clock() - start) / (double)(CLOCKS_PER_SEC / 1000) << " ms" << std::endl;
    return op.HandleMainException();
  }
    std::cout << "Time: " << (std::clock() - start) / (double)(CLOCKS_PER_SEC / 1000) << " ms" << std::endl;
  if(dbg>0)std::cout<< "almost done with Converter. exiting" << std::endl;
  return 0;
}
