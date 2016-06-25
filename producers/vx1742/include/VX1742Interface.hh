/* --------------------------------------------------------------------------------------------
** CAEN VX1742 implementation into the EUDAQ framework
** 
**
** <VX1742Interface>.hh
** 
** Date: April 2016
** Remarks: This is not a stand-alone driver, which was specifically written for our needs.
** Author: Christian Dorfer (dorfer@phys.ethz.ch)
** -------------------------------------------------------------------------------------------- */


#ifndef VX1742INTERFACE_HH
#define VX1742INTERFACE_HH

class VX1742Event;

// VME includes
#include "RCDVme/RCDVme.h"
#include "RCDVme/RCDCmemSegment.h"
#include "VX1742DEFS.hh"


class VX1742Interface{

  private:
    RCD::VME *vme;
    RCD::VMEMasterMap *vmm;
    unsigned long virtual_address;
    RCD::CMEMSegment* seg;
    vmec::vx1742_regs_t *vx1742;

    //correction data related
    vmec::drs4_correction corrTable[vmec::VX1742_GROUPS][vmec::VX1742_DRS4_COUNT];
    bool correctionDataInitialized[4];
    uint32_t FLASH(uint32_t n){return  (0x10D0 | ( n << 8));}  // base address of the flash memory (first byte)
    uint32_t SEL_FLASH(uint32_t n){return  (0x10CC | ( n << 8));}  // flash enable/disable 
    uint32_t STATUS(uint32_t n){return  (0x1088 | ( n << 8));}  // status register


  public:
	VX1742Interface();
	~VX1742Interface(); //if mapping not yet destroyed do it
	void openVME(); //open vme and create master mapping
	void closeVME(); //close and destroy mapping

	std::string getSerialNumber();
	std::string getFirmwareVersion();
	std::string getDRS4FirmwareVersion();
	uint32_t isRunning();
	void startAcquisition();
	void stopAcquisition();
	void softwareReset();
	void clearBuffers();
	void printAcquisitionStatus();
	void printAcquisitionControl();
	void disableIndividualTriggers();

	void setSamplingFrequency(uint32_t param);
	uint32_t getSamplingFrequency();

	void setPostTriggerSamples(uint32_t param);
	uint32_t getPostTriggerSamples();

	bool SPIBusBusy(uint32_t grp);
	void enableTRn(uint32_t enable[2], uint32_t threshold[2], uint32_t offset[2], uint32_t polarity);

	void setCustomSize(uint32_t param);
	uint32_t getCustomSize();

	void setTriggerSource(uint32_t param);
	std::string getTriggerSource();

	void toggleGroups(uint32_t param[]);
	uint32_t getActiveChannels();
	//uint32_t getGroupMask(); fixme: implement and use it to send bore event

	void sendBusyToTRGout();

	void setTriggerCount();
	void setMaxBLTEvents(uint32_t param);
	bool eventReady();
	uint32_t getEventsStored();
	uint32_t getNextEventSize();

	//deal with correction data:
	uint32_t readFlashPage(uint32_t group, int8_t* page, uint32_t pagenumber);
	uint32_t loadDRS4CorrectionTables(uint32_t grp, uint32_t frequency);	
	uint32_t initializeDRS4CorrectionTables(uint32_t frequency);
	//GetTimeCorrectionTable


	void SetMaxBLTEvents(uint32_t param);
	uint32_t BlockTransferEventD64(VX1742Event *vxEvent);
};


#endif