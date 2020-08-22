#ifndef rwt_sim_h
#define rwt_sim_h

#include "SoftwareSerial.h"
#include "Arduino.h"

class rwt_sim
{
private:
    String readSerial();
    
    int timeout;
	  String buffer;
public:
    String writeCommand(String const &command);
    String updateSerial();
    bool initialize();
    int getStatus();
    void signalQuality();
    bool pickUp();
    void callNumber(char* number);
    bool speedDialing(int position);
    bool hangUp();
    bool speedDialingProgramming(int position, String number);
};




#endif //!rwt_sim_h
