/*
  ATmega128RFA1 Dev Board Basic Chat
  by: Jim Lindblom
      SparkFun Electronics
  date: July 3, 2012
  License: Beerware. Feel free to use, reuse, and modify this code
  as you please. If you find it useful, you can buy me a beer.

  This code sets up the ATmega128RFA1's wireless transciever in
  the most basic way possible to serve as a serial gateway.
  Serial into the ATmega128RFA1's UART0 will go out the RF radio.
  Data into the RF radio will go out the MCU's UART0.
*/

#include "RadioFunctions.h"
enum BoardType { Unknown, Sender, Receiver};

char startSignal = '1';
char messageSignal = '5';
char terminalSignal = '7';
int counter = 0;
int numOfPackets = 3000;
int bufferSize = 127;
char charStr[127];
unsigned long timeOutStartTime = 0;
BoardType type = Unknown;
bool started;
void setup()
{
  for (int i=0; i< bufferSize; i++) {
    charStr[i] = messageSignal;
  }
  Serial.begin(9600);  // Start up serial
  rfBegin(16);  // Initialize ATmega128RFA1 radio on channel 11 (can be 11-26)
}

void loop()
{
  
  if (!started)  
  {
    // If serial comes in and we haven't started transmission
    // This marks this board as the receiving end of the transmission.
    if (Serial.available() && (type != Sender)) {
      char serialInput = Serial.read();
      if (serialInput == startSignal) {
        type = Receiver;
        Serial.print("You typed: ");
        Serial.print(serialInput);
        Serial.print("\n");
        rfWrite(serialInput); // Send a starting signal to the other board.
        started = true;
        // Set the start time of the transmission from the reciving end to control waiting time in case of a packet drop.
        timeOutStartTime = micros();
      }
    } else if ( rfAvailable() && (type != Receiver)) {
      // This marks this board as the sending end of the transmission.
      char readInput = rfRead();
      if (readInput == startSignal) {
        type = Sender;
        Serial.print("Recieved Start Bit: ");
        Serial.print(readInput);
        Serial.print("\n");
        started = true;
      }
    }
  }

  if (started) 
  {
    // This runs when it is the reciving end of the transmission.
    if (rfAvailable() && type == Receiver) {// If data receievd on radio...
      // This runs on the reciving end of the transmission.
      char rfReadChar = rfRead();
      // Count how many message bytes we can collect.
      if (rfReadChar == messageSignal) {
        counter++;
      } else if (rfReadChar == terminalSignal ) {
        Serial.print("Done\n");
        MarkTermination();
      } else if (micros() - timeOutStartTime > 2000000) {
        Serial.print("Timeout!\n");
        MarkTermination();
      }
    } else if (!rfAvailable() && type == Sender){
      String str(charStr);
      unsigned long startTime = micros();
      // Loop to send packets.
      for (int i=0; i<numOfPackets; ++i) {
        // to test maximum speed, uncomment this line, and sends packets with maximum buffersize.
        rfPrint(str); 
      }
      unsigned long timePast = micros() - startTime;
      Serial.print("Time used: ");
      Serial.print(timePast);
      Serial.print(" ms.\n");
      // Transmits a non-message signal to indicate the termination of this test session.
      rfWrite(terminalSignal);
      Serial.print("\nSent (bytes): ");
      Serial.print(numOfPackets * bufferSize);
      Serial.print("\n\n\n");

      // turn off the transmission.
      started = false; 
    }
  } 
}

// Print the counter, mark the end of the test session and reset counter and timerout timer.
void MarkTermination() {
  Serial.print("Time from start:");
  Serial.print(micros()-timeOutStartTime);
  Serial.print("\n");
  Serial.print("Counter:");
  Serial.print(counter);
  Serial.print("\n");      
  // Mark the end of this test.
  started = false; 
  // Reset the counter.
  counter = 0; 
  timeOutStartTime = 0;
}

