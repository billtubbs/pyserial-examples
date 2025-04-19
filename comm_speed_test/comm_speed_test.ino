/* Arduino script for handling serial communication with another device
 * running a Python script (see comm_speed_test.py).
 *
 * A start marker 254 and an end marker of 255 are used to denote the 
 * beginning and end of a chunk of data. 253 is declared a special byte
 * to be able to reproduce 253, 254 and 255 value.
 * The first two bytes after the start marker indicate the length of
 * the data in bytes. If the number of bytes is 0 the recipient will 
 * assume the data is a debug string which may be printed or logged.
 * It also sends data to the PC using the same system.
 */

#define MY_NAME "Teensy4"
#define HOST_NAME "Rpi02"
#define START_MARKER 254
#define END_MARKER 255
#define SPECIAL_BYTE 253
#define MAX_MSG_LEN 8192

// TODO: consider making some of these locals?
uint16_t bytesRecvd = 0;
uint16_t bytesSent = 0; // number of bytes in the package
uint16_t dataRecvCount = 0;

byte dataRecvd[MAX_MSG_LEN];
byte dataSend[MAX_MSG_LEN];
byte tempBuffer[MAX_MSG_LEN];

uint16_t dataSendCount = 0; // number of data bytes to send to the PC
uint16_t dataTotalSend = 0; // number of actual bytes to send to PC including encoded bytes

boolean inProgress = false;
boolean startFound = false;
boolean allReceived = false;
boolean connEstablished = false;

void setup() {
  pinMode(13, OUTPUT); // onboard LED
  delay(500);
  Serial.begin(57600);
 
  // Wait for serial port to connect. Needed for native USB port only
  while (!Serial) {
    delay(0);
  }

  debugToPC(strcat("My name is ", MY_NAME));
}

void loop() {

  getSerialData();

  processData();

}

void getSerialData() {
  /* Receives data into tempBuffer[]
   *   saves the number of bytes that the PC said it sent - which will be in tempBuffer[1]
   *   uses decodeHighBytes() to copy data from tempBuffer to dataRecvd[]
   *
   * the Arduino program will use the data it finds in dataRecvd[]
   */
  char msg_buffer[50];

  if(Serial.available() > 0) {

    byte x = Serial.read();
    if (x == START_MARKER) { 
      bytesRecvd = 0;
      inProgress = true;
      // debugToPC("start received");
    }

    if(inProgress) {
      tempBuffer[bytesRecvd] = x;
      bytesRecvd ++;
    }

    if (x == END_MARKER) {
      inProgress = false;
      allReceived = true;

      decodeHighBytes();

      // Save the first two bytes which contain an integer value for 
      // the number of bytes sent.
      bytesSent = tempBuffer[1] * 256 + tempBuffer[2];
      sprintf(msg_buffer, "Received %d bytes.", bytesSent);
      debugToPC(msg_buffer);
    }
  }
}

void processData() {
  // processes the data that is in dataRecvd[]

  if (allReceived) {
  
    // For demonstration, copy dataRecvd to dataSend and send back to PC
    dataSendCount = dataRecvCount;
    for (byte n = 0; n < dataRecvCount; n++) {
       dataSend[n] = dataRecvd[n];
    }

    dataToPC();

    allReceived = false; 
  }
}

void decodeHighBytes() {
  /*  copies to dataRecvd[] only the data bytes i.e. excluding the marker bytes and the count byte
   *  and converts any bytes of 253 etc into the intended numbers
   *  Note that bytesRecvd is the total of all the bytes including the markers
   */
  dataRecvCount = 0;
  for (byte n = 3; n < bytesRecvd - 1 ; n++) {  // skips the start marker and the count bytes
    byte x = tempBuffer[n];
    if (x == SPECIAL_BYTE) {
       n++;
       x = x + tempBuffer[n];
    }
    dataRecvd[dataRecvCount] = x;
    dataRecvCount ++;
  }
}


void dataToPC() {
  /* expects to find data in dataSend[]
   * uses encodeHighBytes() to copy data to tempBuffer
   * sends data to PC from tempBuffer
   */
  encodeHighBytes();

  Serial.write(START_MARKER);
  Serial.write(highByte(dataSendCount));  // send integer as two bytes
  Serial.write(lowByte(dataSendCount));
  Serial.write(tempBuffer, dataTotalSend);
  Serial.write(END_MARKER);
}


void encodeHighBytes() {
  /* Copies to tempBuffer[] all of the data in dataSend[]
   * and converts any bytes of 253 or more into a pair of 
   * bytes, 253 0, 253 1 or 253 2 as appropriate.
   */
  dataTotalSend = 0;
  for (byte n = 0; n < dataSendCount; n++) {
    if (dataSend[n] >= SPECIAL_BYTE) {
      tempBuffer[dataTotalSend] = SPECIAL_BYTE;
      dataTotalSend++;
      tempBuffer[dataTotalSend] = dataSend[n] - SPECIAL_BYTE;
    }
    else {
      tempBuffer[dataTotalSend] = dataSend[n];
    }
    dataTotalSend++;
  }
}


void debugToPC(char arr[]) {
    byte nb = 0;
    Serial.write(START_MARKER);
    Serial.write(nb);
    Serial.write(nb);
    Serial.print(arr);
    Serial.write(END_MARKER);
}


void debugToPC(byte num) {
    byte nb = 0;
    Serial.write(START_MARKER);
    Serial.write(nb);
    Serial.print(num);
    Serial.write(END_MARKER);
}


void blinkLED(byte numBlinks) {
    for (byte n = 0; n < numBlinks; n ++) {
      digitalWrite(13, HIGH);
      delay(200);
      digitalWrite(13, LOW);
      delay(200);
    }
}
