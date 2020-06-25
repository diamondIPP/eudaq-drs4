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
    print_banner("STEP 1: Calculating decoding offsets ...", '~');
    Configuration config(configFileName.Value(), "Converter.telescopetree", true);
    if (config.Get("decoding_offset_v", "").empty()) {  // only run the decoder if the config file does not already have entries
      std::shared_ptr<eudaq::FileWriter> decoder(FileWriterFactory::Create("cmsdecoder", &config));
      decoder->StartRun(reader2.RunNumber());
      ProgressBar pbar(uint32_t(decoder->GetMaxEventNumber()));
      uint32_t event_nr = 0;

      do {
        if (!numbers2.empty() && reader2.GetDetectorEvent().GetEventNumber() > numbers2.back()) {
          break;
        }
        if (reader2.GetDetectorEvent().IsBORE() || reader2.GetDetectorEvent().IsEORE() || numbers2.empty() ||
            std::find(numbers2.begin(), numbers2.end(), reader2.GetDetectorEvent().GetEventNumber()) != numbers2.end()) {
          decoder->WriteEvent(reader2.GetDetectorEvent());
          if (dbg > 0) { std::cout << "writing one more event" << std::endl; }
          ++event_nr;
          if (event_nr == decoder->GetMaxEventNumber() + 1) { decoder->GetStats(reader2.GetDetectorEvent()); }
          pbar.update(event_nr);
        }
      } while (reader2.NextEvent() && (decoder->GetMaxEventNumber() <= 0 || event_nr <=
                                                                            decoder->GetMaxEventNumber()));// Added " && (writer->GetMaxEventNumber() <= 0 || event_nr <= writer->GetMaxEventNumber())" to prevent looping over all events when desired: DA

      decoder->Run(); // Calculate Level1, decoding offsets and timing compensations (alphas)

      /** ------------------------------------------------------
       * Get and save the decoding parameters to the config file */
      std::vector<float> black_offsets = decoder->GetBlackOffsets();
      std::vector<float> level1_offsets = decoder->GetLeve1Offsets();
      std::vector<float> alphas = decoder->GetAlphas();
      std::cout << "Calculated decoding offets: " << to_string(black_offsets, ", ", 0, 3) << std::endl;
      std::cout << "Calculated levels 1: " << to_string(level1_offsets, ", ", 0, 2) << std::endl;
      std::cout << "Calculated time compensations (alphas): " << to_string(alphas, ", ", 0, 2) << std::endl;
      config.SetSection("Converter.telescopetree");
      config.Set("decoding_offset_v", to_string(black_offsets, ",", 0, 3));
      config.Set("decoding_l1_v", to_string(level1_offsets, ",", 0, 2));
      config.Set("decoding_alphas_v", to_string(alphas, ",", 0, 2));
      config.Save();
      config.SetSection("Converter." + type.Value());
      decoder.reset();
    }
    std::cout << "\n... STEP 1 done in " << std::setprecision(1) << elapsed_time(start) << " s" << std::endl;
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
      writer->setTU(reader.hasTUEvent());
//      writer->SetConfig(&config);
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
