#include <SPI.h>
#include <Ethernet.h>
#include <EthernetUdp.h>

#include <JeeLib.h> // https://github.com/jcw/jeelib
#include "DHT.h"

#include <Wire.h>
#include <OneWire.h>

// Fixed RF12 settings
#define MYNODE 30            //node ID of the receiever
#define freq RF12_433MHZ     //frequency
#define group 210            //network group

// buffers for receiving and sending data
char packetBuffer[UDP_TX_PACKET_MAX_SIZE]; //buffer to hold incoming packet,

// Enter a MAC address for your controller below.
// Newer Ethernet shields have a MAC address printed on a sticker on the shield
byte mac[] = {  
  0x00, 0xAA, 0xBB, 0xCC, 0xDE, 0x02 };
// Initialize the Ethernet client library
// with the IP address and port of the server 
// that you want to connect to (port 80 is default for HTTP):
EthernetClient client;
EthernetUDP Udp;
//IPAddress receiverIP(192, 168, 0, 195);
IPAddress receiverIP(95, 85, 39, 222);
unsigned int receiverPort = 6001; 
unsigned int arduinoPort = 8888; 

//RF12 variables
typedef struct {
  int rxD;              // sensor value
  int supplyV;          // tx voltage
  int data1;
} 
Payload;
Payload rx;

//Variables

int millivolts = 0;
int nodeID;   
int nullbat = 0;

//  DHT22 
DHT dht;
//  SD
const int chipSelect = 10; //SS for Arduino UNO


void setup () {

  Serial.begin(9600);
  rf12_initialize(MYNODE, freq,group); // Initialise the RFM12B
  dht.setup(7); // Analog 3= Digital 17
  pinMode(10, OUTPUT); // For SD
  pinMode(17, INPUT); 

  if (Ethernet.begin(mac) == 0) {
    Serial.println("Failed to configure Ethernet using DHCP");
    // no point in carrying on, so do nothing forevermore:
    for(;;)
      ;
  }
  // print your local IP address:
  Serial.print("My IP address: ");
  for (byte thisByte = 0; thisByte < 4; thisByte++) {
    // print the value of each byte of the IP address:
    Serial.print(Ethernet.localIP()[thisByte], DEC);
    Serial.print("."); 
  }


#ifdef AVR
  Wire.begin();
#else
  Wire1.begin(); // Shield I2C pins connect to alt I2C bus on Arduino Due
#endif

  Udp.begin(arduinoPort);  

}

void loop() {
  getlocalvalues();
  getrf12b();

  delay(200);
}



void upload(String id, String type,int value){
  //char  ReplyBuffer[] = "acknowledged"; 
  String ReplyBuffer = id +"#"+type+"#" +value;
  int length = ReplyBuffer.length() + 1;
  char dom [length];
  Serial.print("cadena: ");
  Serial.println(ReplyBuffer);

  ReplyBuffer.toCharArray(dom, length ) ;

  Udp.beginPacket(receiverIP, receiverPort); //start udp packet
  Udp.write(dom); //write sensor data to udp packet
  Udp.endPacket(); // end packet
}


void getlocalvalues(){
  delay(2000);
  upload("01","H",(int)(dht.getHumidity()*100));
  upload("01","T",(int)(dht.getTemperature()*100));
}

void getrf12b(){
  //RFM12B reciving part
  if (rf12_recvDone() && rf12_crc == 0 && (rf12_hdr & RF12_HDR_CTL) == 0) {
    Serial.println("<<<<<<<<<<<<<<<<<====================>>>>>>>>>>>>>>>>>>>>>>>");
    nodeID = rf12_hdr & 0x1F;  // get node ID
    rx = *(Payload*) rf12_data;

    int value = rx.rxD;
    //  vout = rx.supplyV;
    
    int hum = rx.data1;
    upload((String)nodeID,"H",(int)rx.rxD);
    upload((String)nodeID,"T",(int)rx.data1);

    if (RF12_WANTS_ACK) {                  // Send ACK if requested
      rf12_sendStart(RF12_ACK_REPLY, 0, 0);
    }
  }
}


int freeRam () {
  extern int __heap_start, *__brkval; 
  int v; 
  return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval); 
}