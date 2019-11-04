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
    std::vector<unsigned> numbers2 = parsenumbers(events.Value());
    std::sort(numbers2.begin(),numbers2.end());
    eudaq::multiFileReader reader2(!async.Value());
    for (size_t i = 0; i < op.NumArgs(); ++i) {
      reader2.addFileReader(op.GetArg(i), ipat.Value());
	}
    Configuration config2("");
    if (configFileName.Value() != ""){
        std::cout << "Read config file: "<<configFileName.Value()<<std::endl;
        std::ifstream file2(configFileName.Value().c_str());
        if (file2.is_open()) {
          config2.Load(file2,"");
          std::string name = configFileName.Value().substr(0, configFileName.Value().find("."));
          config2.Set("Name",name);
        } else {
          std::cout<<"Unable to open file '" << configFileName.Value() << "'" << std::endl;
        }
        file2.close();
    }
      std::shared_ptr<eudaq::FileWriter> writer2(FileWriterFactory::Create(type.Value(),&config2));
      writer2->setTU(reader2.hasTUEvent());
      writer2->SetConfig(&config2);
      writer2->SetFilePattern(opat.Value());
      writer2->StartRun2(reader2.RunNumber());
      ProgressBar bla2(uint32_t(writer2->GetMaxEventNumber()));
	    uint32_t event_nr = 0;

      do {
        if ( !numbers2.empty() && reader2.GetDetectorEvent().GetEventNumber()>numbers2.back() ) {
          break;
        } else if (reader2.GetDetectorEvent().IsBORE() || reader2.GetDetectorEvent().IsEORE() || numbers2.empty() ||
                  std::find(numbers2.begin(), numbers2.end(), reader2.GetDetectorEvent().GetEventNumber()) != numbers2.end()) {
          writer2->WriteEvent2(reader2.GetDetectorEvent());
          if(dbg>0)std::cout<< "writing one more event" << std::endl;
          ++event_nr;
          if (writer2->GetMaxEventNumber()){
              if(event_nr == writer2->GetMaxEventNumber() + 1)
                  writer2->GetStats(reader2.GetDetectorEvent());
            bla2.update(event_nr);
          }
          else
          if (event_nr % 1000 == 0) std::cout<<"\rProcessing event: "<< std::setfill('0') << std::setw(7) << event_nr << " " << std::flush;
        }
      } while (reader2.NextEvent() && (writer2->GetMaxEventNumber() <= 0 || event_nr <= writer2->GetMaxEventNumber()));// Added " && (writer->GetMaxEventNumber() <= 0 || event_nr <= writer->GetMaxEventNumber())" to prevent looping over all events when desired: DA

      writer2->TempFunction();
      std::vector<float> l1Offs = writer2->TempFunctionL1Off();
      std::cout << "Offsets L1: ";
      for(size_t it = 0; it < l1Offs.size(); it++)
          std::cout << float(l1Offs.at(it)) << ", ";
      std::cout << std::endl;
      std::vector<float> decOffs = writer2->TempFunctionDecOff();
      std::cout << "Offsets decOffsets: ";
      for(size_t it = 0; it < decOffs.size(); it++)
          std::cout << float(decOffs.at(it)) << ", ";
      std::cout << std::endl;


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
      std::string tempBla (configFileName.Value());
      std::string tempConvType ("Converter.");
      tempConvType.append(type.Value());
      tempBla.append(".dasb");
      if (configFileName.Value() != ""){
          std::cout << "Read base config file: "<<configFileName.Value()<<std::endl;
          std::ifstream file(configFileName.Value().c_str());
          if (file.is_open()) {
              config.Load(file,"");
              std::string name = configFileName.Value().substr(0, configFileName.Value().find("."));
              config.Set("Name",name);
              config.SetSection(tempConvType);
              config.Set("decoding_l1_offset_v", l1Offs);
              config.Set("decoding_offset_v", decOffs);
              std::ofstream filebla(tempBla.c_str());
              config.Save(filebla);
          } else {
              std::cout<<"Unable to open file '" << configFileName.Value() << "'" << std::endl;
          }
          file.close();
      }
      Configuration config3("");
      if(tempBla != ""){
          std::cout << "Read modified config file: "<<tempBla<<std::endl;
          std::ifstream filebla2(tempBla.c_str());
          if (filebla2.is_open()) {
              config3.Load(filebla2,"");
              std::string name = tempBla.substr(0, configFileName.Value().find("."));
              config3.Set("Name",name);
              config3.SetSection(tempConvType);
//              config3.PrintKeys(tempConvType);
              config3.Print();
          } else {
              std::cout<<"Unable to open file '" << configFileName.Value() << "'" << std::endl;
          }
          filebla2.close();
      }

      std::shared_ptr<eudaq::FileWriter> writer(FileWriterFactory::Create(type.Value(),&config3));
      writer->setTU(reader.hasTUEvent());
      writer->SetConfig(&config3);
      writer->SetFilePattern(opat.Value());
      writer->StartRun(reader.RunNumber());
      ProgressBar bla(uint32_t(writer->GetMaxEventNumber()));
      event_nr = 0;
      do {
        if ( !numbers.empty() && reader.GetDetectorEvent().GetEventNumber()>numbers.back() ) {
          break;
        } else if (reader.GetDetectorEvent().IsBORE() || reader.GetDetectorEvent().IsEORE() || numbers.empty() ||
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
