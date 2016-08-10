/* ---------------------------------------------------------------------------------
** Interface to Artix-7 (AC701 Development Board) via HTTP server running on board.
** Based on work done at OSU
**
**
** <TUDEFS>.h
** 
** Date: August 2016
** Author: Christian Dorfer (dorfer@phys.ethz.ch)
** ------------------------------------------------------------------------------------*/

#ifndef _TUDEFS_H
#define _TUDEFS_H

namespace tuc{
  const unsigned int HOST_PORT = 80;
  const unsigned int STREAM_HOST_PORT = 8080;

  //readout addresses
  const unsigned int TRIGGER_LOGIC_HEADER = 0;
  const unsigned int TRIGGER_LOGIC_READBACK_ID = 4;
  const unsigned int TRIGGER_COUNT_0 = 8;
  const unsigned int TRIGGER_COUNT_1 = 12;
  const unsigned int TRIGGER_COUNT_2 = 16;
  const unsigned int TRIGGER_COUNT_3 = 20;
  const unsigned int TRIGGER_COUNT_4 = 24;
  const unsigned int TRIGGER_COUNT_5 = 28;
  const unsigned int TRIGGER_COUNT_6 = 32;
  const unsigned int TRIGGER_COUNT_7 = 36;
  const unsigned int TRIGGER_COUNT_8 = 40;
  const unsigned int TRIGGER_COUNT_9 = 44;
  const unsigned int TRIGGER_LOGIC_COINCIDENCE_CNT = 48;
  const unsigned int TRIGGER_LOGIC_BEAM_CURRENT = 52;
  const unsigned int TRIGGER_LOGIC_PRESCALER_CNT = 56;
  const unsigned int TRIGGER_LOGIC_PRESCALER_XOR_PULSER_CNT = 60;
  const unsigned int TRIGGER_LOGIC_PRESCALER_XOR_PULSER_AND_PRESCALER_DELAYED_CNT = 64;
  const unsigned int TRIGGER_LOGIC_PULSER_DELAY_AND_XOR_PULSER_CNT = 68;
  const unsigned int TRIGGER_LOGIC_HANDSHAKE_CNT = 72;
  const unsigned int TRIGGER_LOGIC_COINCIDENCE_CNT_NO_SIN = 76;
  const unsigned int TRIGGER_LOGIC__S = 80;  
  const unsigned int TRIGGER_LOGIC_TIME_STAMP_HIGH = 84;
  const unsigned int TRIGGER_LOGIC_TIME_STAMP_LOW = 88;
  const unsigned int TRIGGER_LOGIC_CHECK_SUM = 96;
  const unsigned int SLAVE_REG_24 = 92;
  const unsigned int TRIGGER_LOGIC_END_FAG = 92;
  const unsigned int TRIGGER_LOGIC_READBACK_FILE_SIZE = 104;


  struct RET_DATA{
    unsigned int id; //readout count
    unsigned int trigger_counts[10];
    unsigned int coincidence_count_no_sin;
    unsigned int coincidence_count;
    unsigned int beam_curent;
    unsigned int prescaler_count; 
    unsigned int prescaler_count_xor_pulser_count;
    unsigned int prescaler_xor_pulser_and_prescaler_delayed_count;
    unsigned int pulser_delay_and_xor_pulser_count;
    unsigned int handshake_count;
    unsigned int coincidence_rate;
    unsigned int prescaler_rate;
    unsigned long time_stamp; 
    unsigned int check_sum;
    unsigned int end_flag; //should be 'E" = 69 desimal
  }typedef Readout_Data;
  
}
#endif