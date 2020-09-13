#include "CMSPixelProducer.hh"

#include "helper.h"
#include "eudaq/Logger.hh"

#include <iostream>
#include <ostream>
#include <vector>

using namespace pxar;
using namespace std;

std::vector<int32_t> &CMSPixelProducer::split(const std::string &s, char delim, std::vector<int32_t> &elems) {
  std::stringstream ss(s);
  std::string item;
  int32_t def = 0;
  while (getline(ss, item, delim) != nullptr) {
    elems.push_back(eudaq::from_string(item,def));
  }
  return elems;
}
std::vector<int32_t> CMSPixelProducer::split(const std::string &s, char delim) {
  std::vector<int32_t> elems;
  split(s, delim, elems);
  return elems;
}

std::string CMSPixelProducer::prepareFilename(const std::string & name, const std::string & n) {
  return m_config.Get("directory", "") + name + "Parameters" + m_config.Get("trim_value", "") + "_C" + n + ".dat";
}

void CMSPixelProducer::ReadPxarConfig() {
  /** get parameters from pxar configParameters.dat */
  ifstream file(m_config.Get("directory", "") + "configParameters.dat");
  string line;
  map<string, string> config;
  while(getline(file, line) != nullptr) {
    if (line.empty()) { continue; }
    if (line.at(0) == '#' or line.at(0) == ' ' or line.at(0) == '-' or line.size() < 3)  { continue; }
    size_t pos = line.find_first_of(": ");
    config[line.substr(0, pos)] = line.substr(pos + 1);
  }
  m_pxar_config = config;
}

std::vector<int8_t> CMSPixelProducer::GetI2Cs() {
  /** get i2cs from pxar configParameters.dat */
  vector<int8_t> i2cs;
  uint8_t i(0);
  for (const auto & i2c: eudaq::split(eudaq::split(m_pxar_config.at("nRocs"), ":").at(1), ",")) {
    i2cs.emplace_back(stoi(i2c));
    m_i2c_map[(stoi(i2c))] = i++; }
  if (i2cs.empty()) { EUDAQ_ERROR("Did not understand i2cs from pxar configParameters.dat!"); }
  return i2cs;
}

std::vector<std::pair<std::string,uint8_t> > CMSPixelProducer::GetConfDACs(int16_t i2c, bool tbm) {

  std::string regname = (tbm ? "TBM" : "DAC");

  std::string filename;
  // Read TBM register file, Core A:
  if(tbm && i2c < 1) { filename = prepareFilename("tbm","0a"); }
    // Read TBM register file, Core B:
  else if(tbm && i2c >= 1) { filename = prepareFilename("tbm","0b"); }
    // Read ROC DAC file, no I2C address indicator is given, assuming filename is correct "as is":
  else if(i2c < 0) { filename = m_config.Get("dacFile", ""); }
    // Read ROC DAC file, I2C address is given, appending a "_Cx" with x = I2C:
  else { filename = prepareFilename("dac", std::to_string(i2c)); }

  std::vector<std::pair<std::string,uint8_t> > dacs;
  std::ifstream file(filename);
  size_t overwritten_dacs = 0;

  if(!file.fail()) {
    EUDAQ_INFO(string("Reading ") + regname + string(" settings from file \"") + filename + string("\"."));
    std::cout << "Reading " << regname << " settings from file \"" << filename << "\"." << std::endl;

    std::string line;
    while(std::getline(file, line)) {
      std::stringstream   linestream(line);
      std::string         name;
      int                 dummy, value;
      linestream >> dummy >> name >> value;

      // Check if the first part read was really the register:
      if(name.empty()) {
        // Rereading with only DAC name and value, no register:
        std::stringstream newstream(line);
        newstream >> name >> value;
      }

      // Convert to lower case for cross-checking with eduaq config:
      std::transform(name.begin(), name.end(), name.begin(), ::tolower);

      // Check if reading was correct:
      if(name.empty()) {
        EUDAQ_ERROR(string("Problem reading DACs from file \"") + filename + string("\": DAC name appears to be empty.\n"));
        throw pxar::InvalidConfig("WARNING: Problem reading DACs from file \"" + filename + "\": DAC name appears to be empty.");
      }

      // Skip the current limits that are wrongly placed in the DAC file sometimes (belong to the DTB!)
      if(name == "vd" || name == "va") { continue; }

      // Check if this DAC is overwritten by directly specifying it in the config file:
      int old_value = value;
      if(m_config.Get(name, -1) != -1) {
        // value = m_config.Get(name, -1);
        auto values = split(m_config.Get(name, "-1"),' ');
        value = values.size() > 1 ? values.at(m_i2c_map.at(i2c)) : values[0];
        std::cout << "Overwriting DAC " << name << " from file value: " << old_value << " -> " << value << std::endl;
        EUDAQ_INFO("Overwriting DAC " + name + " from file value: " + std::to_string(old_value) + " -> " + std::to_string(value));
        overwritten_dacs++;
      }

      dacs.emplace_back(name, value);
      m_alldacs.append(name + " " + std::to_string(value) + "; ");
    }

    EUDAQ_EXTRA(string("Successfully read ") + std::to_string(dacs.size())
                + string(" DACs from file, ") + std::to_string(overwritten_dacs) + string(" overwritten by config."));
  }
  else {
    if(tbm) throw pxar::InvalidConfig("Could not open " + regname + " file.");

    std::cout << "Could not open " << regname << " file \"" << string(filename) << "\"." << std::endl;
    EUDAQ_ERROR(string("Could not open ") + regname + string(" file \"") + filename + string("\"."));
    EUDAQ_INFO(string("If DACs from configuration should be used, remove dacFile path."));
    throw pxar::InvalidConfig("Could not open " + regname + " file.");
  }

  return dacs;
}

std::vector<pxar::pixelConfig> CMSPixelProducer::GetConfMaskBits() {

  // Read in the mask bits:
  std::vector<pxar::pixelConfig> maskbits;
  return maskbits;
}

std::vector<pxar::pixelConfig> CMSPixelProducer::GetConfTrimming(std::vector<pxar::pixelConfig> maskbits, int16_t i2c) {

  std::string filename;
  // No I2C address indicator is given, assuming filename is correct "as is":
  if(i2c < 0) { filename = m_config.Get("trimFile", ""); }
    // I2C address is given, appending a "_Cx" with x = I2C:
  else { filename = prepareFilename("trim", std::to_string(i2c)); }

  std::vector<pxar::pixelConfig> pixels;
  std::ifstream file(filename);
  if(!file.fail()) {
    std::string line;
    while(std::getline(file, line)) {
      std::stringstream   linestream(line);
      std::string         dummy;
      int                 trim, col, row;
      linestream >> trim >> dummy >> col >> row;
      pixels.push_back(pxar::pixelConfig(col,row,trim,false,false));
    }
    m_trimmingFromConf = true;
  }
  else {
    std::cout << "Couldn't read trim parameters from \"" << string(filename) << "\". Setting all to 15." << std::endl;
    EUDAQ_WARN(string("Couldn't read trim parameters from \"") + string(filename) + ("\". Setting all to 15.\n"));
    for(int col = 0; col < 52; col++) {
      for(int row = 0; row < 80; row++) {
        pixels.push_back(pxar::pixelConfig(col,row,15,false,false));
      }
    }
    m_trimmingFromConf = false;
  }

  // Process the mask bit list:
  for(std::vector<pxar::pixelConfig>::iterator px = pixels.begin(); px != pixels.end(); px++) {

    // Check if this pixel is part of the maskbit vector:
    std::vector<pxar::pixelConfig>::iterator maskpx = std::find_if(maskbits.begin(),
                                                                   maskbits.end(),
                                                                   findPixelXY(px->column(), px->row(), i2c < 0 ? 0 : i2c));
    // Pixel is part of mask vector, set the mask bit:
    if(maskpx != maskbits.end()) { px->setMask(true); }
  }

  if(m_trimmingFromConf) {
    EUDAQ_EXTRA(string("Trimming successfully read from ") + m_config.Name() + string(": \"") + string(filename) + string("\"\n"));
  }
  return pixels;
} // GetConfTrimming

vector<masking> CMSPixelProducer::GetConfMask(){

  string mask_name = m_config.Get("mask_name", "");
  string filename = mask_name.empty() ? m_config.Get("maskFile", "") : m_config.Get("directory", "") + mask_name;
  vector<masking>  mask;
  ifstream file(filename);

  if(!file.fail()){
    std::string line;
    while(std::getline(file, line)){
      stringstream   linestream(line);
      string         identifier;
      uint16_t       roc, col, row;
      linestream >> identifier >> roc >> col >> row;
      mask.push_back(masking(identifier, roc, col, row));

    }
    m_maskingFromConf = true;
  }
  else {
    cout << "Couldn't read mask parameters from \"" << string(filename) << "\". So I don't know how to mask ;-)\n";
    EUDAQ_WARN(string("Couldn't read mask parameters from \"") + string(filename) + ("\". So I don't know how to mask ;-)\n"));
    m_maskingFromConf = false;
  }

  cout << "\033[1;32;48mFILENAME: \033[0m" <<  filename << "\n";

  return mask;
}

std::vector<std::pair<std::string, uint8_t> > CMSPixelProducer::GetTestBoardDelays() {
  /** read in the testboard parameters from the pxar config file and overwrite provided option in the eudaq config file */
  string filename = m_config.Get("directory", "") + m_pxar_config.at("tbParameters");
  EUDAQ_INFO("Reading testboard delays from file: " + filename);
  ifstream file(filename);
  string line, name;
  int dummy, value, overwritten_dacs(0);
  vector<pair<string, uint8_t> > delays;
  while (getline(file, line) != nullptr) {
    stringstream ss(line);
    ss >> dummy >> name >> value;
    if (name.empty()) { continue; }
    if (m_config.Get(name, -1) != -1) {
      std::cout << "Overwriting testboard delay " << name << " from file value: " << value << " -> " << m_config.Get(name, -1) << endl;
      EUDAQ_INFO("Overwriting testboard delay " + name + " from file value: " + to_string(value) + " -> " + m_config.Get(name, ""));
      value = m_config.Get(name, -1);
      overwritten_dacs++;
    }
    delays.emplace_back(name, value);
  }
  EUDAQ_EXTRA("Successfully read " + std::to_string(delays.size()) + " testboard delays from file, " + to_string(overwritten_dacs) + " overwritten by config.");
  return delays;
}


