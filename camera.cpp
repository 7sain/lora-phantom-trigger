#include <Arduino.h>
#include <RadioLib.h>

// --- HARDWARE PINS ---
#define CONN_LED 33
#define PC817_PIN 4
#define LOW_BAT_LED 42 // New Red LED pin

// --- BATTERY PINS (Heltec V4) ---
#define VBAT_PIN 1     // Internal battery voltage reading pin
#define VBAT_CTRL 37   // Controls the internal voltage divider

// --- LORA PINS ---
#define LORA_NSS 8
#define LORA_DIO1 14
#define LORA_NRST 12
#define LORA_BUSY 13
#define LORA_PA_PWR 7   
#define LORA_PA_CSD 2   
#define LORA_PA_CTX 5   

SX1262 radio = new Module(LORA_NSS, LORA_DIO1, LORA_NRST, LORA_BUSY);

// --- TIMERS ---
unsigned long lastRxTime = 0;
unsigned long pc817OffTime = 0;
unsigned long lastBatCheckTime = 0; // Timer for battery checks
bool isTriggered = false;

void enableTransmit() {
    digitalWrite(LORA_PA_CSD, HIGH); 
    digitalWrite(LORA_PA_CTX, HIGH); 
}

void enableReceive() {
    digitalWrite(LORA_PA_CSD, HIGH); 
    digitalWrite(LORA_PA_CTX, LOW);  
    radio.startReceive();           
}

void transmitPacket(String data) {
    enableTransmit();
    radio.transmit(data);
    enableReceive();
}

void setup() {
    // Serial.begin(115200);
    pinMode(CONN_LED, OUTPUT);

    // Low Battery LED Setup
    pinMode(LOW_BAT_LED, OUTPUT);
    digitalWrite(LOW_BAT_LED, LOW);
    
    // Heltec Battery Reader Setup
    pinMode(VBAT_CTRL, OUTPUT);
    digitalWrite(VBAT_CTRL, HIGH); // Pull LOW to enable the internal battery circuit

    pinMode(PC817_PIN, OUTPUT);
    digitalWrite(PC817_PIN, LOW); // Ensure camera isn't triggered on boot

    pinMode(LORA_DIO1, INPUT); 

    pinMode(LORA_PA_PWR, OUTPUT);
    pinMode(LORA_PA_CSD, OUTPUT);
    pinMode(LORA_PA_CTX, OUTPUT);
    digitalWrite(LORA_PA_PWR, HIGH); 

    // Max Range Config: 868MHz, SF12, CR8, 22dBm
    // int state = radio.begin(868.0, 125.0, 12, 8, 0x12, 22);
    int state = radio.begin(915.0, 500.0, 7, 5, 0x12, 22);

    if (state != RADIOLIB_ERR_NONE) while (true); 

    radio.setDio2AsRfSwitch(true); 
    enableReceive();
}

void loop() {
    unsigned long currentMillis = millis();

    // 1. CONNECTION LED: Off if no signal for 25 seconds
    if (currentMillis - lastRxTime > 25000) {
        digitalWrite(CONN_LED, LOW);
    } else {
        digitalWrite(CONN_LED, HIGH);
    }

    // 2. PC817 TRIGGER SHUTOFF: Hold trigger for 200ms then release
    if (isTriggered && currentMillis > pc817OffTime) {
        digitalWrite(PC817_PIN, LOW);
        isTriggered = false;
    }

    // 3. LOW BATTERY CHECK (Runs once every 10 seconds)
    if (currentMillis - lastBatCheckTime > 10000) {
        lastBatCheckTime = currentMillis;
        
        int bat_val = analogRead(VBAT_PIN);

        // Serial.print("Raw Battery ADC Value: ");
        // Serial.println(bat_val);
        
        // A raw ADC value of ~2100 roughly equals 3.3V (a nearly dead LiPo battery).
        // The bat_val > 500 check prevents the LED from turning on if running on USB with no battery attached.
        if (bat_val > 500 && bat_val < 835) {
            digitalWrite(LOW_BAT_LED, HIGH); // Battery is low, turn LED ON
        } else {
            digitalWrite(LOW_BAT_LED, LOW);  // Battery is fine, turn LED OFF
        }
    }
    
    // 4. RECEIVER LOGIC
    if (digitalRead(LORA_DIO1) == HIGH) {
        String str;
        int state = radio.readData(str);
        
        if (state == RADIOLIB_ERR_NONE) {
            lastRxTime = currentMillis; // Valid packet received, update timer
            
            if (str == "T") {
                // Trigger Command Received!
                digitalWrite(PC817_PIN, HIGH);
                isTriggered = true;
                pc817OffTime = currentMillis + 200; // Release PC817 after 200ms
                
                transmitPacket("K"); // Send Trigger ACK back to remote
            } 
            else if (str == "H") {
                // Heartbeat Received. Just acknowledge it to keep Remote's LED on.
                transmitPacket("A"); 
            }
        }
        enableReceive();
    }
}