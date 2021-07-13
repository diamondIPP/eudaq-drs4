//
// Created by reichmann on 07.04.16.
//
#ifndef EUDAQ_FILEWRITETREEDRS4_HH
#define EUDAQ_FILEWRITETREEDRS4_HH

#include "eudaq/FileWriterWF.hh"

namespace eudaq {

    class FileWriterTreeDRS4 : public FileWriterWF {
    public:
      explicit FileWriterTreeDRS4(const std::string &);
      ~FileWriterTreeDRS4() override = default;
      void AddInfo(uint8_t, const StandardWaveform*) override;
      void ExtractForcTiming();
      float LoadSamplingFrequency(const DetectorEvent &) override;
    };
}

#endif //EUDAQ_FILEWRITETREEDRS4_HH
