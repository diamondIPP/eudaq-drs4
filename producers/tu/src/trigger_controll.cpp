
#include "trigger_controll.h"
#include "TUDEFS.h"
#include <stdio.h>
#include <string.h>    //strlen
#include <sys/socket.h>
#include <arpa/inet.h> //inet_addr
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>
#include "iostream"

//#define HOST "128.146.33.69"
using namespace std;
#include <libconfig.h++>
using namespace libconfig;

    trigger_controll::trigger_controll():
        n_planes(8),
        clk40_phase1(0),
        clk40_phase2(0) {
        plane_delays.resize(n_planes, 0);
        trigger_delays.resize(3, 40);
        parser = new(http_responce_pars);
    }

    int trigger_controll::get_coincidence_pulse_width(){return coincidence_pulse_width;}
    int trigger_controll::get_coincidence_edge_width(){return coincidence_edge_width;}
    int trigger_controll::get_clk40_phase1(){return this->clk40_phase1;}
    int trigger_controll::get_clk40_phase2(){return this->clk40_phase2;}
    int trigger_controll::get_pulser_polarities(){return this->pulser_polarity;}


    int trigger_controll::set_delays(){
        string cmd_str = "/a?";
        cmd_str += "a0=" + to_string(scintillator_delay);
        for (auto i_d(0); i_d < n_planes; i_d++) {
            cmd_str += "a" + to_string(i_d + 1) + "=" + to_string(plane_delays.at(i_d));
        }
        cmd_str += "a" + to_string(n_planes) + "=" + to_string(pad_delay);
        return this->http_backend(cmd_str);
    }

    int trigger_controll::enable(bool state) {
        string cmd_str = state ? "/a?k=7" : "/a?k=0";
        return this->http_backend(cmd_str);
    }

    int trigger_controll::set_coincidence_pulse_width(int width){
        char cmd_str[128];
        coincidence_pulse_width = width;
        sprintf(cmd_str,"/a?r=%d",width);
        return this->http_backend(cmd_str);
    }

    int trigger_controll::set_coincidence_edge_width(int width){
        char cmd_str[128];
        coincidence_edge_width = width;
        sprintf(cmd_str,"/a?q=%d",width);
        return this->http_backend(cmd_str);
    }

    int trigger_controll::send_coincidence_pulse_width(){
        char cmd_str[128];
        sprintf(cmd_str,"/a?r=%d",coincidence_pulse_width);
        return this->http_backend(cmd_str);
    }

    int trigger_controll::send_coincidence_edge_width(){
        char cmd_str[128];
        sprintf(cmd_str,"/a?q=%d",coincidence_edge_width);
        return this->http_backend(cmd_str);
    }

    int trigger_controll::set_Pulser_freq(double freq){
        char cmd_str[128];
        sprintf(cmd_str,"/a?o=%f",freq);
        return this->http_backend(cmd_str);
    }
    
    int trigger_controll::set_Pulser_width(double freq,int width){
        char cmd_str[128];
        sprintf(cmd_str,"/a?o=%f&p=%d",freq, width);
        return this->http_backend(cmd_str);
    }

    int trigger_controll::set_handshake_delay(int delay){
        char cmd_str[128];
        this->handshake_delay = delay;
        sprintf(cmd_str,"/a?m=%d",delay);
        return this->http_backend(cmd_str);
    }

    int trigger_controll::set_handshake_mask(int mask){
        char cmd_str[128];
        this->handshake_mask = mask;
        sprintf(cmd_str,"/a?l=%d",mask);
        return this->http_backend(cmd_str);
    }

    int trigger_controll::get_handshake_delay(){
        return this->handshake_delay ;
    }

    int trigger_controll::get_handshake_mask(){
        return this->handshake_mask;
    }

    int trigger_controll::send_handshake_delay(){
        char cmd_str[128];
        sprintf(cmd_str,"/a?m=%d",this->handshake_delay );
        return this->http_backend(cmd_str);
    }

    int trigger_controll::send_handshake_mask(){
        char cmd_str[128];
        sprintf(cmd_str,"/a?l=%d",this->handshake_mask);
        return this->http_backend(cmd_str);
    }

    int trigger_controll::set_time(){
        char cmd_str[128];
        time_t t;
        unsigned int l,h;
        time(&t);
        t = t*1000;
        l = t & 0xFFFFFFFF;
        h = t >>32;
        sprintf(cmd_str,"/a?s=%u&t=%u",h,l);
        return this->http_backend(cmd_str);
    }




    /*************************************************************************
     * read_back
     * return a Readout_Data structure populated with the return data or NULL
     *************************************************************************/
    tuc::Readout_Data * trigger_controll::read_back(){
       tuc::Readout_Data * ret_data=NULL;
       if(http_backend((char*)"/a?R=1") !=0)
           return NULL;


       if(parser->get_content_length() >= tuc::TRIGGER_LOGIC_READBACK_FILE_SIZE){
           ret_data = (tuc::Readout_Data*) malloc(sizeof(tuc::Readout_Data));
           char *raw_ret = parser->get_content();
           if(ret_data == NULL)
               return NULL;
           memcpy(ret_data,raw_ret,sizeof(tuc::Readout_Data));
       }
       return ret_data;
    }




    int trigger_controll::http_backend(char * command)
    {
        int socket_desc;
        struct sockaddr_in server;
        char message[1024];
        char server_reply[2000];
        int recv_len=0;
        struct timeval tv;
        fd_set fdset;
        parser->clean_up();


        socket_desc = socket(AF_INET , SOCK_STREAM , 0);
        if (socket_desc == -1){
            //printf("Could not create socket");
            //sprintf(error_str,"Error: Could not create socket");
            return 1;
        }

        server.sin_addr.s_addr = inet_addr(this->ip_adr.c_str());
        server.sin_family = AF_INET;
        server.sin_port = htons(tuc::HOST_PORT);

        // make non blocking so it does not lockup
        fcntl(socket_desc, F_SETFL, O_NONBLOCK);

        connect(socket_desc , (struct sockaddr *)&server , sizeof(server));
        //Connect to remote server
        /*if (connect(socket_desc , (struct sockaddr *)&server , sizeof(server)) < 0)
        {
            puts("connect error");
            return 1;
        }
        */

        FD_ZERO(&fdset);
        FD_SET(socket_desc, &fdset);
        tv.tv_sec = 2;             /* 10 second timeout */
        tv.tv_usec = 0;
        if (select(socket_desc + 1, NULL, &fdset, NULL, &tv) == 1)
        {
          int so_error;
          socklen_t len = sizeof so_error;

          getsockopt(socket_desc, SOL_SOCKET, SO_ERROR, &so_error, &len);
           //see if the socket actualy connected
          if (so_error != 0) {
              //printf("Error connecting\n");
              //sprintf(error_str,"Error: Error connecting");
              close(socket_desc);
              return 1;
          }
        }
        //Send some data
        sprintf( message , "GET %s HTTP/1.0\r\n\r\n",command);
        if( send(socket_desc , message , strlen(message) , 0) < 0)
        {
            //puts("Send failed");
            //sprintf(error_str,"Error: Send failed");
            close(socket_desc);
            return 1;
        }

        int flags =0;
        if (-1 == (flags = fcntl(socket_desc, F_GETFL, 0)))
            flags = 0;
        fcntl(socket_desc, F_SETFL, flags & (!O_NONBLOCK));

        //Receive a reply from the server
        if( (recv_len = recv(socket_desc, server_reply , 2000 , 0) )< 0){
            //puts("recv failed");
            //sprintf(error_str,"Error: recv failed");
            close(socket_desc);
            return 1; //error
        }else{
            parser->pars_more(server_reply,recv_len);
            while(!parser->is_done()) //we have more data to get from the serevre
            {
                int toget = parser->get_how_much_more();
                if(toget <2000)
                    toget = 2000;
                if( (recv_len = recv(socket_desc, server_reply , toget , 0) )< 0)
                {
                    //puts("recv failed");
                    //sprintf(error_str,"Error: recv failed");
                    close(socket_desc);
                    return 1; //error
                }
                parser->pars_more(server_reply,recv_len);

            }
        }
        close(socket_desc);
        //puts(parser->get_content());
        //sprintf(error_str,"OK");

        return 0;
    }




    char * trigger_controll::get_error_str()
    {
        return this->error_str;
    }
    int trigger_controll::reset_counts()
    {
        char str[32];
        sprintf(str,"/a?c=0");
        return this->http_backend(str);
    }
    int trigger_controll::set_coincidence_enable(int en)
    {
        char str[32];
        sprintf(str,"/a?e=%d",en);
        return this->http_backend(str);
    }
    int trigger_controll::set_prescaler(int scaler)
    {
        char str[32];
        sprintf(str,"/a?f=%d",scaler);
        return this->http_backend(str);
    }
    /*
    int trigger_controll::set_mux(int mux_comand)
    {
        char str[32];
        sprintf(str,"/a?s=%d",mux_comand);
        return this->http_backend(str);
    }
    */
    int trigger_controll::set_prescaler_delay(int delay)
    {
        char str[32];
        sprintf(str,"/a?g=%d",delay);
        return this->http_backend(str);
    }
    int trigger_controll::set_pulser_delay(int delay)
    {
        char str[32];
        sprintf(str,"/a?j=%d",delay);
        return this->http_backend(str);
    }
    /* the phase ctl of the 40MHz clk is set by 2 4bit numbers packed in to one 8bit int*/
    int trigger_controll::set_clk40_phases(int phase1, int phase2)
    {   
        this->clk40_phase1 = phase1;
        this->clk40_phase2 = phase2;
        int phases = (phase2<<4) | phase1;
        if (phases > 255) {return -1;}
        char str[32];
        sprintf(str,"/a?u=%d",phases);
        return this->http_backend(str);
    }

    /** delay 1 & 2 are packed in to a 32: bit 0-11 = delay 1, bit 12-23 = delay 2 */
    int trigger_controll::set_trigger_12_delay(int delay) {
        trigger_delays.at(0) = delay & 0xfff;
        trigger_delays.at(1) = delay >> 12;
        string cmd = "/a?v=" + to_string(delay);
        return this->http_backend(cmd);
    }

    int trigger_controll::set_trigger_3_delay(unsigned short delay) {
        trigger_delays.at(2) = delay;
        string cmd = "/a?w=" + to_string(delay);
        return this->http_backend(cmd);
    }

    std::string trigger_controll::get_ip_adr()
    {
        return this->ip_adr;
    }

    int trigger_controll::load_from_file(char *fname) {

        Config * cfg;
        cfg = new Config();
        // Read the file. If there is an error, mark it and continue on.
        try {
          cfg->readFile(fname);
        } catch(const FileIOException & fioex) {
            cout << "I/O error while reading config file." << endl;
            return 1;
        } catch(const ParseException & pex) {
            cout << "Error parsing config file. " << endl;
        }

        libconfig::Setting & root = cfg->getRoot();
        try {
          root.lookupValue("ip_adr", ip_adr);
        } catch(const SettingNotFoundException & nfex) {
          ip_adr = "192.168.1.120";
        }
        if( root.exists("delays") ) {
            Setting & delays = root["delays"];
            if (delays.exists("scintillator")) { delays.lookupValue("scintillator", scintillator_delay); }
            for (auto i_plane(0); i_plane < n_planes; i_plane++){
                string key = "plane" + to_string(i_plane + 1);
                if (delays.exists(key)) { delays.lookupValue(key, plane_delays.at(i_plane)); }
            }
            if(delays.exists("pad")) { delays.lookupValue("pad", pad_delay); }
            if (set_delays() > 0) {
                cout << "Error setting delays: %s\n" << error_str << endl;
                return 1;
            }
        }
        float f;
        int i;
        if(root.exists("pulser_width"))
        {
            root.lookupValue("pulser_width",i);
        }
        if( root.exists("pulser_freq"))
        {
            root.lookupValue("pulser_freq", f);
            if(set_Pulser_width(f,i)){ //error
                printf("error setting pulser dutty");
                return 1;
            }
        }
        if( root.exists("prescaler"))
        {
            root.lookupValue("prescaler", i);
            if(set_prescaler(i))
                return 1;
        }
        if( root.exists("prescaler_delay")){
            root.lookupValue("prescaler_delay", i);
            if(set_prescaler_delay(i))
                return 1;
        }
        /*
        if(! root.exists("readout_period")) //make a default config file if settings donot exists
        {
             root.add("readout_period",Setting::TypeInt) = 1000;
        }
        */
        if(! root.exists("use_planes")) //make a default config file if settings donot exists
        {
            bool use;
            int en=0;
            Setting &use_planes = root["use_planes"];
            if(use_planes.exists("scintillator")){
                use_planes.lookupValue("scintillator", use);
                en =(use);
            }
            if(use_planes.exists("plane1")){
                use_planes.lookupValue("plane1", en);
                use += en<<1;
            }
            if(use_planes.exists("plane2")){
                use_planes.lookupValue("plane2", en);
                use += en<<2;
            }
            if(use_planes.exists("plane3")){
               use_planes.lookupValue("plane3", en);
               use += en<<3;
            }
            if(use_planes.exists("plane4")){
                use_planes.lookupValue("plane4", en);
                use += en<<4;
            }
            if(use_planes.exists("plane5")){
                use_planes.lookupValue("plane5", en);
                use += en<<5;
            }
            if(use_planes.exists("plane6")){
                use_planes.lookupValue("plane6", en);
                use += en<<6;
            }
            if(use_planes.exists("plane7")){
                use_planes.lookupValue("plane7", en);
                use += en<<7;
            }
            if(use_planes.exists("plane8")){
                use_planes.lookupValue("plane8", en);
                use += en<<8;
            }
            if(use_planes.exists("pad")){
                use_planes.lookupValue("pad", en);
                use += en<<9;
            }

            if(set_coincidence_enable(en))
                return 1;
        }
        if( root.exists("coincidence_edge_width"))
        {
            root.lookupValue("coincidence_edge_width", i) ;
            if(set_coincidence_edge_width(i))
                return 1;
        }
        if(root.exists("coincidence_pulse_width"))
        {
            root.lookupValue("coincidence_pulse_width", i) ;
            if(set_coincidence_pulse_width(i))
                return 1;
        }
        if( root.exists("handshake_mask"))
        {
            root.lookupValue("handshake_mask", i) ;
            if(set_handshake_mask(i))
                return 1;
        }
        if(root.exists("handshake_delay"))
        {
            root.lookupValue("handshake_delay", i) ;
            if(set_handshake_delay(i))
                return 1;
        }
        int delay = 0;
        if(root.exists("trig_1_delay"))
        {
            root.lookupValue("trig_1_delay", delay) ;
        }
        if(root.exists("trig_2_delay"))
        {
            root.lookupValue("trig_2_delay", i) ;
            delay = (delay & 0x0FF) | i <<12;
            if(set_trigger_12_delay(delay))
                return 1;
        }
        if(root.exists("trig_3_delay"))
        {
            root.lookupValue("trig_3_delay", i) ;
            if(set_trigger_3_delay(i))
                return 1;
        }
        this->set_time();//send the curent time stamp
        return 0;
    }
