/*
 * EventSanityChecker.cc
 *
 *  Created on: Jun 16, 2011
 *      Author: stanitz
 */

#include "EventSanityChecker.hh"
#include <iostream>

//default constructor
EventSanityChecker::EventSanityChecker()
{
  NPlanes=0;
  NWaveforms = 0;
}

//set the number of planes and frames
EventSanityChecker::EventSanityChecker(int nplanes, int nwaveforms)
{
  NPlanes=nplanes;
  NWaveforms=nwaveforms;
  std::cout << "##Number of waveforms set in EventSanityChecker constructor: " << NWaveforms << std::endl;
}

EventSanityChecker::~EventSanityChecker()
{
  // TODO Auto-generated destructor stub
}


unsigned int EventSanityChecker::getNPlanes() const
{
  return NPlanes;
}



void EventSanityChecker::setNPlanes(int NPlanes)
{
  this->NPlanes = NPlanes;
}



unsigned int EventSanityChecker::getNWaveforms() const
{
  return NWaveforms;
}



void EventSanityChecker::setNWaveforms(int NWaveforms)
{
  this->NWaveforms = NWaveforms;
  //std::cout << "##Number of waveforms set in EventSanityChecker set method: " << NWaveforms << std::endl;
}


