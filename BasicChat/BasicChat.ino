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

const unsigned long MAX_WAIT_TIME = 20000000; // microseconds, 10 seconds.
char startSignal = '1'; // start signal in one byte.
char messageSignal = '5'; // message signal in one byte.
char terminalSignal = '7'; // termination signal in one byte that indicates the end of the test session.
unsigned long counter = 0; // counts the number bytes rfRead recevied.
unsigned long numOfPackets = 1000; // number of packets to send.
unsigned long bufferSize = 125; // number of bytes in one packet.
char charStr[125];
unsigned long timeOutStartTime = 0; // keep track of start of a test session.
BoardType type = Unknown; // initialize the type of this board.
bool started = false; // initialize the start flag for each session.

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
    if (Serial.available() && (type != Sender)) {
      TrySendStartSignal();
    } else if ( rfAvailable() && (type != Receiver)) { // If RF comes in and no serial comes in and we haven't started transmission.
      TryReceiveStartSignal();
    }
  }

  if (started) 
  {
    // This runs when it is the reciving end of the transmission.
    if (rfAvailable() && type == Receiver) {// If data receievd on radio...
      ReceiveAndCount();
    } else if (!rfAvailable()){
      
      if(type == Sender){
        SendSignals();
      } else if (type == Receiver && micros() - timeOutStartTime > MAX_WAIT_TIME) {
      
        // If it is the receiver side that has started counting over 2 seconds without receviing any more data, mark as overtime.
        Serial.print("Timeout!\n");
        MarkTermination();
      }
    }
  } 
}

void ReceiveAndCount() {
  // This runs on the reciving end of the transmission.
  char rfReadChar = rfRead();
  // Count how many message bytes we can collect.
  if (rfReadChar == messageSignal) {
    counter++;
  } else if (rfReadChar == terminalSignal ) {
    Serial.print("Done\n");
    MarkTermination();
  } 
}

// If this is the sender, send as much data as possible.
void SendSignals(){
  // Construct the string.
  String str(charStr);

  // Mark the start time of the sending process.
  unsigned long startTime = micros();
  
  // Loop to send a number of packets.
  for (int i=0; i<numOfPackets; ++i) {
    // to test maximum speed, uncomment this line, and sends packets with maximum buffersize.
    rfPrint(str);
  }

  // Mark the end of the sending process.
  unsigned long timeUsed = micros() - startTime;

  // Transmits a non-message signal to indicate the termination of this test session.
  rfWrite(terminalSignal);

  // Print out some statistics.
  Serial.print("Time used: ");
  Serial.print(String(timeUsed));
  Serial.print(" ms.\n");
  Serial.print("\nSent (bytes): ");
  unsigned long totalBytes = numOfPackets* bufferSize;
  Serial.print(String(totalBytes));
  Serial.print("\n");
  float bps = float(totalBytes)* 8.0/(float(timeUsed)/1000000.0);
  Serial.print(String(bps));
  Serial.print("\n\n\n");

  // turn off the transmission.
  started = false; 
}

// This marks this board as the receiving end of the transmission and send a startSignal over RF.
void TrySendStartSignal(){
  char serialInput = Serial.read();
  if (serialInput != startSignal) {
    // The serial input is not a start signal, ignore.
    return;
  }
  // Set board type to receiver.
  type = Receiver;
  
  // Print serial input.
  Serial.print("You typed: ");
  Serial.print(serialInput);
  Serial.print("\n");

  // Send a starting signal to the other board.
  rfWrite(serialInput); 

  // Mark the start of a test session on the receiver side.
  started = true;

  // Set the start time of the transmission from the reciving end to control waiting time in case of a packet drop.
  timeOutStartTime = micros();
}

// This function tries to read a start signal, and marks this board as the sending end of the transmission if a startSignal is found.
void TryReceiveStartSignal(){
  char readInput = rfRead();
  if (readInput != startSignal) {
    // The RF signal is not startSignal, ignore.
    return;
  }

  // Set board type to sender.
  type = Sender;

  // Print received information
  Serial.print("Recieved Start Signal: ");
  Serial.print(readInput);
  Serial.print("\n");

  // Mark the start of a test session on the sender side.
  started = true;
}

// Print the counter, mark the end of the test session and reset counter and timerout timer.
void MarkTermination() {
  Serial.print("Time from start:");
  Serial.print(String(micros()-timeOutStartTime));
  Serial.print("\n");
  Serial.print("Number of packets:");
  Serial.print(String(counter));
  Serial.print("\n\n\n");      
  // Mark the end of this test.
  started = false; 
  // Reset the counter.
  counter = 0; 
  timeOutStartTime = 0;
}

