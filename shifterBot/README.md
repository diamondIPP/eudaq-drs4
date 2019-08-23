# EUDAQ Shifter Bot

 - EUDAQ run bots for the PSI beam tests communicating via Google Sheets

##Main Bot 

 - start on the DAQ computer
 - location eudaq-dir/shifterBot

### Config
 - get the pixel positions of the config, start and stop buttons and the flux field with python mouse.py
 - add them to conf/eudaq.ini

### Running

 - ALWAYS start the bot after a EUDAQ run was started
 - command: ./shifter.py (alias ShifterBot)
 -  optional arguments:
    - -t, test mode
    - -n, --no_restart
    - -s, --manual_select
    - -c, --reconfigure
    - -o, --online_mon, also starts OnlineMonitor for the started run
 - starts a selecion of which bot to choose (pixel, pad, voltage scan)
 - procedure after a run has finished:
    - fills the runplan in the google sheet
    - waits for the Collimator Bot to set the checkmark of the finished run 
    - configures EUDAQ (optional)
    - restarts a new run
    - idles until the endrun log was 
    
## Collimator Bot

 - directory on Setpoint PC: /home/acsop/Desktop/venv/
 - enter: 'source bin/activate' to start the virtual environment

###Config

 - getting pixel positions of Setpoint with bin/python mouse.py:
    - empty area 
    - field of the set-value of first collimator
    - field of the read-value of first collimator
 - measure the distance between two rows with bin/python mouse.py
 - enter values in eudaq-drs4/shifterBot/config/psi.ini
 
### Running
 - AFTER the run was started run:
 - cd /home/acsop/venv/eudaq-drs4/shifterBot
 - ../../bin/ipython set_collimators.py
 - in ipython:
    - manually adjust collimators with set/adjust_fs*(value)
    - start bot with z.run()
 - bot waits until the next checkmark in 'Status Run' is set and then sets the collimators for the next run
 - when it's finished it makes the 'Collim' checkmark which gives the signal for the Main Bot to start the nex run
 