#include <eudaq/PluginManager.hh>
#include "eudaq/FileWriter.hh"
#include "eudaq/OptionParser.hh"
#include "eudaq/Logger.hh"
#include "eudaq/MultiFileReader.hh"
#include "iomanip"

using namespace eudaq;
unsigned dbg = 0;

int main(int /*unused*/, char ** argv) {
  std::clock_t start = std::clock();

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
    std::sort(numbers2.begin(), numbers2.end());
    eudaq::multiFileReader reader2(!async.Value());
    for (size_t i = 0; i < op.NumArgs(); ++i) {
      reader2.addFileReader(op.GetArg(i), ipat.Value());
    }
    /** -----------------------------------------------
     * First step: Find analogue decoding in 100k events:
     * -----------------------------------------------*/
    Configuration config(configFileName.Value(), "Converter.telescopetree", true);
    if (config.Get("decoding_offset_v", "").empty()) {  // only run the decoder if the config file does not already have entries
      print_banner("STEP 1: Calculating decoding offsets ...", '~');
      std::shared_ptr<eudaq::FileWriter> decoder(FileWriterFactory::Create("cmsdecoder", &config));
      decoder->StartRun(reader2.RunNumber());

      ProgressBar pbar(uint32_t(decoder->GetMaxEventNumber()));
      uint32_t event_nr = 0;
      size_t max_decoder = decoder->GetMaxEventNumber() <= 0 ? UINT32_MAX: decoder->GetMaxEventNumber();
      size_t max_number = numbers2.empty() ? UINT32_MAX : numbers2.back();
      DetectorEvent dev = reader2.GetDetectorEvent();
      while (reader2.NextEvent() and event_nr < std::min(max_decoder, max_number)) {
        event_nr = dev.GetEventNumber();
        decoder->WriteEvent(dev);
        pbar.update(event_nr);
        dev = reader2.GetDetectorEvent();
      }
      decoder->GetStats(reader2.GetDetectorEvent());
      decoder->Run(); // Calculate Level1, decoding offsets and timing compensations (alphas)

      /** Save the decoding parameters to the config file */
      decoder->PrintResults();
      decoder->SaveResults();
      decoder.reset();
      std::cout << "\n... STEP 1 done in " << std::setprecision(1) << elapsed_time(start) << " s" << std::endl;
    }
    start = clock();

    /** -----------------------------------------------
     * Second step: Conversion
     * -----------------------------------------------*/
      std::vector<unsigned> numbers = parsenumbers(events.Value());
      std::sort(numbers.begin(), numbers.end());
      eudaq::multiFileReader reader(!async.Value());
      for (size_t i = 0; i < op.NumArgs(); ++i) {
          reader.addFileReader(op.GetArg(i), ipat.Value());
      }

      print_banner("STARTING EUDAQ " + to_string(type.Value()) + " CONVERTER");

      std::shared_ptr<eudaq::FileWriter> writer(FileWriterFactory::Create(type.Value(), &config));
      writer->SetTU(reader.hasTUEvent());
      writer->SetFilePattern(opat.Value());
      writer->StartRun(reader.RunNumber());
      auto pbar = ProgressBar(uint32_t(writer->GetMaxEventNumber()));
      auto event_nr = 0;
      do {
        if ( !numbers.empty() && reader.GetDetectorEvent().GetEventNumber()>numbers.back() )
        { break; }
        if (reader.GetDetectorEvent().IsBORE() || reader.GetDetectorEvent().IsEORE() || numbers.empty() ||
        std::find(numbers.begin(), numbers.end(), reader.GetDetectorEvent().GetEventNumber()) != numbers.end()) {
          writer->WriteEvent(reader.GetDetectorEvent());
          ++event_nr;
          if (writer->GetMaxEventNumber() != 0){
            if (event_nr == writer->GetMaxEventNumber() + 1)
            { writer->GetStats(reader.GetDetectorEvent()); }
            pbar.update(event_nr);
          }
          else
          if (event_nr % 1000 == 0) { std::cout<<"\rProcessing event: "<< std::setfill('0') << std::setw(7) << event_nr << " " << std::flush; }
        }
      } while (reader.NextEvent() && (writer->GetMaxEventNumber() <= 0 || event_nr <= writer->GetMaxEventNumber()));// Added " && (writer->GetMaxEventNumber() <= 0 || event_nr <= writer->GetMaxEventNumber())" to prevent looping over all events when desired: DA
    if(dbg>0) { std::cout<< "no more events to read" << std::endl; }
    
  } catch (...) {
    std::cout << "Time: " << elapsed_time(start) << " s" << std::endl;
    return op.HandleMainException();
  }
    std::cout << "Time: " << elapsed_time(start) << " s" << std::endl;
  if(dbg>0) { std::cout<< "almost done with Converter. exiting" << std::endl; }
  return 0;
}
