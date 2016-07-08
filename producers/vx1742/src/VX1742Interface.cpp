/* --------------------------------------------------------------------------------------------
** CAEN VX1742 implementation into the EUDAQ framework
** 
**
** <VX1742Interface>.cpp
** 
** Date: April 2016
** Remarks: This is not a stand-alone driver, which was specifically written for our needs.
** Author: Christian Dorfer (dorfer@phys.ethz.ch)
** -------------------------------------------------------------------------------------------- */

#include "VX1742Interface.hh"
#include "VX1742Event.hh"

#include <iostream>
#include <sstream>

using namespace RCD;

VX1742Interface::VX1742Interface(){
  vme = 0;
  vmm = 0;
  seg = 0;
  vx1742=0;
  virtual_address = 0;
}


VX1742Interface::~VX1742Interface(){
	//unmap, delete pointers, etc
}


void VX1742Interface::openVME(){
  try{
    std::cout << std::endl << "###Initialize VX1742 connection.. ";  
    vme = VME::Open();
    if((*vme)()){
      std::cout << std::endl << "ERROR opening VME object." << std::endl;
      vme->ErrorPrint((*vme)());
      std::exit(-1);
    }
    std::cout << "[OK]" << std::endl;

    std::cout << "###Create VME master mapping.. "; 
    vmm = vme->MasterMap(vmec::vmebus_address, vmec::window_size, vmec::address_modifier, vmec::options);
    if((*vmm)()) {
      std::cout << "ERROR opening VME master mapping." << std::endl;
      vme->ErrorPrint((*vmm)());
      std::exit(-1);
    }

    virtual_address = 0;
    vmm->VirtualAddress(&virtual_address);
    if((*vmm)() || virtual_address == 0) {
      std::cout << "ERROR returning virtual address." << std::endl;
      vme->ErrorPrint((*vmm)());
      std::exit(-1);
    }

    vx1742 = (vmec::vx1742_regs_t *) virtual_address;
    std::cout << "[OK]" << std::endl;
     
    //printf("checksum              is at 0x%08x\n", (size_t)&vx1742->flash_data  - (size_t)&vx1742->output_buffer);
    std::cout << "###Create new contiguous memory segment.. ";
    seg = new CMEMSegment("caenProducer", vmec::buffer_size);
    if((*seg)()){
    	std::cout << "ERROR generating contiguous memory segment..\n Check /proc/cmem_rcc for more information." << std::endl;
    	vme->ErrorPrint((*seg)());
    	std::exit(-1);
    }
    std::cout << "[OK]" << std::endl;

    //connection is initialized at this point, print some information:
    std::cout << std::endl << "***********************************************************************" << std::endl;
    std::cout << "Board serial number: " << this->getSerialNumber() << std::endl;
    std::cout << this->getFirmwareVersion() << std::endl;
    std::cout << this->getDRS4FirmwareVersion();
    std::cout << "***********************************************************************" << std::endl << std::endl;

    //initialize variables
    correctionDataInitialized[vmec::VX1742_GROUPS][vmec::VX1742_NFREQ] = {false};
    for(uint32_t grp=0; grp<vmec::VX1742_GROUPS; grp++){
      for(uint32_t freq=0; freq<vmec::VX1742_NFREQ; freq++){
        std::fill_n(corrTable[grp][freq].time, vmec::VX1742_MAX_SAMPLES, 0);
        for(uint32_t ch=0; ch<vmec::VX1742_MAX_CHANNEL_SIZE; ch++){
          std::fill_n(corrTable[grp][freq].cell[ch], vmec::VX1742_MAX_SAMPLES, 0);
          std::fill_n(corrTable[grp][freq].nsample[ch], vmec::VX1742_MAX_SAMPLES, 0);
        }
      }
    }

  }catch(...){
  	std::cout << "Problem occured in VX1742Interface::OpenVME()" << std::endl;
  }
}


void VX1742Interface::closeVME(){
    delete seg;
    vme->MasterUnmap(vmm);
    vme->Close();
}


std::string VX1742Interface::getSerialNumber(){
    return std::to_string(vx1742->configuration_rom.sernum0);	
}

std::string VX1742Interface::getFirmwareVersion(){
    std::stringstream firmware;
    uint32_t value, value1, value2;
    value = ((vx1742->mother_roc_firmware)&0xff);
    value1 = (((vx1742->mother_roc_firmware)>>8)&0xff);
    value2 = ((vx1742->mother_roc_firmware)>>16);
    uint32_t day = (value2&0xf) + 10*((value2>>4)&0xf);
    uint32_t month = ((value2>>8)&0xf);
    uint32_t year = ((value2>>12)&0xf);
    firmware << "VX1742 firmware version: " << value1 << "." << value << " from " << day << "." << month << "." << year;
    return firmware.str();
}


std::string VX1742Interface::getDRS4FirmwareVersion(){
	std::stringstream firmware;
	uint32_t tmp, value, value1, value2;
	for(uint32_t idx=0; idx<vmec::VX1742_GROUPS; idx++){
		tmp = vx1742->group_n_conf[idx].daugther_board_fw;
		value = (tmp&0xff);
  		value1 = ((tmp>>8)&0xff);
  		value2 = (tmp>>16);
  		uint32_t day = (value2&0xf) + 10*((value2>>4)&0xf);
  		uint32_t month = ((value2>>8)&0xf);
  		uint32_t year = ((value2>>12)&0xf);
  		firmware << "DRS4 firmware version group " << idx << ": " << value1 << "." << value << " from " << day << "." << month << "." << year << std::endl;
	}
	return firmware.str();
}

uint32_t VX1742Interface::isRunning(){
	return vx1742->acq_status.run; //returns 1 if running
}

void VX1742Interface::startAcquisition(){
	vx1742->acq_control.run = 1;
}

void VX1742Interface::stopAcquisition(){
	vx1742->acq_control.run = 0;
}

void VX1742Interface::printAcquisitionStatus(){
	printf("Board %s\n", vx1742->acq_status.run ? "running" : "not running");
	printf("Event %s\n", vx1742->acq_status.event_ready ? "ready" : "not ready");
	printf("Event %s\n", vx1742->acq_status.event_full ? "full" : "free");
	printf("Clock source %s\n", vx1742->acq_status.clock_source ? "external" : "internal");
	printf("PLL %s\n", vx1742->acq_status.PLL_bypass ? "bypassed" : "no bypass");
	printf("PLL %s\n", vx1742->acq_status.PLL_status ? "locked" : "loss of lock");
	printf("Board %s\n", vx1742->acq_status.ready ? "ready" : "not ready");
}

void VX1742Interface::printAcquisitionControl(){
	std::cout << "reserved 0: " << vx1742->acq_control.reserved_0 << std::endl;
	std::cout << "buffer mode: " << vx1742->acq_control.buffer_mode<< std::endl;
	std::cout << "reserved 1: " << vx1742->acq_control.reserved_1 << std::endl;
	std::cout << "trigger count: " << vx1742->acq_control.trigger_count << std::endl;
	std::cout << "run: " << vx1742->acq_control.run << std::endl;
	std::cout << "reserved" << vx1742->acq_control.reserved << std::endl;

}

void VX1742Interface::softwareReset(){
	vx1742->software_reset = 1;
}

void VX1742Interface::clearBuffers(){
	vx1742->software_clear = 1;
}

void VX1742Interface::setSamplingFrequency(uint32_t param){
	vx1742->sampling_frequency = param;
}

//0=5GS/s, 1=2.5GS/s, 2=1GS/s
uint32_t VX1742Interface::getSamplingFrequency(){
	return vx1742->sampling_frequency;

}

bool VX1742Interface::SPIBusBusy(uint32_t grp){
	return vx1742->group_n_conf[grp].group_status.spi_busy;
}

void VX1742Interface::enableTRn(uint32_t enable[2], uint32_t threshold[2], uint32_t offset[2], uint32_t polarity){
	if(enable[0] || enable[1]){
		vx1742->group_conf.trigger_trn_enable = 1;
		vx1742->group_conf.sig_trn_enable = 0; //disable readout of signal for now
		vx1742->group_conf.trigger_polarity = polarity;
	}
	for(int grp=0; grp<2; grp++){
		if(!SPIBusBusy(grp*2)){
			vx1742->group_n_conf[grp*2].trn_threshold = threshold[grp];
			vx1742->group_n_conf[grp*2].trn_dc_offset = offset[grp];
		}else{
			std::cout << std::endl << "Enabling TRn failed - SPI Bus busy!" << std::endl;
			exit(-1);
		}
	}
}

void VX1742Interface::setPostTriggerSamples(uint32_t param){
	vx1742->post_trigger = param;
}

uint32_t VX1742Interface::getPostTriggerSamples(){
	return vx1742->post_trigger;
}

void VX1742Interface::setTriggerSource(uint32_t param){
	if(param==0){
		vx1742->trigger_src.sw_trigger = 0;
		vx1742->trigger_src.ext_trigger = 1;
	}
	if(param==1){
		vx1742->trigger_src.ext_trigger = 0;
		vx1742->trigger_src.sw_trigger = 1;
	}
}

std::string VX1742Interface::getTriggerSource(){
	std::stringstream source;
	source << "Trigger source: ";
	uint32_t sw = vx1742->trigger_src.sw_trigger;
	uint32_t ext = vx1742->trigger_src.ext_trigger; 
	if(sw==0 and ext==1){source << "external";}
	if(sw==1 and ext==0){source << "software";
	}else{source << "not defined";}
	return source.str();
}


void VX1742Interface::toggleGroups(uint32_t param[]){
	param[0]==1 ? vx1742->group_en_mask.group0=1 : vx1742->group_en_mask.group0=0;
	param[1]==1 ? vx1742->group_en_mask.group1=1 : vx1742->group_en_mask.group1=0;
	param[2]==1 ? vx1742->group_en_mask.group2=1 : vx1742->group_en_mask.group2=0;
	param[3]==1 ? vx1742->group_en_mask.group3=1 : vx1742->group_en_mask.group3=0;
}

uint32_t VX1742Interface::getActiveChannels(){
	return ((vx1742->group_en_mask.group0 + vx1742->group_en_mask.group1 + vx1742->group_en_mask.group2 + vx1742->group_en_mask.group3)*8);
}



void VX1742Interface::sendBusyToTRGout(){
	vx1742->front_panel_io_control.force_trg_out_mode = 0; //force it or not, it does not matter
	vx1742->front_panel_io_control.trg_out_mode = 0; //trg_out is internal signal
	vx1742->front_panel_io_control.trg_out_mode_select = 1; //select trg_out
	vx1742->front_panel_io_control.motherboad_probe = 3; //allows roc to issue busy signal
	vx1742->front_panel_io_control.busy_unlock = 0; //propagates it to trg_out
}


void VX1742Interface::setCustomSize(uint32_t param){
	vx1742->custom_size = param;
}

uint32_t VX1742Interface::getCustomSize(){
	return vx1742->custom_size;
}

void VX1742Interface::setTriggerCount(){
	vx1742->acq_control.trigger_count = 0;
}

void VX1742Interface::setMaxBLTEvents(uint32_t param){
	vx1742->blt_event_number = param;
}

bool VX1742Interface::eventReady(){
	return vx1742->acq_status.event_ready;
}


uint32_t VX1742Interface::getEventsStored(){
	return vx1742->events_stored;
}

uint32_t VX1742Interface::getNextEventSize(){
	return vx1742->event_size;
}

void VX1742Interface::disableIndividualTriggers(){
	vx1742->group_conf.individual_trg = 1;
}




uint32_t VX1742Interface::readFlashPage(uint32_t group, int8_t* page, uint32_t pagenumber){
	uint32_t flash_addr;
	uint8_t addr0, addr1, addr2;
	uint16_t dd = 0xffff;
	uint32_t ret = 0;
	uint32_t fl_size = 528;
	uint32_t fl_a[fl_size];
	uint32_t tmp[fl_size];

	flash_addr = pagenumber<<9;
	addr0 = uint8_t(flash_addr);
	addr1 = uint8_t(flash_addr>>8);
	addr2 = uint8_t(flash_addr>>16);

	while((dd>>2) & 0x1)
		if((ret = vmm->ReadSafe(STATUS(group), &dd)) != 0){std::cout << "Digitizer memory corrupted (0)!" << std::endl; return -1;}
		
	if((ret = vmm->WriteSafe(SEL_FLASH(group), (uint16_t)1)) != 0){std::cout << "Digitizer memory corrupted (1)!" << std::endl; return -1;}
	if((ret = vmm->WriteSafe(FLASH(group), vmec::VX1742_MAIN_MEM_PAGE_READ)) != 0){std::cout << "Digitizer memory corrupted (2)!" << std::endl; return -1;}

	if((ret = vmm->WriteSafe(FLASH(group), (uint16_t)addr2)) != 0){std::cout << "Digitizer memory corrupted (3)!" << std::endl; return -1;}
	if((ret = vmm->WriteSafe(FLASH(group), (uint16_t)addr1)) != 0){std::cout << "Digitizer memory corrupted (4)!" << std::endl; return -1;}
	if((ret = vmm->WriteSafe(FLASH(group), (uint16_t)addr0)) != 0){std::cout << "Digitizer memory corrupted (5)!" << std::endl; return -1;}

	for(uint32_t idx=0; idx<4; idx++){
		if((ret = vmm->WriteSafe(FLASH(group), (uint16_t)0)) != 0){std::cout << "Digitizer memory corrupted (6)!" << std::endl; return -1;}
	}

	for(uint32_t idx=0; idx<fl_size; idx+=2){
		fl_a[idx] = FLASH(group);
		fl_a[idx+1] = STATUS(group);
	}

	for(uint32_t rd=0; rd<fl_size; rd++){
		if((ret = vmm->ReadSafe(fl_a[rd], &tmp[rd])) != 0){std::cout << "Digitizer memory corrupted (7)!" << std::endl; return -1;}
	}

	for(uint32_t idx=0; idx<fl_size; idx+=2){
		page[(int)(idx/2)] = (int8_t) tmp[idx];
	}

	if((ret = vmm->WriteSafe(SEL_FLASH(group), (uint16_t)0)) != 0){std::cout << "Digitizer memory corrupted (8)!" << std::endl; return -1;}
	return 0;
}





uint32_t VX1742Interface::loadDRS4CorrectionTables(uint32_t group, uint32_t frequency){
    uint32_t pagenum = 0;
    uint32_t start;
	  uint32_t ret = 0;
    int8_t tempCell[264];
    int8_t *p;
    int8_t tmp[0x1000]; // 256byte * 16 pages


    for (int ch=0; ch<vmec::VX1742_MAX_CHANNEL_SIZE; ch++){
        pagenum = 0;
        pagenum = (group %2) ? 0xC00: 0x800;
        pagenum |= frequency << 8;
        pagenum |= ch << 2;

        //load offset cell correction:
        p = tempCell;
        uint32_t start = 0;
        for(uint32_t idx=0; idx<4; idx++){
        	uint32_t endidx = 256;
        	if((ret = this->readFlashPage(group, p, pagenum)) != 0)
        		return ret;

       		//peak correction
       		for(uint32_t jdx=start; jdx<(start+256); jdx++){
       			if(p[jdx-start] != 0x7f){ //case not to be corrected
       				corrTable[group][frequency].cell[ch][jdx] = p[jdx-start]; 
       			}else{ //case to be corrected
       				uint16_t cel = (uint16_t)((p[endidx+1] << 0x08) | ((uint8_t)p[endidx]));
       				if (cel == 0) corrTable[group][frequency].cell[ch][jdx] = p[jdx-start]; else corrTable[group][frequency].cell[ch][jdx] = cel;
       				endidx += 2;
       				if(endidx > 263){
       					std::cout << "Something went wrong when reading the VX1742 calibration data from the board." << std::endl;
       					return -1;
       				}
       			}
       		}
       		start += 256;
       		pagenum++;	
        }


        //load the offset number of samples correction
        start = 0;
        p = tempCell;
		    pagenum &= 0xF00;
        pagenum |= 0x40;
        pagenum |= ch << 2;    

        for(uint32_t idx=0; idx<4; idx++){
        	if((ret = readFlashPage(group, p, pagenum)) != 0)
        		return ret;
        	
        	for(uint32_t jdx=start; jdx<(start+256); jdx++){
        		corrTable[group][frequency].nsample[ch][jdx] = 0;
        		if(isdigit(p[jdx-start]))
        			corrTable[group][frequency].nsample[ch][jdx] = p[jdx-start];
        	}
        	start += 256;
        	pagenum++;
        }


        //load time correction
        if(ch == (vmec::VX1742_CHANNELS_PER_GROUP)){
   		    p = tempCell;
            pagenum &= 0xF00;
            pagenum |= 0xA0;
            start = 0;     	

            for (uint32_t idx=0; idx<16; idx++){
                if ((ret = readFlashPage(group,p,pagenum)) != 0) 
                    return ret;
                for (uint32_t jdx=start; jdx<start+256; jdx++){
                	tmp[jdx] = p[jdx-start];
                }
                start +=256;
                pagenum++;
            }


            for (uint32_t idx=0; idx<1024; idx++){
                p = (int8_t *) &(corrTable[group][frequency].time[idx]);
                p[0] = tmp[idx*4];
                p[1] = tmp[(idx*4)+1];
                p[2] = tmp[(idx*4)+2];
                p[3] = tmp[(idx*4)+3];
            }
        }

    }//end channel for loop
 
    return 0;
}	



uint32_t VX1742Interface::initializeDRS4CorrectionTables(uint32_t frequency){
	uint ret = 0;

	switch(frequency){
		case 0:
			std::cout << "Initializing correction tables for 5 GS/s sampling speed:" << std::endl;
			break;
		case 1:
			std::cout << "Initializing correction tables for 2.5 GS/s sampling speed:" << std::endl;
			break;
		case 2:
			std::cout << "Initializing correction tables for 1 GS/s sampling speed:" << std::endl;
			break;
		case 3:
			std::cout << "Initializing correction tables for 750 MS/s sampling speed:" << std::endl;
			break;
		default: return -1;
	}

	for(uint32_t grp=0; grp<vmec::VX1742_GROUPS; grp++){
		if(correctionDataInitialized[grp][frequency] != 1){ //if table has not been initialized, initialize it
			if((ret = this->loadDRS4CorrectionTables(grp, frequency)) == 0){
        std::cout << "Initializing group " << grp << " [OK]." << std::endl;
				correctionDataInitialized[grp][frequency] = 1; //fixme: CAEN does not give read access to groups that are not activated, this might be initialized but there's no 'real' data
			}else{std::cout << "ERROR in loading calibration tables!" << std::endl; return -1;}
		}
	}
	return 0;

}


void VX1742Interface::printDRS4CorrectionTables(){
	for(int grp=0;grp<vmec::VX1742_GROUPS;grp++){
    	for(int freq=0;freq<vmec::VX1742_NFREQ;freq++){
    		if(correctionDataInitialized[grp][freq] == 1){
    			std::cout << "Cell data:" << std::endl;
    			for(int ch=0; ch<vmec::VX1742_MAX_CHANNEL_SIZE; ch++){
    				//cell data
    				std::cout << "Group" << grp << ", channel " << ch << " cell data:";
    				for(int sample=0; sample<vmec::VX1742_MAX_SAMPLES; sample++)
    					std::cout << "(" << sample << "," << corrTable[grp][freq].cell[ch][sample] << "), ";
    				std::cout << "\n\n\n";

    				//nsamples
    				std::cout << "Group" << grp << ", channel " << ch << " nsamples:";
    				for(int sample=0; sample<vmec::VX1742_MAX_SAMPLES; sample++)
    					std::cout << "(" << sample << "," << std::to_string(corrTable[grp][freq].nsample[ch][sample]) << "), ";
    				std::cout << "\n\n\n";			
    			}

    			//timing data
    			std::cout << "Group " << grp << " timing:";
    			for(int sample=0; sample<vmec::VX1742_MAX_SAMPLES; sample++)
    			std::cout << "(" << sample << "," << corrTable[grp][freq].time[sample] << "), ";
    			std::cout << "\n\n\n";	
    		}else{
    			std::cout << "No data for group " << grp << " and sampling speed (0=5GS/s; 1=2.5GS/s; 2=1GS/s; 3=0.75GS/s) " << freq << " initialized." << std::endl;	
    		}
    	}
    }
}


int16_t VX1742Interface::getCellCorrectionValues(uint32_t group, uint32_t freq, uint32_t channel, uint32_t idx){
		//std::cout << "Group: " << group << " Frequency: " << freq << " Channel: " << channel << " idx: " << idx << " Value: " << corrTable[group][freq].cell[channel][idx] <<  std::endl;
    //std::cout << corrTable[group][freq].cell[channel][idx] << std::endl;
    return (uint16_t)corrTable[group][freq].cell[channel][idx];
}


int8_t VX1742Interface::getNSamplesCorrectionValues(uint32_t group, uint32_t freq, uint32_t channel, uint32_t idx){
    	return (uint8_t)corrTable[group][freq].nsample[channel][idx];
}


float VX1742Interface::getTimingCorrectionValues(uint32_t group, uint32_t freq, uint32_t idx){
    	return (float)corrTable[group][freq].time[idx];
}


//nEvents needs to be smaller than 255
uint32_t VX1742Interface::BlockTransferEventD64(VX1742Event *vxEvent){
	uint32_t eventsize = this->getNextEventSize();
	uint32_t eventsize_history = eventsize;
	
	//this->setMaxBLTEvents(1); //raise BE if more than 1 event is read out

	if(eventsize > 0){
		uint32_t ret;
		uint32_t increment = 0;
		bool remaining = false;
		while(eventsize > 1024){
			ret = vme->RunBlockTransfer(*seg, increment, vmec::vmebus_address, 0x400, VME_DMA_D64R, vmec::buffer_size, true);
			if(ret != 0){std::printf("Error in BlockTransfer!"); return -1;}
			increment += 0x400;
			eventsize -= 0x400;
			remaining = true;
		}
		if(remaining){
			ret = vme->RunBlockTransfer(*seg, increment, vmec::vmebus_address, eventsize, VME_DMA_D64R, vmec::buffer_size, true); //get the rest
			if(ret != 0){std::printf("Error in BlockTransfer!"); return -1;}
		}else{
			ret = vme->RunBlockTransfer(*seg, 0x0, vmec::vmebus_address, eventsize, VME_DMA_D64R, vmec::buffer_size, true); //get small event
			if(ret != 0){std::printf("Error in BlockTransfer!"); return -1;}
		}

		std::cout << "size of u_long" << sizeof(u_long) << " ,size of uint32_t: " << sizeof(uint32_t) << std::endl;
		//uint32_t* data = (uint32_t*)seg->VirtualAddress();
		uint32_t* data = (uint32_t*)(seg->VirtualAddress());

    	
    	#ifdef DEBUG
    		std::printf("CMEM segment, virt = %p, phys = 0x%016lx, size = %u\n", (void*)data, seg->PhysicalAddress(), eventsize);
    		std::cout <<"Dump data:" << std::endl;
    		for(uint32_t i=0; i<eventsize; i++){
        		std::printf("%6d: 0x%08x\n",i , data[i]);}
        #endif
        
		//write data to event class
		VX1742Event::header head;
		
		uint32_t offset = 0;
		head.size.raw = data[offset];
		//std::cout << "Header size raw: " << data[offset] << std::endl;
		
		//some checks on the data
		if(head.size.raw == 0)
			std::printf("First word of VX1742 event not found!\n");
		if(head.size.eventSize != eventsize_history)
			std::printf("Read event contains %u words instead of the expected %u!\n", head.size.eventSize, eventsize_history);
		if(head.size.A != 0xA)
			std::printf("Event does not start with 0xA!");

		head.pattern.raw= data[(++offset)];
		head.evnt_cnt.raw= data[(++offset)];
		head.trigger_time= data[(++offset)];


		//#ifdef DEBUG
			printf("******************************************************\n");
			printf("RAW header:         0x%08X\n", head.size.raw);
			printf("0xA:                0x%01X\n", head.size.A);
			printf("Event size:         %u\n", head.size.eventSize);
			printf("------------------------------------------------------\n");
			printf("RAW header_pattern: 0x%08X\n", head.pattern.raw);
			printf("Group mask:         %u\n", head.pattern.group_mask);
			printf("Pattern:            %u\n", head.pattern.pattern);
			printf("Board ID:           %u\n", head.pattern.board_id);
			printf("------------------------------------------------------\n");
			printf("RAW event_counter:  0x%08X\n", head.evnt_cnt.raw);
			printf("Event count:        %u\n", head.evnt_cnt.event_counter);
			printf("------------------------------------------------------\n");
			printf("Trigger time:       %u\n", head.trigger_time);
			printf("******************************************************\n\n");
		//#endif

		vxEvent->setData(&head, data+4, (head.size.eventSize-4));
	}

	return 0;
}