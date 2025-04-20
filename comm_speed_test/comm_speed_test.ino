/* Arduino script for handling serial communication with another device
 * running a Python script (see comm_speed_test.py).
 *
 * A start marker of value 254 and an end marker of 255 are used to 
 * denote the beginning and end of a chunk of data. 253 is declared a 
 * special byte to be able to reproduce 253, 254 and 255 values.
 * The first two bytes after the start marker indicate the length of
 * the data in bytes. If the number of bytes is 0 the data is
 * considered a 'debug' message string.
 * In this demonstration script, a function called processData simply
 * sends the received data back to the host to verify the communication
 * process and test the round-trip transmission speed. 
 */

#define MY_NAME "Teensy4"
#define HOST_NAME "Rpi02"
#define START_MARKER 254
#define END_MARKER 255
#define SPECIAL_BYTE 253
#define MAX_PACKAGE_LEN 8192
#define MSG_BUFFER_SIZE 100

// TODO: consider making some of these locals?
uint16_t numBytesRecvd = 0;
uint16_t numBytesSent = 0; // number of bytes in the package

byte dataRecvd[MAX_PACKAGE_LEN];
byte dataSend[MAX_PACKAGE_LEN];
byte tempBuffer[MAX_PACKAGE_LEN];

uint16_t dataRecvCount = 0;
uint16_t dataSendCount = 0; // number of data bytes to send to the PC
uint16_t dataTotalSend = 0; // number of actual bytes to send to PC including encoded bytes

boolean inProgress = false;
boolean allReceived = false;
boolean connEstablished = false;

char msg_buffer[MSG_BUFFER_SIZE];

void setup() {

  pinMode(LED_BUILTIN, OUTPUT); // onboard LED

  delay(500);
  Serial.begin(57600);

  // The board LED will flash until a connection is established.
  digitalWrite(LED_BUILTIN, HIGH);

}

void loop() {
  if (Serial) {
    if (!connEstablished) {
      connEstablished = true;
      newConnection();
    }
    getSerialData();
    processData();
  }
  else {
    connEstablished = false;
    flashBoardLed();
  }
}

void newConnection() {
  snprintf(msg_buffer, MSG_BUFFER_SIZE, "My name is %s", MY_NAME);
  debugToPC(msg_buffer);
  digitalWrite(LED_BUILTIN, LOW);
}

void flashBoardLed() {
  if ((millis() % 1000) > 100) {
    digitalWrite(LED_BUILTIN, LOW);
  }
  else {
    digitalWrite(LED_BUILTIN, HIGH);
  }
}


void getSerialData() {
  /* Receives data into tempBuffer[]
   *   saves the number of bytes that the PC said it sent - which will be in tempBuffer[1]
   *   uses decodeHighBytes() to copy data from tempBuffer to dataRecvd[]
   *
   * the Arduino program will use the data it finds in dataRecvd[]
   */

  if(Serial.available() > 0) {

    byte x = Serial.read();
    if (x == START_MARKER) {
      numBytesRecvd = 0;
      inProgress = true;
    }

    if(inProgress) {
      tempBuffer[numBytesRecvd] = x;
      numBytesRecvd ++;
    }

    if (x == END_MARKER) {
      inProgress = false;
      allReceived = true;

      // Save the first two bytes which contain an integer value for 
      // the number of bytes sent.
      numBytesSent = tempBuffer[1] * 256 + tempBuffer[2];
      snprintf(msg_buffer, MSG_BUFFER_SIZE, "Num data bytes indicated: %d", numBytesSent);
      debugToPC(msg_buffer);
      snprintf(msg_buffer, MSG_BUFFER_SIZE, "Total actual bytes received: %d.", numBytesRecvd);
      debugToPC(msg_buffer);

      decodeHighBytes();
      snprintf(msg_buffer, MSG_BUFFER_SIZE, "Data bytes actually received: %d.", dataRecvCount);
      debugToPC(msg_buffer);

      // Check expected number of data bytes received
      if (dataRecvCount < numBytesSent) {
        snprintf(msg_buffer, MSG_BUFFER_SIZE, "%d less bytes of data received than expected.", numBytesSent - dataRecvCount);
        debugToPC(msg_buffer);
      }
      else if (dataRecvCount > numBytesSent) {
        snprintf(msg_buffer, MSG_BUFFER_SIZE, "%d more bytes of data received than expected.", dataRecvCount - numBytesSent);
        debugToPC(msg_buffer);
      }
    }
  }
}

void processData() {
  // processes the data that is in dataRecvd[]

  if (allReceived) {

    // For demonstration, copy dataRecvd to dataSend and send back to PC
    dataSendCount = dataRecvCount;
    for (uint16_t n = 0; n < dataRecvCount; n++) {
       dataSend[n] = dataRecvd[n];
    }

    dataToPC();

    allReceived = false; 
  }
}

void decodeHighBytes() {
  /*  copies the length data and the main data bytes to dataRecvd[], excluding 
   *  the start and end marker bytes and converts any bytes of 253 etc into 
   *  the intended values.
   *  Note that numBytesRecvd is the total of all the bytes including the markers
   */
  dataRecvCount = 0;
  // Skip the start and end markers
  for (uint16_t n = 1; n < numBytesRecvd - 1 ; n++) {
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
  for (uint16_t n = 2; n < dataSendCount; n++) {
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
