#ifndef TRIGGER_CONTROL_H
#define TRIGGER_CONTROL_H
#include<stdio.h>
#include<string.h>    //strlen
#include <vector>
#include<sys/socket.h>
#include<arpa/inet.h> //inet_addr
#include "http_responce_pars.h"
#include "trigger_logic_tpc_stream.h"
#include <libconfig.h++>
#include <exception>
/************************************************************************//**
 *  trigger_controll cllass 
 *  a class for contraling the trigger box. 
 *
 *  Inless sted otherwise all set comads sond the setting to the triger box.
 *  set_scintillator_delay, set_plane_*_delay and set_pad_delay do not you
 *  must call  set_delays(); to send all the input delays at once.
 *
 *  requires: libconfig
 *  @see Triger_Logic_tpc_Stream for read out. 
 **************************************************************************/
class trigger_control{

public:
    trigger_control();

    unsigned short n_planes;
    template <typename T>
    int set_value(const std::string& cmd, T value, T & field);
    template <typename T, typename Q=int>
    int set_value(const std::string& cmd, T value, const std::string & cmd2="", Q value2=0);
    /*********************************************************************//*!
     * set the delays locally, set_delays must be called to send all deays to the trigger box
     * @param d the input delay for the scintillator -  
     * @see set_delays()
     ************************************************************************/
    void set_scintillator_delay(int d) { scintillator_delay =d; }
    void set_plane_delay(size_t i_plane, int d) { plane_delays.at(i_plane) = d; }
    void set_pad_delay(int d) { pad_delay = d; };

    /*******************************************************************//*!
     * @return the delay stored in the class. set by @set
     * @see set_scintillator_delay()
     ************************************************************************/
    int get_scintillator_delay() { return scintillator_delay; }
    int get_plane_delay(size_t i_plane) { return plane_delays.at(i_plane); }
    int get_pad_delay() { return pad_delay; }

    /*******************************************************************//*!
     * sends the set dellays stored in the class to the triger box 
     * @return 0 on sucess 1 an error 
     * @see set_*_delay()
     * @see get_error_str()
     ************************************************************************/
    int set_delays();   //loads all delays to the fpga must set all indivdualy first
    /*******************************************************************//*!
     * send the stored coincidence edge width to the trigger box;
     * @return 0 on sucess 1 on error
     * @see set_coincidence_pulse_width()
     ************************************************************************/
    int send_coincidence_edge_width(); //resend the stored coincidence_edge_width
    /*******************************************************************//*!
     * send the stored coincidence edge width to the trigger box;
     * @return 0 on sucess 1 on error
     * @see set_coincidence_pulse_width()
     * @see get_coincidence_pulse_width()
     ************************************************************************/
    int send_coincidence_pulse_width(); //resend the stored coincidence_pulse_width


    /*******************************************************************//*!
     * send the command to clear all count registers.
     * this must be called on starup after enable has been set
     * @return 0 on sucess 1 on error
     ************************************************************************/
//    int clear_triggercounts();

    /*******************************************************************//*!
     * Set the delay for trigger output 1 + 2 and 3
     * the delay is 12 bits. trig 1 is stored in 11 downto 0
     * trig 2 delay is stored in 23 downto 12
     * @return 0 on sucess 1 on error
     ************************************************************************/
    int set_trigger_12_delay(int delay);
    int set_trigger_3_delay(unsigned short delay);
    std::vector<unsigned short> get_trigger_delays() { return trigger_delays; }
    /*******************************************************************//*!
     * @return 0 on sucess 1 on error
     ************************************************************************/
//    int current_reset();
//    int clear_coni_disable(int mask);
    /*******************************************************************//*!
     * DONOT USE 
     *
     * Use Triger_Logic_tpc_Stream class for readback. 
     ************************************************************************/
//    tuc::Readout_Data* read_back(); //DO NOT USE use stream readout
    /*******************************************************************//*!
     * Get the error string form the last send command .
     * @return pointer to a string describing the last error
     ************************************************************************/
//    char * get_error_str() { return error_str; }
    /*******************************************************************//*!
     * Reset all counters on the triger box. This must be called during setup.
     * @return 0 on sucess 1 on error
     ************************************************************************/
    int reset_counts();
    /*******************************************************************//*!
     * Sets the globle enable. This is the trigger enable and others.
     * @param true = enable false = disable
     * @return 0 on sucess 1 on error
     ************************************************************************/
    int enable(bool state);
    /*******************************************************************//*!
     * Sets the coincidence pulse width in 2.5 ns divisons 
     * pulse_width is stored localy and sent to the trigger box
     * @param width - the nuber of 2.5 ns perids to hold coincidence out high
     * @return 0 on sucess 1 an error 
     * @see send_coincidence_pulse_width() */
    int set_coincidence_pulse_width(int width);
    int get_coincidence_pulse_width() { return coincidence_pulse_width; }
    /*******************************************************************//*!
     * Sets the coincidence input edge width locally and sents it to the trigger box
     * coincidence edge width is how many clk cycles to hold a rising edge of
     * the input signals for detecting a coincidence.  i.e. if coincidence
     * edge width is set to 3 their can be a riseing edges on inputs within a
     * 3 clk cycle window and a coincidence pulse will be generated.
     * @param width - the nuber of 2.5 ns perids to hold trigger edges high going in to the coincidence unit
     * @return 0 on sucess 1 an error */
    int set_coincidence_edge_width(int width);
    int get_coincidence_edge_width() { return coincidence_edge_width; }
    /*******************************************************************//*!
     * Sets the pulser frequency and pulse width
     * @param freq - pulser frequancy in Hz
     * @param width- the number of 2.5 ns clk cycles to hold pulse high, max 20000
     * @return 0 on sucess 1 an error */
    int set_pulser(double freq, int width);
    /*******************************************************************//*!
     * Sets the mask of which inputs to use for determing a coincidence
     * mask bits are as follows:
     *     0 - sicinilator 
     *     1 - plane 1 
     *     2 - plane 2 
     *     3 - plane 3 
     *     4 - plane 4 
     *     5 - plane 5 
     *     6 - plane 6 
     *     7 - plane 7 
     *     8 - plane 8 
     *     9 - pad 
     * @param en - enable maks 
     * @return 0 on sucess 1 an error 
     ************************************************************************/
    int set_coincidence_enable(int en);
    /*******************************************************************//*!
     * Set the prescaler scaler 
     * @param scaler - nubre of coincidence required to generate a prescaler 
     *		       pullse. Range 1 cto 2^10-1
     * @return 0 on sucess 1 an error 
     ************************************************************************/
    int set_prescaler(int scaler);
    /*******************************************************************//*!
     * Un implimented do not use 
     *************************************************************************/
//    int set_mux(int mux_comand); // not iwplimented do not use
    /*******************************************************************//*!
     * @param delay - the prescaler delay  must be > 4
     ************************************************************************/
    int set_prescaler_delay(int delay);
    /*******************************************************************//*!
     * @param delay - the pulser delay must be > 4
     ************************************************************************/
    int set_pulser_delay(int delay);
     /*******************************************************************//*!
     * Store handshake in the class and send it the trigger box.
     * Handshake delay is vt1 of the handshake unit. this is the veto hold 
     * time beond the end of a busy pulse.  
     * the time of the deay is in units of 2.5 nS clock pulses
     * @return 0 on sucess 1 on error
     * @see send_handshake_delay()
     ************************************************************************/
    int set_handshake_delay(int delay);
    int get_handshake_delay() { return handshake_delay; }

    /*******************************************************************//*!
     * Send the two phase settings packed in one int
     * @param phases two 4bit phase settings for the 40MHz clk gen packed in an int
     * @return 0 on sucess 1 on error
     ************************************************************************/
    int set_clk40_phases(int phase1, int phase2);
    int get_clk40_phase1() { return clk40_phase1; }
    int get_clk40_phase2() { return clk40_phase2; }
    
    /*******************************************************************//*!
     * @param: 0=negative polarity/1=positive polarity, LSB of an int
               bit0: polarity of pulser 1
               bit1: polarity of pulser 2
               eg: 000000000000000010 means positive pulser 2, negative pulser1
     * @return 0 on sucess 1 on error
     ************************************************************************/
    int set_pulser_polarity(bool pol_pulser1, bool pol_pulser2);
    unsigned short get_pulser_polarities() { return pulser_polarity; };
    int send_handshake_delay();
    /*******************************************************************//*!
     * Set the handshake mask and send it to the trigger box.
     * @param mask - the lower 4 bit corispond to M1 - M4 of the handshake unit 
     * @return 0 on sucess 1 on error
     * @see send_handshake_mask()
     * @see get_handshake_mask()
     ************************************************************************/
    int set_handshake_mask(int mask);
    int get_handshake_mask() { return handshake_mask; }
    int send_handshake_mask();
    /*******************************************************************//*!
     * Set the current time in the trigger box.  This sends the current
     * unix  time stamp * 1000 to the triger box. 
     * Time on the trigger box is a mS counter stored as a 64 bit unsigned intiger 
     * @return 0 on sucess 1 on error */
    int set_time();
    /**********************************************************************//*!
     * loads all saved setting to the trigger box form a settings file.
     * @param fname - pointer to the config file name to load to the fpga in the 
     *         trigger box
     * @return 0 on sucess
     *************************************************************************/
//    int load_from_file(char *fname);
    /**********************************************************************//*!
     * set the ip adress of the trigger controll box
     * @param ip_address - the ip address of the trigger controll box
     *************************************************************************/
    void set_ip_adr(std::string ip_address) { ip_adr = std::move(ip_address); }
    std::string get_ip_adr() { return ip_adr; }

private:
    int http_backend(char * command);
    int http_backend(const std::string & cmd) { return http_backend(const_cast<char *>(cmd.c_str())); }
    int scintillator_delay;
    std::vector<int> plane_delays;
    std::vector<unsigned short> trigger_delays;
    int pad_delay;
    http_responce_pars * parser;
    char error_str[255];
    int coincidence_pulse_width;
    int coincidence_edge_width;
    int handshake_mask;
    int handshake_delay;
    int clk40_phase1;
    int clk40_phase2;
    unsigned short pulser_polarity;
    std::string ip_adr;
};
class tu_program_exception: public std::exception {

  const std::string _msg;

public:
  explicit
  tu_program_exception(std::string msg=""): _msg("TUProgramError: " +  std::move(msg)) { }
  const char* what() const noexcept override { return _msg.c_str(); }
};

#endif // TRIGGER_CONTROL_H
