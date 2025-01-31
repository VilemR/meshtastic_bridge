#include "Helper.cpp"

#define HELTEC_POWER_BUTTON // must be before "#include <heltec_unofficial.h>"

#include <heltec_unofficial.h>

#define HELTEC_WIRELESS_STICK

float LORA_FREQUENCY = 869.525; // 869.525 433.062

#define MAX_NODES_ENTRIES 20  // Maximum number of entries in the map
#define NODE_ADDRESS_LENGTH 8 // Length of a MAC address string

boolean isRepeater = true; // ale by default je sender - Setup().changeMode()
byte syncWord = 0x2B;      // 0x2B

// Adresa se v packetu posila nejnizsi byte jako prvni!
// Primarne se adresy udrzuji a zapisuji prvni nejvyssi byte a v kodu se to prohazuje

String SENDER = "aaaaaa1a"; // Reverse order!!!!
String REPEATER = "bbbbbb1b";
String MESSAGEID = "cccccc1c";

String addressReceipient = "";
String addressSender = "";

struct LoraRawPacket
{
    String sender;
    String receipient;
    String messageId;
    byte flags;
    byte channel;
    float snr;
    int rssi;
};

int cycle = 0;
long lastSendTime = 0; // last send time
int interval = 10000;  // interval between sends
int interval_rnd = 3000;

int transmissionState = RADIOLIB_ERR_NONE;
bool transmitFlag = false;
volatile bool operationDone = false;
volatile bool packetReceived = false;
ICACHE_RAM_ATTR

void setFlagOperationDone(void)
{
    operationDone = true;
}

void setFlagPacketReceived(void)
{
    packetReceived = true;
}

String getByteString(byte byteArray[], int len)
{
    String hexString = "";
    for (int i = 0; i < len; i++)
    {
        if (byteArray[i] < 0x10)
            hexString += '0';
        hexString += String(byteArray[i], HEX);
    }
    return hexString;
}

String printRawPacket(byte *packet, int len)
{
    String hexString = "\nbyte loraPacket[] = {";
    for (int i = 0; i < len; i++)
    {
        hexString = hexString + "0x" + String(packet[i], HEX) + ", ";
    }
    return hexString.substring(0, hexString.length() - 2) + "};";
}

String convertToPrintableString(byte *packet, int from, int to)
{
    String result = "";
    for (int i = from; i <= to; i++)
    {
        char c = (char)0x20;
        if (((byte)packet[i] < 0x21) or ((byte)packet[i] > 0x7e))
        {
            c = (char)0x20;
        }
        else
        {
            c = (char)packet[i];
        }

        result += c;
    }
    return result;
}

void packetOverride(byte *loraPacket, String hexString, int fromByte, int toByte)
{

    int hexStringLength = hexString.length() / 2; // Number of bytes in HEX string
    if ((toByte - fromByte + 1) != hexStringLength)
    {
        Serial.println("Error: HEX string length does not match the specified range!");
        return;
    }
    for (int i = 0; i < hexStringLength; i++)
    {
        String byteHex = hexString.substring(i * 2, i * 2 + 2);                 // Extract 2 hex characters
        loraPacket[toByte - i] = (uint8_t)strtol(byteHex.c_str(), nullptr, 16); // Convert HEX to byte
    }
}

void packetOverrides(byte *loraPacket)
{
    packetOverride(loraPacket, addressReceipient, 0, 3);
    packetOverride(loraPacket, addressSender, 4, 7);
    // packetOverride(loraPacket, MESSAGEID, 8, 11);
}

void setValue(byte payload[], byte destArr[], int from, int to, boolean reverse)
{
    for (int i = from; i <= to; i++)
    {
        destArr[i - from] = payload[i];
    }
    if (reverse == true)
    {
        int len = to - from + 1;
        for (int i = 0; i < len / 2; i++)
        {
            byte temp = destArr[i];
            destArr[i] = destArr[len - i - 1];
            destArr[len - i - 1] = temp;
        }
    }
}

void showMode()
{
    display.clear();
    if (isRepeater == true)
    {
        heltec_led(100);
        display.println("Repeater");
        delay(2000);
        heltec_led(0);
    }
    else
    {
        heltec_led(100);
        display.println("Sender");
        delay(2000);
        heltec_led(0);
    }
}

void changeMode()
{
    if (isRepeater == true)
    {
        isRepeater = false;
        addressReceipient = REPEATER;
        addressSender = SENDER;
        Serial.println(F("Changed to SENDER mode!"));
    }
    else
    {
        isRepeater = true;
        addressReceipient = SENDER;
        addressSender = REPEATER;
        Serial.println(F("Changed to REPEATER mode!"));
    }
    showMode();
}

// #define MAX_NODES_ENTRIES   20 // Maximum number of entries in the map
// #define NODE_ADDRESS_LENGTH  8 // Length of a MAC address string

char nodeAddresses[MAX_NODES_ENTRIES][NODE_ADDRESS_LENGTH];
char names[MAX_NODES_ENTRIES][20]; // Adjust name length as needed
int mapSize = 0;                   // Current number of entries in the map

void addNode(String mac, String name)
{
    if (mapSize >= MAX_NODES_ENTRIES)
    {
        Serial.println("Map is full!");
        return;
    }
    strncpy(nodeAddresses[mapSize], mac.c_str(), NODE_ADDRESS_LENGTH);
    strncpy(names[mapSize], name.c_str(), 20);
    mapSize++;
}

String getNodeName(String mac)
{
    char subArray[9];
    for (int i = 0; i < mapSize; i++)
    {
        strncpy(subArray, nodeAddresses[i], 8);
        subArray[8] = '\0';
        if (strcmp(subArray, mac.c_str()) == 0)
        {
            return String(names[i]);
        }
    }
    return "?"; // Return null if not found
}

boolean isKnown(String mac)
{
    return strcmp(String("?").c_str(), mac.c_str()) == 0 ? true : false;
}

int initLoRaRadio(lora_config loraRequiredConfig)
{
/*
    int state = radio.begin(loraRequiredConfig.LORA_FREQUENCY,
                            loraRequiredConfig.bandwidth,
                            loraRequiredConfig.spreading_factor,
                            loraRequiredConfig.coding_rate,
                            loraRequiredConfig.syncWord,
                            loraRequiredConfig.power,
                            loraRequiredConfig.preambleLength,
                            loraRequiredConfig.tcxoVoltage,
                            false);
float LORA_FREQUENCY = 869.525; // 869.525 433.062
int state = radio.begin(LORA_FREQUENCY, // freq 869.525
                          250.0,          // bw
                          11,             // sf
                          5,              // cr
                          syncWord,       // syncWord
                          21,             // power
                          16,             // preambleLength
                          1.6,            // tcxoVoltage
                          false);      
*/

Serial.print(loraRequiredConfig.spreading_factor);
          int state = radio.begin(loraRequiredConfig.LORA_FREQUENCY,
                            loraRequiredConfig.bandwidth,
                            loraRequiredConfig.spreading_factor,
                            loraRequiredConfig.coding_rate,
                            loraRequiredConfig.syncWord,
                            loraRequiredConfig.power,
                            loraRequiredConfig.preambleLength,
                            loraRequiredConfig.tcxoVoltage,
                            false);                

    if (state == RADIOLIB_ERR_NONE)
    {
        display.println("LoRa OK");
        radio.setDio1Action(setFlagOperationDone);
        radio.setPacketReceivedAction(setFlagPacketReceived);
        packetReceived = false;
        state = radio.startReceive();
        if (state == RADIOLIB_ERR_NONE)
        {
            display.println("Recv OK");
        }
        else
        {
            Serial.print(F("Recv ERR"));
            Serial.println(state);
            display.printf("ERR: %i\n", state);
            while (true)
            {
                delay(10);
            }
        }
    }
    else
    {
        display.printf("LoRa Err: %i\n", state);
        while (true)
        {
            delay(10);
        }
    }
    return state;
}

lora_config LORA_LF869525;
lora_config LORA_LF869775;
lora_config LORA_MF869525;
lora_config LORA_MF869775;

lora_config LORA_FROM;
lora_config LORA_TO;

void setup()
{
    heltec_setup();

    LORA_MF869525.LORA_FREQUENCY = 869.525;
    LORA_MF869775.LORA_FREQUENCY = 869.775;
    LORA_MF869525.spreading_factor = 9;
    LORA_MF869775.spreading_factor = 9;

    LORA_FROM = LORA_LF869525;
    LORA_TO = LORA_MF869775;

    Serial.println("Serial() - connection established...");

    initLoRaRadio(LORA_FROM);

    float vbat = heltec_vbat();
    display.printf("%.2fV (%d%%)\n", vbat, heltec_battery_percent(vbat));
}

void sendPacket(byte byteArray[], int len)
{
    transmissionState = radio.startTransmit(byteArray, len, syncWord);
    lastSendTime = millis(); // timestamp the message
    heltec_led(0);
    cycle = cycle + 1;
    delay(500);
    int cyc = 100;
    while (transmissionState != RADIOLIB_ERR_NONE and cyc != 0)
    {
        delay(100);
        cyc = cyc - 1;
    }
    if (transmissionState != RADIOLIB_ERR_NONE)
    {
        Serial.println(F("Sent but looped into error!"));
        display.println("SendErr");
        while (1)
        {
            delay(100);
        }
    }
    else
    {
        Serial.println(F("Packet sent OK!"));
    }
    packetReceived = false;
}

void bridge(byte byteArray[], int len)
{
    initLoRaRadio(LORA_TO);

    sendPacket(byteArray, len);

    initLoRaRadio(LORA_FROM);
    radio.startReceive();
}

void loop()
{

    heltec_loop();

    if (packetReceived == true)
    {
        packetReceived = false;
        byte *byteArr = new byte[255];
        int numBytes = radio.getPacketLength();
        int state = radio.readData(byteArr, numBytes);
        if (state == RADIOLIB_ERR_NONE)
        {
            Serial.println(F("\n===============================================================\n[SX1262] Received packet:"));
            Serial.println(printRawPacket(byteArr, numBytes));
            Serial.println("String loraPacketHex = \"" + getByteString(byteArr, numBytes) + "\"");
            byte *rec = new byte[4];
            byte *sen = new byte[4];
            byte *mid = new byte[4];
            byte *flags = new byte[1];
            byte *channel = new byte[1];
            byte *payload = new byte[255];
            setValue(byteArr, rec, 0, 3, true);
            setValue(byteArr, sen, 4, 7, true);
            setValue(byteArr, mid, 8, 11, true);
            setValue(byteArr, flags, 12, 12, false);
            setValue(byteArr, channel, 13, 13, false);
            setValue(byteArr, payload, 16, numBytes - 1, false);

            String payloadStr = convertToPrintableString(payload, 0, numBytes - 17);

            Serial.println("   Receipient: " + getByteString(rec, 4) + " (" + getNodeName(getByteString(rec, 4)) + ")");
            Serial.println("   Sender    : " + getByteString(sen, 4) + " (" + getNodeName(getByteString(sen, 4)) + ")");
            Serial.println("    packet id: " + getByteString(mid, 4));
            Serial.println("        flags: " + getByteString(flags, 1));
            Serial.println("      channel: " + getByteString(channel, 1));
            Serial.println("      payload: " + getByteString(payload, numBytes - 16) + " len(" + String(numBytes - 16) + ")");

            if (getByteString(sen, 4) == REPEATER || getByteString(sen, 4) == SENDER || true)
            {
                Serial.println("        str(): " + payloadStr);
            }

            Serial.println("         RSSI: " + String(radio.getRSSI()) + " [dBm]");
            Serial.println("          SNR: " + String(radio.getSNR()) + " [dB]");
            display.clear();
            display.println("" + getByteString(sen, 4) + " -> " + getByteString(rec, 4));
            display.println("ID:" + getByteString(mid, 4) + " (" + getByteString(channel, 1) + ")");
            display.println("RSSI: " + String(radio.getRSSI()) + " [dBm]");
            display.println("SNR : " + String(radio.getSNR()) + " [dB]");

            Serial.println(" compare SEN : " + getByteString(sen, 4) + " <> " + SENDER);

            bridge(byteArr, numBytes);
        }
    }
}
