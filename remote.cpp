#include <Arduino.h>
#include <RadioLib.h>

// --- HARDWARE PINS ---
#define CONN_LED 33
#define TRIG_LED 34
#define PUSH_BTN 4
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
unsigned long lastTxTime = 0;
unsigned long lastRxTime = 0;
unsigned long flashEndTime = 0;
unsigned long lastDebounceTime = 0;
unsigned long lastBatCheckTime = 0; // Timer for battery checks
bool buttonState = HIGH;
bool lastButtonState = HIGH;

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
    lastTxTime = millis();
}

void setup() {
    
    // Serial.begin(115200);
    pinMode(CONN_LED, OUTPUT);
    pinMode(TRIG_LED, OUTPUT);
    pinMode(PUSH_BTN, INPUT_PULLUP);

    // Low Battery LED Setup
    pinMode(LOW_BAT_LED, OUTPUT);
    digitalWrite(LOW_BAT_LED, LOW);

    // Heltec Battery Reader Setup
    pinMode(VBAT_CTRL, OUTPUT);
    digitalWrite(VBAT_CTRL, HIGH); // Pull LOW to enable the internal battery circuit

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

    // 1. HEARTBEAT: Send "H" every 10 seconds to keep connection alive
    if (currentMillis - lastTxTime > 10000) {
        transmitPacket("H"); 
    }

    // 2. CONNECTION LED: Off if no signal for 25 seconds
    if (currentMillis - lastRxTime > 25000) {
        digitalWrite(CONN_LED, LOW);
    } else {
        digitalWrite(CONN_LED, HIGH);
    }

    // 3. LOW BATTERY CHECK (Runs once every 10 seconds)
    if (currentMillis - lastBatCheckTime > 10000) {
        lastBatCheckTime = currentMillis;
        
        int bat_val = analogRead(VBAT_PIN);
         
        // Muted for field deployment!
        // Serial.print("Remote Raw Battery ADC Value: ");
        // Serial.println(bat_val);
        
        // Alarm triggers if voltage drops below ~3.4V (ADC 860)
        if (bat_val > 500 && bat_val < 860) {
            digitalWrite(LOW_BAT_LED, HIGH); // Battery is low, turn LED ON
        } else {
            digitalWrite(LOW_BAT_LED, LOW);  // Battery is fine, turn LED OFF
        }
    }

    // 4. PUSH BUTTON: Debounce and trigger
    int reading = digitalRead(PUSH_BTN);
    if (reading != lastButtonState) lastDebounceTime = currentMillis;
    
    if ((currentMillis - lastDebounceTime) > 50) {
        if (reading != buttonState) {
            buttonState = reading;
            if (buttonState == LOW) { // Button Pressed
                transmitPacket("T"); // Send "T" for Trigger
            }
        }
    }
    lastButtonState = reading;

    // 5. TRIGGER LED FLASHING: Flash for 10 seconds after ACK
    if (currentMillis < flashEndTime) {
        // Toggle LED every 200ms
        if ((currentMillis / 200) % 2 == 0) digitalWrite(TRIG_LED, HIGH);
        else digitalWrite(TRIG_LED, LOW);
    } else {
        digitalWrite(TRIG_LED, LOW);
    }

    // 6. RECEIVER LOGIC
    if (digitalRead(LORA_DIO1) == HIGH) {
        String str;
        int state = radio.readData(str);
        if (state == RADIOLIB_ERR_NONE) {
            lastRxTime = currentMillis; // Update connection timer
            
            if (str == "K") { 
                // "K" means Trigger ACK was received! Start 10-second flash.
                flashEndTime = currentMillis + 10000; 
            }
        } 
        enableReceive();
    }
}