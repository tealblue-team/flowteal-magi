#include <RH_RF95.h>
#include <ArduinoUniqueID.h>

int sensorPin = A2;    // select the input pin for the potentiometer
int sensorValue = 0;  // variable to store the value coming from the sensor
int sensorPowerCtrlPin = 5;

float slope = 1.55; // slope from linear fit
float intercept = -0.6; // intercept from linear fit

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
	Serial.print("UniqueID: ");
	for (size_t i = 0; i < UniqueIDsize; i++)
	{
		if (UniqueID[i] < 0x10)
			Serial.print("0");
		Serial.print(UniqueID[i], HEX);
		Serial.print(" ");
	}
	Serial.println();

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

int16_t packetnum = 0;  // packet counter, we increment per xmission

void loop()
{
  sensorPowerOn();//
  delay(100);
  sensorValue = analogRead(sensorPin);
  delay(200);
  sensorPowerOff();
  Serial.print(F("Moisture ADC : "));
  Serial.println(sensorValue);
  float voltage,vol_water_cont; // preallocate to approx. voltage and theta_v
  Serial.print("Voltage: ");
  voltage = (float(sensorValue)/1023.0)*3.3;
  Serial.print(voltage); // read sensor
  Serial.print(" V, Theta_v: ");
  vol_water_cont = ((1.0/voltage)*slope)+intercept; // calc of theta_v (vol. water content)
  Serial.print(vol_water_cont);
  Serial.println(" cm^3/cm^3"); // cm^3/cm^3


  String message = "{\"msgid\":"+(String)packetnum+";\"adc\":"+(String)sensorValue+";\"V\":"+(String)voltage+";\"vwc\":"+(String)vol_water_cont+"}";
  Serial.println(message);
  delay(100);
  packetnum++;
  Serial.println("Transmit: Sending to rf95_server");
  
  // // Send a message to rf95_server

  uint8_t radioPacket[message.length()+1];
  message.toCharArray(radioPacket, message.length()+1);
  
  radioPacket[message.length()+1]= '\0';

  Serial.println("Sending..."); delay(10);
  rf95.send((uint8_t *)radioPacket, message.length()+1); 
  Serial.println("Waiting for packet to complete..."); delay(10);
  rf95.waitPacketSent();
  Serial.println("Packet sent"); delay(10);
  delay(1000);
}
