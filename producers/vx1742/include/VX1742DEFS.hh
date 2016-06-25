/* ------------------------------------------------------------------------------------
** CAEN VX1742B constants and memory layout
** 
**
** <VX1742DEFS>.hh
** 
** Date: April 2016
** Author: Christian Dorfer (dorfer@phys.ethz.ch)
** ------------------------------------------------------------------------------------*/

#ifndef _VX1742DEFS_H
#define _VX1742DEFS_H

#include <cstdint>

namespace vmec{

    // Constants
    const uint32_t VX1742_CHANNELS = 32;
    const uint32_t VX1742_GROUPS = 4;
    const uint32_t VX1742_DRS4_COUNT = 4;
    const uint32_t VX1742_CHANNELS_PER_GROUP = 8;
    const uint32_t VX1742_MAX_CHANNEL_SIZE = 9; //8+TRn
    const uint32_t VX1742_RESOLUTION = 12;
    const uint32_t VX1742_MAX_SAMPLES = 1024;    

    // Constants for VME master map and BlockTransfer
    const uint32_t vmebus_address = 0x32100000; 
    const uint32_t window_size = 0x10000;
    const uint32_t address_modifier = 0x2; //VME_A32
    const uint32_t options = 0;
    const uint32_t buffer_size = 0x10000;

    const uint16_t VX1742_MAIN_MEM_PAGE_READ = 0x00D2;
    const uint16_t VX1742_MAIN_MEM_PAGE_PROG_TH_BUF1 = 0x0082;

    typedef struct{
      int16_t     cell[VX1742_MAX_CHANNEL_SIZE][VX1742_MAX_SAMPLES];
      int8_t      nsample[VX1742_MAX_CHANNEL_SIZE][VX1742_MAX_SAMPLES];
      float       time[VX1742_MAX_SAMPLES];
    }drs4_correction;


    typedef struct{
      uint32_t memory_full          :1; 
      uint32_t memory_empty         :1;
      uint32_t spi_busy             :1;
      uint32_t group_even_enable    :1; 
      uint32_t group_odd_enable     :1; 
      uint32_t reserved             :1;
      uint32_t group_even_pll_lock  :1; 
      uint32_t group_odd_pll_lock   :1; 
      uint32_t drs4_busy            :1;
      uint32_t mezzanine_rev        :1;
      uint32_t reserved_1           :22;
    }st_group_status;   


    typedef struct{
      uint32_t ch_thres;                                       //0x1080
      uint32_t reserved1;                                      //0x1084 - 0x1088
      st_group_status group_status;                            //0x1088
      uint32_t daugther_board_fw;                              //0x108C
      uint32_t reserved2;                                      //0x1090 - 0x1094
      uint32_t buffer_occupation;                              //0x1094
      uint32_t channel_dc_offset;                              //0x1098
      uint32_t reserved3;                                      //0x109C - 0x10A0
      uint32_t drs4_temperature;                               //0x10A0
      uint32_t dac_sel;                                        //0x10A4
      uint32_t ch_trg_enable_mask;                             //0x10A8
      uint32_t reserved4[8];                                   //0x10AC - 0x10CC
      uint32_t mem_calib_tables_enable;                        //0x10CC
      uint32_t mem_calib_tables_data;                          //0x10D0
      uint32_t trn_threshold;                                  //0x10D4
      uint32_t reserved5;                                      //0x10D8 - 0x10DC
      uint32_t trn_dc_offset;                                  //0x10DC
      uint32_t reserved6[40];                                  //0x10E0 - 0x1180
    }st_group_n_conf; //used for group 0-3 (for <group n variable address> add n*0x100 to the addresses listed)    
    

    typedef struct{
      uint32_t reserved_0          :3; 
      uint32_t test_mode           :1;
      uint32_t reserved_1          :1;
      uint32_t reserved_2          :1; 
      uint32_t trigger_polarity    :1; 
      uint32_t reserved_3          :1;
      uint32_t individual_trg      :1; 
      uint32_t reserved_4          :2; 
      uint32_t sig_trn_enable      :1;
      uint32_t trigger_trn_enable  :1;
      uint32_t reserved_5         :15;
      uint32_t monitor_signal      :4;
    }st_group_conf;    
    

    typedef struct{
      uint32_t reserved_0      :2;
      uint32_t run             :1;
      uint32_t trigger_count   :1;
      uint32_t reserved_1      :1;
      uint32_t buffer_mode     :1;
      uint32_t reserved       :26;
    }st_acq_control;    

    typedef struct{
      uint32_t reserved2       :2;
      uint32_t run             :1;
      uint32_t event_ready     :1;
      uint32_t event_full      :1;
      uint32_t clock_source    :1; 
      uint32_t PLL_bypass      :1; 
      uint32_t PLL_status      :1;
      uint32_t ready           :1; 
      uint32_t reserved1      :23;
    }st_acq_status;    
    

    typedef struct{
      uint32_t reserved       :30;
      uint32_t ext_trigger     :1;
      uint32_t sw_trigger      :1;
    }st_trigger_source;    
    

    typedef struct{
      uint32_t grp0_trg        :1;
      uint32_t grp1_trg        :1;
      uint32_t grp2_trg        :1;
      uint32_t grp3_trg        :1;
      uint32_t reserved       :26;
      uint32_t ext_trigger     :1;
      uint32_t sw_trigger      :1;
    }st_trigger_out_mask;    
    

    typedef struct{
      uint32_t trg_clk_level          :1;
      uint32_t panel_output_state     :1;
      uint32_t lvds_direction_0_3     :1;
      uint32_t lvds_direction_4_7     :1;
      uint32_t lvds_direction_8_11    :1;
      uint32_t lvds_direction_12_15   :1;
      uint32_t io_mode                :2;
      uint32_t reserved2              :6;
      uint32_t force_trg_out_mode     :1; 
      uint32_t trg_out_mode           :1;
      uint32_t trg_out_mode_select    :2;
      uint32_t motherboad_probe       :2;
      uint32_t busy_unlock            :1;
      uint32_t reserved1             :11;
    }st_front_panel_io_control;    
    

    typedef struct{
      uint32_t group0    :1;
      uint32_t group1    :1;
      uint32_t group2    :1;
      uint32_t group3    :1;
      uint32_t reserved  :24;
    }st_group_en_mask;    
    

    typedef struct{
      uint32_t board_type  :8;
      uint32_t memory      :8;
      uint32_t reserved    :16;
    }st_board_info;    
    

    typedef struct{
      uint32_t interrupt_level :3;
      uint32_t optical_link    :1;
      uint32_t berr            :1; 
      uint32_t align64         :1;
      uint32_t reloc           :1;
      uint32_t release         :1;
      uint32_t reserved       :24;
    }st_vme_control;    
    

    typedef struct{
      uint32_t event_ready   :1;
      uint32_t reserved2     :1;
      uint32_t bus_error     :1;
      uint32_t fifo_empty    :1;
      uint32_t reserved1    :28;
    }st_vme_status;    
    

    typedef struct{
      uint32_t checksum;                                            //0xF000                       
      uint32_t checksum_length2;                                    //0xF004
      uint32_t checksum_length1;                                    //0xF008
      uint32_t checksum_length0;                                    //0xF00C
      uint32_t constant2;                                           //0xF010
      uint32_t constant1;                                           //0xF014
      uint32_t constant0;                                           //0xF018
      uint32_t c_code;                                              //0xF01C
      uint32_t r_code;                                              //0xF020
      uint32_t oui2;                                                //0xF024
      uint32_t oui1;                                                //0xF028
      uint32_t oui0;                                                //0xF02C
      uint32_t vers;                                                //0xF030
      uint32_t board2;                                              //0xF034
      uint32_t board1;                                              //0xF038
      uint32_t board0;                                              //0xF03C
      uint32_t revis3;                                              //0xF040
      uint32_t revis2;                                              //0xF044
      uint32_t revis1;                                              //0xF048
      uint32_t revis0;                                              //0xF04C
      uint32_t reserved[12];                                        //0xF050 - 0xF080
      uint32_t sernum1;                                             //0xF080
      uint32_t sernum0;                                             //0xF084
    }st_configuration_rom;    
    
    

    // VX1742 Buffer Layout
    typedef volatile struct s_vx1742_regs_t{
      uint32_t output_buffer;                                      //0x0000
      uint32_t dummy1[1023];                                       //0x0004 - 0x0FFC
      uint32_t dummy2[32];                                         //0x0FFC - 0x1080
      st_group_n_conf group_n_conf[4];                             //0x1080 - 0x1480
      uint32_t dummy3[6880];                                       //0x1480 - 0x8000
      st_group_conf group_conf;                                    //0x8000 
      uint32_t group_config_bit_set;                               //0x8004
      uint32_t group_config_bit_clear;                             //0x8008
      uint32_t buffer_organisation;                                //0x800C
      uint32_t dummy4[4];                                          //0x8010 - 0x8020
      uint32_t custom_size;                                        //0x8020
      uint32_t dummy5[22];                                         //0x8024 - 0x807C
      uint32_t initial_testwave;                                   //0x807C
      uint32_t dummy6[22];                                         //0x8080 - 0x80D8
      uint32_t sampling_frequency;                                 //0x80D8
      uint32_t dummy7[9];                                          //0x80DC - 0x8100
      st_acq_control acq_control;                                  //0x8100
      st_acq_status acq_status;                                    //0x8104
      uint32_t software_trigger;                                   //0x8108
      st_trigger_source trigger_src;                               //0x810C
      st_trigger_out_mask trigger_out_mask;                        //0x8110
      uint32_t post_trigger;                                       //0x8114
      uint32_t front_panel_io_data;                                //0x8118
      st_front_panel_io_control front_panel_io_control;            //0x811C
      st_group_en_mask group_en_mask;                              //0x8120
      uint32_t mother_roc_firmware;                                //0x8124
      uint32_t dummy8;                                             //0x8128 - 0812C
      uint32_t events_stored;                                      //0x812C
      uint32_t dummy9[2];                                          //0x8130 - 0x8138
      uint32_t monitor_dac;                                        //0x8138
      uint32_t dummy10;                                            //0x813C - 0x8140
      st_board_info board_info;                                    //0x8140
      uint32_t monitor_mode;                                       //0x8144
      uint32_t dummy11;                                            //0x8148 - 0x814C
      uint32_t event_size;                                         //0x814C
      uint32_t dummy12[7020];                                      //0x8150 - 0xEF00
      st_vme_control vme_control;                                  //0xEF00
      st_vme_status vme_status;                                    //0xEF04
      uint32_t board_id;                                           //0xEF08
      uint32_t mc_base_control;                                    //0xEF0C
      uint32_t reloc_addr;                                         //0xEF10
      uint32_t intrr_stat_id;                                      //0xEF14
      uint32_t intrr_event_nr;                                     //0xEF18
      uint32_t blt_event_number;                                   //0xEF1C
      uint32_t scratch;                                            //0xEF20
      uint32_t software_reset;                                     //0xEF24
      uint32_t software_clear;                                     //0xEF28
      uint32_t flash_enable;                                       //0xEF2C
      uint32_t flash_data;                                         //0xEF30
      uint32_t configuration_reload;                               //0xEF34
      uint32_t dummy13[50];                                        //0xEF38 - 0xF000                
      st_configuration_rom configuration_rom;                      //0xF000 - 0xF088                      
      uint32_t dummy14[221];                                       //0xF088 - 0xF3FC                    
    }vx1742_regs_t;

}
#endif