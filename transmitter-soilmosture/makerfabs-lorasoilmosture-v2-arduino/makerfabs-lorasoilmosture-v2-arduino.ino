#include <RH_RF95.h>
#include <ArduinoUniqueID.h>

static int sensorPin = A2;    // select the input pin for the potentiometer
static int sensorValue = 0;  // variable to store the value coming from the sensor
static int sensorPowerCtrlPin = 5;

static float slope = 1.55; // slope from linear fit
static float intercept = -0.6; // intercept from linear fit

static const String sid;
static char hex[19];

static float voltage;
static float vol_water_cont;
static int vol_water_cont_1e2;

static int16_t packetnum = 1;  // packet counter

void sensorPowerOn(void)
{
  digitalWrite(sensorPowerCtrlPin, HIGH);//Sensor power on 
}
void sensorPowerOff(void)
{
  digitalWrite(sensorPowerCtrlPin, LOW);//Sensor power off
}

#define RFM95_CS 10
#define RFM95_RST 4
#define RFM95_INT 2

#define RF95_FREQ 868.0

// Singleton instance of the radio driver
RH_RF95 rf95(RFM95_CS, RFM95_INT);

void setup() 
{
  pinMode(RFM95_RST, OUTPUT);
  digitalWrite(RFM95_RST, LOW);
  delay(100);
  digitalWrite(RFM95_RST, HIGH);

  pinMode(sensorPowerCtrlPin, OUTPUT);
  sensorPowerOn();

  Serial.begin(115200);
  delay(100);
  UniqueIDdump(Serial);

  for (size_t i = 0; i < UniqueIDsize; i++)  {
    sprintf(&hex[i*2],"%02x", UniqueID[i]);
  }             
  sid += hex;
  // manual reset
  digitalWrite(RFM95_RST, LOW);
  delay(10);
  digitalWrite(RFM95_RST, HIGH);
  delay(10);

  while(!rf95.init()) {
    Serial.println("LoRa radio init failed");
    while (1);
  }
  Serial.println("LoRa radio init OK!");

  rf95.setTxPower(20,false);
  rf95.setSpreadingFactor(9);
  rf95.setSignalBandwidth(125000);
  rf95.setCodingRate4(5);
  rf95.setPreambleLength(8);
  if (!rf95.setFrequency(RF95_FREQ)) {
    Serial.println("setFrequency failed");
    while (1);
  }
  Serial.print("Set Freq to: "); 
  Serial.println(RF95_FREQ);
}

void loop()
{
  sensorPowerOn();//
  delay(100);
  sensorValue = analogRead(sensorPin);
  delay(200);
  sensorPowerOff();
  //Serial.print(F("Moisture ADC : "));
  //Serial.println(sensorValue);
  //Serial.print("Voltage: ");
  voltage = ((float(sensorValue)/1023.0)*3.3);
  //Serial.print(voltage); // read sensor
  //Serial.print(" V, Theta_v: ");
  vol_water_cont = (((1.0/voltage)*slope)+intercept); // calc of theta_v (vol. water content)
  vol_water_cont_1e2 = (int)(vol_water_cont * 1e2);
  //Serial.print(vol_water_cont);
  //Serial.println(" cm^3/cm^3"); // cm^3/cm^3

  const String message = "{\"sid\":\""+(String)sid
  +"\",\"msgid\":"+(String)packetnum
  //+",\"adc\":"+(String)sensorValue
  //+",\"V\":"+(String)voltage
  +",\"vwc\":"+(String)vol_water_cont_1e2+"}";
  //Serial.println(message);
  delay(100);
  packetnum++;
  //Serial.println("Transmit: Sending to rf95_server");
  
  // // Send a message to rf95_server
  const uint8_t radioPacketLen = message.length()+1;
  uint8_t radioPacket[radioPacketLen];
  message.toCharArray(radioPacket, sizeof(radioPacket));
  
  radioPacket[radioPacketLen-1] = '\0';

  for(uint8_t i = 0; i < sizeof(radioPacket); i++ )
	{
		Serial.print(char(radioPacket[i]));
	}
	Serial.print("\n");
  
  //Serial.println("Sending..."); delay(10);
  rf95.send((uint8_t *)radioPacket, sizeof(radioPacket)); 
  //Serial.println("Waiting for packet to complete..."); delay(10);
  rf95.waitPacketSent();
  //Serial.println("Packet sent"); delay(10);
  delay(30*60*1000);
}
