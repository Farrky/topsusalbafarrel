#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <DHT.h>
#include <Fuzzy.h>

// **Konfigurasi LCD I2C untuk ESP32**
#define SDA_PIN 21
#define SCL_PIN 22
LiquidCrystal_I2C lcd(0x27, 16, 2);

// **Konfigurasi DHT22**
#define DHTPIN 12  
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);
float h, t;

// **Konfigurasi Relay & Buzzer**
#define RELAY_PIN 13
#define BUZZER_PIN 4

// **Inisialisasi Fuzzy Logic**
Fuzzy *fuzzy = new Fuzzy();

// **Variabel untuk Buzzer**
unsigned long lastBuzzTime = 0;
bool buzzerState = false;

void setupFuzzy() {
    // **Input: Suhu**
    FuzzySet *dingin = new FuzzySet(0, 0, 20, 25);
    FuzzySet *normal = new FuzzySet(20, 25, 30, 35);
    FuzzySet *panas = new FuzzySet(30, 35, 50, 50);
    FuzzyInput *suhu = new FuzzyInput(1);
    suhu->addFuzzySet(dingin);
    suhu->addFuzzySet(normal);
    suhu->addFuzzySet(panas);
    fuzzy->addFuzzyInput(suhu);

    // **Input: Kelembaban**
    FuzzySet *kering = new FuzzySet(0, 0, 40, 50);
    FuzzySet *normalK = new FuzzySet(40, 50, 60, 70);
    FuzzySet *lembab = new FuzzySet(60, 70, 100, 100);
    FuzzyInput *kelembaban = new FuzzyInput(2);
    kelembaban->addFuzzySet(kering);
    kelembaban->addFuzzySet(normalK);
    kelembaban->addFuzzySet(lembab);
    fuzzy->addFuzzyInput(kelembaban);

    // **Output: Kondisi**
    FuzzySet *nyaman = new FuzzySet(0, 0, 50, 60);
    FuzzySet *tidakNyaman = new FuzzySet(50, 60, 100, 100);
    FuzzyOutput *kondisi = new FuzzyOutput(3);
    kondisi->addFuzzySet(nyaman);
    kondisi->addFuzzySet(tidakNyaman);
    fuzzy->addFuzzyOutput(kondisi);

    // **Aturan fuzzy**
    FuzzyRuleAntecedent *rule1 = new FuzzyRuleAntecedent();
    rule1->joinWithAND(dingin, kering);
    FuzzyRuleConsequent *then1 = new FuzzyRuleConsequent();
    then1->addOutput(nyaman);
    fuzzy->addFuzzyRule(new FuzzyRule(1, rule1, then1));

    FuzzyRuleAntecedent *rule2 = new FuzzyRuleAntecedent();
    rule2->joinWithAND(panas, lembab);
    FuzzyRuleConsequent *then2 = new FuzzyRuleConsequent();
    then2->addOutput(tidakNyaman);
    fuzzy->addFuzzyRule(new FuzzyRule(2, rule2, then2));
}

void setup() {
    Serial.begin(115200);
    Serial.println("Hello Buzz!");

    // **Inisialisasi I2C LCD**
    Wire.begin(SDA_PIN, SCL_PIN);
    lcd.init();
    lcd.backlight();
    
    // **Konfigurasi Pin**
    pinMode(RELAY_PIN, OUTPUT);
    pinMode(BUZZER_PIN, OUTPUT);
    digitalWrite(RELAY_PIN, HIGH);  // Matikan relay awalnya
    digitalWrite(BUZZER_PIN, LOW);  // Matikan buzzer awalnya

    dht.begin();
    delay(2000); // Tunggu sensor DHT siap
    setupFuzzy();
}

void loop() {
    h = dht.readHumidity();
    t = dht.readTemperature();

    // **Cek jika sensor error**
    if (isnan(h) || isnan(t)) {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Sensor Error!");
        Serial.println("Sensor Error!");
        delay(2000);
        return;
    }
    
    // **Masukkan data ke Fuzzy Logic**
    fuzzy->setInput(1, t);
    fuzzy->setInput(2, h);
    fuzzy->fuzzify();
    int kondisi = fuzzy->defuzzify(3);
    
    // **Tampilkan di LCD**
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Suhu: ");
    lcd.print(t);
    lcd.print(" C");
    
    lcd.setCursor(0, 1);
    lcd.print("Status: ");
    
    // **Cek kondisi dan kendalikan relay & buzzer**
    if (kondisi < 50) {
        lcd.print("Nyaman");
        digitalWrite(RELAY_PIN, HIGH);   // Matikan relay (kipas OFF)
        digitalWrite(BUZZER_PIN, LOW);   // Matikan buzzer
    } else {
        lcd.print("Tdk Nymn");
        digitalWrite(RELAY_PIN, LOW);    // Nyalakan relay (kipas ON)

        // **Buzzer ON/OFF setiap 1 detik (seperti contoh awal)**
        if (millis() - lastBuzzTime >= 1000) {
            lastBuzzTime = millis();
            buzzerState = !buzzerState;  // Toggle buzzer
            digitalWrite(BUZZER_PIN, buzzerState);
        }
    }

    // **Tampilkan di Serial Monitor**
    Serial.print("Suhu: "); Serial.print(t);
    Serial.print(" C, Kelembaban: "); Serial.print(h);
    Serial.print(" %, Status: ");
    Serial.println(kondisi < 50 ? "Nyaman" : "Tidak Nymn");

    delay(500); // Delay pendek agar program tetap responsif
}
