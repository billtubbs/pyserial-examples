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
uint16_t numBytesExpected = 0; // number of bytes in the package

byte dataRecvd[MAX_PACKAGE_LEN];
byte dataSend[MAX_PACKAGE_LEN];
byte tempBuffer[MAX_PACKAGE_LEN * 2];

uint16_t dataRecvCount = 0;
uint16_t dataSendCount = 0; // number of data bytes to send to the PC
uint16_t dataTotalSend = 0; // number of actual bytes to send to PC including encoded bytes

boolean receivingInProgress = false;
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
  /* Receives data from serial connection into tempBuffer[] then calls
   * decodeHighBytes() to decode and copy data from tempBuffer[] to 
   * dataRecvd[] for use by the Arduino program.
   */

  if(Serial.available() > 0) {

    byte x = Serial.read();

    if (!receivingInProgress) {
      if (x == START_MARKER) {
        numBytesRecvd = 0;
        receivingInProgress = true;
      }
    }
    else {
      if (numBytesRecvd >= MAX_PACKAGE_LEN * 2) {
        receivingInProgress = false;
        snprintf(
          msg_buffer, 
          MSG_BUFFER_SIZE, 
          "getSerialData failed: number of bytes exceeds %d", 
          MAX_PACKAGE_LEN * 2
        );
        debugToPC(msg_buffer);
      }
      else{
        if (x != END_MARKER) {
          tempBuffer[numBytesRecvd] = x;
          numBytesRecvd ++;
        }
        else {
          receivingInProgress = false;

          // Decode the data from tempBuffer[] into dataRecvd[]
          decodeHighBytes();
          if (dataRecvCount >= MAX_PACKAGE_LEN) {
            snprintf(msg_buffer, MSG_BUFFER_SIZE, "Num. of data bytes exceeds buffer size %d", MAX_PACKAGE_LEN);
            debugToPC(msg_buffer);
          }

          // The first two bytes indicate the total number of bytes sent
          numBytesExpected = dataRecvd[0] * 256 + dataRecvd[1];
          snprintf(msg_buffer, MSG_BUFFER_SIZE, "Num. of data bytes expected: %d", numBytesExpected);
          debugToPC(msg_buffer);
          snprintf(msg_buffer, MSG_BUFFER_SIZE, "Total actual bytes received: %d.", numBytesRecvd);
          debugToPC(msg_buffer);
          snprintf(msg_buffer, MSG_BUFFER_SIZE, "Num. of data bytes received: %d.", dataRecvCount);
          debugToPC(msg_buffer);

          // Check expected number of data bytes received
          if ((dataRecvCount < numBytesExpected) || (dataRecvCount < numBytesExpected)) {
            snprintf(msg_buffer, MSG_BUFFER_SIZE, "Num. data bytes received does not match expected.");
            debugToPC(msg_buffer);
          }
          else {
            allReceived = true;
          }
        }
      }
    }
  }
}

void processData() {
  // processes the data that is in dataRecvd[]

  if (allReceived) {
    // Here is where you put your code to process the data received.
    // Instead, for this demonstration, simply copy dataRecvd to dataSend 
    // and send it back to the PC.
    dataSendCount = dataRecvCount;
    for (uint16_t n = 0; n < dataRecvCount; n++) {
       dataSend[n] = dataRecvd[n];
    }
    // highByte(dataSendCount);  // send integer as two bytes
    // lowByte(dataSendCount);
    dataToPC();
    allReceived = false; 
  }
}

void decodeHighBytes() {
  /*  copies the length data in the first two bytes and the data bytes to 
   *  dataRecvd[], converting any bytes following the special byte into 
   *  the intended values.
   *  Note that numBytesRecvd is the total of all the bytes including the
   *  special bytes and length data bytes.
   */
  dataRecvCount = 0;
  for (uint16_t n = 0; n < numBytesRecvd; n++) {
    byte x = tempBuffer[n];
    if (x == SPECIAL_BYTE) {
       n++;
       x = x + tempBuffer[n];
    }
    if (dataRecvCount >= MAX_PACKAGE_LEN) {
      break;
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
  Serial.write(tempBuffer, dataTotalSend);
  Serial.write(END_MARKER);
}


void encodeHighBytes() {
  /* Copies to tempBuffer[] all of the data in dataSend[]
   * and converts any bytes of 253 or more into a pair of 
   * bytes, 253 0, 253 1 or 253 2 as appropriate.
   */
  dataTotalSend = 0;
  for (uint16_t n = 0; n < dataSendCount; n++) {
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
    Serial.write(nb);
    Serial.print(num);
    Serial.write(END_MARKER);
}
