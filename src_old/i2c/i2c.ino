// -------------------------------------------------------------------------------------------
// Basic Master
// -------------------------------------------------------------------------------------------
//
// This creates a simple I2C Master device which when triggered will send/receive a text 
// string to/from a Slave device.  It is intended to pair with a Slave device running the 
// basic_slave sketch.
//
// Pull pin12 input low to send.
// Pull pin11 input low to receive.
//
// This example code is in the public domain.
//
// -------------------------------------------------------------------------------------------

#include <i2c_t3.h>

// Memory
#define MEM_LEN 256
char databuf[MEM_LEN];
int count;
int target = 0x20;
const int IODIRA = 0x00;
const int IOCON = 0xA;
const int IOCON_BANK = 7;
const int IOCON_MIRROR = 6;
const int IOCON_SEQOP = 5;
const int IOCON_ODR = 2;
const int IOCON_INTPOL = 1;
const int IOCON_INTCC = 0;

void write_iocon(){
  int iocon_val = 0x00;
  iocon_val  == (1 << IOCON_SEQOP);
  //iocon_val  == (1 << IOCON_BANK);


  // Transmit to Slave
  Wire1.beginTransmission(target);   // Slave address
  Wire1.write(IODIRA);
  Wire1.write(0xAA);
  int err = Wire1.endTransmission();           // Transmit to Slave

  // Check if error occured
  switch(err){
    case 0:
      Serial.println("Success");
      break;
    case 1:
      Serial.println("Data too long");
      break;
    case 2:
      Serial.println("recv addr NACK");
      break;
    case 3:
      Serial.println("recv data NACK");
      break;
    case 4:
      Serial.println("Other error");
      break;
  }
}

void setup()
{
    pinMode(LED_BUILTIN,OUTPUT);    // LED
    digitalWrite(LED_BUILTIN,LOW);  // LED off
    pinMode(12,INPUT_PULLUP);       // Control for Send
    pinMode(11,INPUT_PULLUP);       // Control for Receive

    // Setup for Master mode, pins 18/19, external pullups, 400kHz, 200ms default timeout
    Wire1.begin(I2C_MASTER, 0x00, I2C_PINS_29_30, I2C_PULLUP_EXT, 400000);
    Wire1.setDefaultTimeout(200000); // 200ms

    // Data init
    memset(databuf, 0, sizeof(databuf));
    count = 0;

    Serial.begin(115200);
}

void loop()
{
    size_t idx;

    // Send string to Slave
    //
    if(true)
    {
        digitalWrite(LED_BUILTIN,HIGH);   // LED on

        // Construct data message
        sprintf(databuf, "Data Message #%d", count++);

        // Print message
        Serial.printf("Sending to Slave: '%s' ", databuf);
        Serial.println();
        write_iocon();

        

        digitalWrite(LED_BUILTIN,LOW);    // LED off
        delay(100);                       // Delay to space out tests
    }

    delay(1000);
    // Read string from Slave
    //
//    if(true)
//    {
//        digitalWrite(LED_BUILTIN,HIGH);   // LED on
//
//        // Print message
//        Serial.print("Reading from Slave: ");
//        
//        // Read from Slave
//        Wire1.requestFrom(target, (size_t)MEM_LEN); // Read from Slave (string len unknown, request full buffer)
//
//        // Check if error occured
//        if(Wire1.getError())
//            Serial.print("FAIL\n");
//        else
//        {
//            // If no error then read Rx data into buffer and print
//            idx = 0;
//            while(Wire1.available())
//                databuf[idx++] = Wire1.readByte();
//            Serial.printf("'%s' OK\n",databuf);
//        }
//
//        digitalWrite(LED_BUILTIN,LOW);    // LED off
//        delay(100);                       // Delay to space out tests
//    }
//
//    delay(1000);
}

