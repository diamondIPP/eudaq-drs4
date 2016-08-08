/* ---------------------------------------------------------------------------------
** Interface to Artix-7 (AC701 Development Board) via HTTP server running on board.
** Based on work done at OSU
**
** <trigger_logic_tpc_stream>.h
** 
** Date: August 2016
** Author: Christian Dorfer (dorfer@phys.ethz.ch)
** ---------------------------------------------------------------------------------*/


#ifndef TRIGGER_LOGIC_TPC_STREAM_H
#define TRIGGER_LOGIC_TPC_STREAM_H

#include <string>
#include "TUDEFS.h"

typedef void (*Triger_CallbackType)(int);

class Triger_Logic_tpc_Stream{
   private:
   int socket_desc;
   int error;
   bool is_socket_open;
   tuc::Readout_Data *pars_stream_ret(char *stream);
   std::string ip_adr;

   public:
    Triger_Logic_tpc_Stream();

    /*******************************************************************//*!
     * Dumps redout data to the screen
     * @param data read out from the trigger box
     *************************************************************************/
    void dump_readout(tuc::Readout_Data *readout);

   /*******************************************************************//*!
    * Returs the stataus of the socket conection to the trigger box.  
    *
    * It dese not test the connection to see if it is god or not just if 
    * the socket is open or not.
    * @return true on connected fase on disconnected 
    * @see set_coincidence_pulse_width()
    *************************************************************************/
    bool is_open();

   /*******************************************************************//*!
    * Open the socket connecion to the trigger box for readback.
    *************************************************************************/
   int open();

   /*******************************************************************//*!
    * Closes the socket connecion to the trigger box for readback.
    * @return 0 on sucess true on fail
    ************************************************************************/
   int close();

   /**********************************************************************//*!
    * set the ip adress of the trigger controll box
    * @param ip_address - the ip address of the trigger controll box
    *************************************************************************/
   void set_ip_adr(std::string);

   /**********************************************************************//*!
    * gets the ip adress of the trigger controll box
    * @return std::string contiaining the ip address.
    *************************************************************************/
   std::string get_ip_adr();

   /*******************************************************************//*!
    * Des the actual readout form the trigger box.
    * @return pointer to a Readout_Data strucuer on sucess NULL on fail
    ************************************************************************/
   tuc::Readout_Data *timer_handler();
};


#endif
