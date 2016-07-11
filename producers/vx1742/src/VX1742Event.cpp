/* ---------------------------------------------------------------------------------
** CAEN VX1742 implementation into the EUDAQ framework - Event Structure
** 
** <VX1742Event>.cpp
** 
** Date: April 2016
** Author: Christian Dorfer (dorfer@phys.ethz.ch)
** ---------------------------------------------------------------------------------*/

#include "VX1742Event.hh"
#include "VX1742DEFS.hh"

#include <string.h>
#include <iostream>


VX1742Event::VX1742Event(){
	buffer = NULL;
	bufsize = 0; 
	//bzero(head.raw,16);
	return;	
}

VX1742Event::~VX1742Event(){
	if (buffer != NULL){
		free(buffer); 
		buffer=NULL;}
}

int VX1742Event::setData(header* newhead, const uint32_t* newbuffer, int newbuflen) {
	newbuflen *= sizeof(uint32_t);
	resize_rawbuffer(newbuflen);
	if (buffer == NULL) {
		printf("VX1742Event: could not (re)allocate event data buffer of size %d\n!", newbuflen);
		bufsize=0;
		return -1;
	}
	// copy given data
	memcpy(buffer, newbuffer, bufsize); //FIXME: check alignment
	for(int i=0; i<4; i++){
		head.raw[i] = newhead->raw[i];
	}

	//for(int idx=0; idx<(bufsize/4-1); idx=idx+2){
	//	printf("%d: %08x %08x\n", idx, buffer[idx], buffer[idx+1]);
	//}
	
	uint32_t group_size = this->getGroupSizeInBuffer();
	for(uint32_t grp = 0; grp<vmec::VX1742_GROUPS; grp++){
		int grppos = getGroupIndexInBuffer(grp);
		if(grppos>=0){			
			group_heads.grh[grp].raw = buffer[grppos];
			group_trails.grt[grp].raw = buffer[grppos+group_size-1];
			/*
			std::printf("###############################################\n");
        	std::printf("This is group: %d\n", grp);
        	std::printf("Group size in buffer: %d\n", group_size);
        	std::printf("Group index in buffer: %d\n", grppos);
        	std::printf("Size of group: %d\n", group_heads.grh[grp].groupSize);
        	std::printf("TR?: %d\n", group_heads.grh[grp].tr);
        	std::printf("Frequency: %d\n", group_heads.grh[grp].frequency);
        	std::printf("Start Index cell: %d\n",  group_heads.grh[grp].index_cell);
        	std::printf("Trigger Time Tag: %d\n",  group_trails.grt[grp].trigger_time);
        	std::printf("###############################################\n");
        	*/
		}
	}
	return 0;
}


int VX1742Event::resize_rawbuffer(int to_bytesize){
	if (buffer == NULL) {
		buffer = (uint32_t*)malloc(to_bytesize);
		bufsize=to_bytesize;
	}else{
		buffer = (uint32_t*)realloc(buffer, to_bytesize);
		bufsize=to_bytesize;
		if ( bufsize == 0 ) buffer=NULL;
	}
	return bufsize;
}

int VX1742Event::rawdata_size() const{
	return bufsize;
}

void VX1742Event::invalidate(){
	for(int i=0;i<4;i++){
		head.raw[i]=0;
	}
}

bool VX1742Event::isValid() const {
	if (buffer == NULL) return false;
	if (head.size.A != 0xA) return false;
	if (head.size.eventSize == 0 ) return false;
	return true;
}
uint32_t VX1742Event::EventSize() const {
	if (!isValid()) std::printf("Event not valid!");
	return head.size.eventSize;
}
uint32_t VX1742Event::BoardID() const {
	if (!isValid()) std::printf("Event not valid!");
	return head.pattern.board_id;
}
uint32_t VX1742Event::Pattern() const {
	if (!isValid()) std::printf("Event not valid!");
	return head.pattern.pattern;
}
uint32_t VX1742Event::GroupMask() const {
	if (!isValid()) std::printf("Event not valid!");
	return head.pattern.group_mask; //channel_mask;
}
uint32_t VX1742Event::EventCounter() const {
	if (!isValid()) std::printf("Event not valid!");
	return head.evnt_cnt.event_counter;
}
uint32_t VX1742Event::TriggerTimeTag() const {
	if (!isValid()) std::printf("Event not valid!");
	return head.trigger_time; 
}

uint32_t VX1742Event::Groups() const {
	if (!isValid()) std::printf("Event not valid!");
	uint32_t groups = this->GroupMask();
	uint32_t temp=0;
	do {temp += groups&1;} while (groups>>=1);
	return temp;
}


const VX1742Event::header* VX1742Event::gethead() const{
	return &head;
}


uint32_t VX1742Event::getGroupSizeInBuffer() const{
	if(this->Groups() > 0)
		return (bufsize/4)/(this->Groups()); //number of 32bit integers
	return 0;
}


int VX1742Event::getGroupIndexInBuffer(uint32_t grp) const{
	int before = 0;
	for(int i=0; i<grp; i++){
		if((1<<i) & this->GroupMask()){
			before++;
		}
	}
	if((1<<grp) & this->GroupMask()){
		return (this->getGroupSizeInBuffer()*before);
	}else{
		return -1;
	}
}

//until here event independent regarding TRn enabled/disabled, now the mess starts:
int VX1742Event::SamplesPerChannel(uint32_t grp) const{
	if (grp > vmec::VX1742_GROUPS) {std::printf("There are only %d groups!\n", vmec::VX1742_GROUPS); return -1;}
	int grppos = getGroupIndexInBuffer(grp);
	if (grppos<0) {std::printf("Group %d NOT FOUND in data!\n", grp); return -1;}
	
	bool TRn_enabled = group_heads.grh[grp].tr;
	uint32_t gr_size = this->getGroupSizeInBuffer()-2; //-2 for group header and trailer
	if(TRn_enabled){
		return (gr_size*32)/((vmec::VX1742_CHANNELS_PER_GROUP+1)*vmec::VX1742_RESOLUTION);
	}
	return (gr_size*32)/((vmec::VX1742_CHANNELS_PER_GROUP)*vmec::VX1742_RESOLUTION);
}


//return 9 per group if TRn is enabled
uint32_t VX1742Event::Channels(uint32_t grp) const{
	uint32_t groups = this->Groups();
	if (groups == 0) return 0;
	bool TRn_enabled = group_heads.grh[grp].tr;
	if(TRn_enabled){
		return 9;
	}
	return 8;
}


uint32_t VX1742Event::GetStartIndexCell(uint32_t grp) const{
	int grppos = getGroupIndexInBuffer(grp);
	if (grppos<0) {std::printf("Group %d NOT FOUND in data!\n", grp); return -1;}
	return group_heads.grh[grp].index_cell;
}

uint32_t VX1742Event::GetEventTimeStamp(uint32_t grp) const{
	int grppos = getGroupIndexInBuffer(grp);
	if (grppos<0) {std::printf("Group %d NOT FOUND in data!\n", grp); return -1;}
	return group_trails.grt[grp].trigger_time;
}


int VX1742Event::getChannelData(unsigned int grp, unsigned int ch, uint16_t* array, unsigned int arraylen) const{
	if (ch >= this->Channels(grp)){std::printf("There are only %d channels!\n", vmec::VX1742_MAX_CHANNEL_SIZE); return -1;}
	if (grp > vmec::VX1742_GROUPS){std::printf("There are only %d groups!\n", vmec::VX1742_GROUPS); return -1;}
	
	int grppos = getGroupIndexInBuffer(grp) + 1;
	if (grppos<0){std::printf("Group %d NOT FOUND in data!\n", grp); return -1;}
	int samples = this->SamplesPerChannel(grp);
	if(samples == -1){return -1;}
	bool TRn_enabled = group_heads.grh[grp].tr;


	//std::cout << "Group position in buffer: " << grppos << std::endl;
	//std::cout << "Group size in buffer: " << this->getGroupSizeInBuffer() << std::endl;
	//std::cout << "Samples per channel: " << samples << std::endl;

	//trn signal
	if(ch==8){
	  grppos = grppos+samples*3;
	  std::cout << "Calculated TRn position: " << grppos << std::endl;
	  uint32_t index = 0;
	  uint32_t start_bit = 0;
	  uint32_t line = 0;
	  uint32_t it = samples;
	  
	  for(uint32_t idx=0; idx < it; idx++){
	  	array[idx] = (buffer[grppos+line]>>start_bit)&0xFFF;

	  	index += 12;
	  	start_bit = index%32;
	  	line = (uint32_t) index/32;
	  	std::cout << "Index: " << index << " start bit: " << start_bit << " line: " << line << std::endl;


	  	if(start_bit > 20){
	  		uint32_t low = (buffer[grppos+line]>>start_bit);
	  		uint32_t high = buffer[grppos+line+1]&((1<<(start_bit-20))-1);
	  		array[idx+1] = high+low;
	  		it--;
	  		index += 12;
	  		start_bit = index%32;
	  		line = (uint32_t) index/32;
	  	    std::cout << "Index (if): " << index << " start bit: " << start_bit << " line: " << line << std::endl;
	  	}
	  }

	}else{ //all other channels
	  uint32_t start_bit = ch*12%32;
	  uint32_t line = (uint32_t) (12*ch)/32;
	  uint32_t temp = 32-start_bit;
	  if(temp<12){
		for(uint32_t idx=0; idx < samples; idx++){
		  uint32_t low = buffer[grppos+idx*3+line]>>start_bit;
		  uint32_t high = (buffer[grppos+idx*3+line+1]&((1<<(12-temp))-1))<<temp;
		  array[idx] = high + low;
		}
	  }
	  if(temp>=12){
		for(uint32_t idx=0; idx < samples; idx++){
		  array[idx] = (buffer[grppos+idx*3+line]>>start_bit)&0xFFF;
		}
	  }
	}

}//method end

