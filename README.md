# ChannelPollingFirmware
This firmware is designed to operate on a MSP430G2553 to control two Texas Instruments (TI) MPC506AP 16:1 multiplexor effectly makeing them a 32:1. 

## Building Firware with CCS
* Download and install Code Composer Studio (CCS) http://www.ti.com/tool/CCSTUDIO
* Clone the GIT Repository to target computer
* Run CCS and create a new project with the same name as this GIT Repository
  * New project should have no files (no main)
  * New project should target the G2553 processor
  * Files from the GIT Repo should auto populate the project folder if everything was done correctly