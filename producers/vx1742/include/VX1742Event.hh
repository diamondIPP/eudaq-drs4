/* ---------------------------------------------------------------------------------
** CAEN VX1742 implementation into the EUDAQ framework - Event Structure
** 
** <VX1742Event>.hh
** 
** Date: April 2016
** Author: Christian Dorfer (dorfer@phys.ethz.ch)
** ---------------------------------------------------------------------------------*/


#ifndef VX1742EVENT_HH
#define VX1742EVENT_HH

//#include <sys/types.h>
//#include <stdint.h>

#include <cstdint>

class VX1742Event{
  
  public:
	union header_size {
	    struct {
	      uint32_t eventSize		:28;
		  uint32_t A				:4;
	  };
		uint32_t raw;
	};


	union header_pattern {
	    struct{
		  uint32_t group_mask		:4;
		  uint32_t reserved_1		:4;
		  uint32_t pattern			:14;
		  uint32_t reserved_0		:5;
		  uint32_t board_id		:5;
		};
		uint32_t raw;
	};

	union header_event_counter {
		struct {
		  uint32_t event_counter		:22;
		  uint32_t reserved			:10;
		};
		uint32_t raw;
	};


	union header {
	  struct {
		union header_size size;
		union header_pattern pattern;
		union header_event_counter evnt_cnt;
		uint32_t trigger_time;
	  };
	  uint32_t raw[4];
	};



	//specific event based group stuff:
	union group_header{
	  struct{
	  	uint32_t groupSize 	   :12;
	  	uint32_t tr 				:1;
	  	uint32_t reserved_3		:3;
	  	uint32_t frequency			:2;
	  	uint32_t reserved_2		:2;
	  	uint32_t index_cell	   :10;
	  	uint32_t reserved_1		:2;
	  };
	  uint32_t raw;
	};


	union gr_headers{
		struct{
			union group_header grh[4];
		};
		uint32_t raw[4];
	};


	union group_trailer{
		struct{
			uint32_t trigger_time		:30;
			uint32_t reserved 			 :2;
		};
		uint32_t raw;
	};


	union gr_trailers{
		struct{
			union group_trailer grt[4];
		};
		uint32_t raw[4];
	};


	VX1742Event();
	virtual ~VX1742Event();
	int setData(header* newhead, const uint32_t* newbuffer, int newbuflen);
	void invalidate();
	bool isValid() const;
	uint32_t EventSize() const;	
	uint32_t BoardID() const;
	uint32_t Pattern() const;
	uint32_t GroupMask() const;
	uint32_t EventCounter() const;	
	uint32_t TriggerTimeTag() const;
	uint32_t Groups() const;
	uint32_t Channels(uint32_t grp) const;	
	uint32_t GetEventTimeStamp(uint32_t grp) const;
	uint32_t GetStartIndexCell(uint32_t grp) const;
	
	const header* gethead() const;
	uint32_t getGroupSizeInBuffer() const;
	int getGroupIndexInBuffer(uint32_t grp) const;
	int SamplesPerChannel(uint32_t grp) const;
	int getChannelData(unsigned int grp, unsigned int chan, uint16_t* array, unsigned int arraylen) const;

	int rawdata_size() const;
	int resize_rawbuffer(int to_bytesize);
	
	
  private:	
	uint32_t	*buffer;	
	int		    bufsize; //bytes
	union 		header head;
	union		gr_headers group_heads;
	union		gr_trailers group_trails;


};


#endif //VX1742EVENT_HH
