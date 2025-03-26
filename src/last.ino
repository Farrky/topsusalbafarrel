// **Konfigurasi WiFi & Blynk**
#define BLYNK_TEMPLATE_ID "TMPL6lwbd1Gcv"
#define BLYNK_TEMPLATE_NAME "temperature and humidity sensor alba farrel"
#define BLYNK_AUTH_TOKEN "r3dX1_WFslJaVydLiGvLdc73D8I291lK"
#include <WiFi.h>
#include <BlynkSimpleEsp32.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <DHT.h>
#include <Fuzzy.h>



char ssid[] = "Wokwi-GUEST";  
char pass[] = "";  

// **Konfigurasi LCD I2C**
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

// **Fuzzy Logic**
Fuzzy *fuzzy = new Fuzzy();

// **Variabel untuk Buzzer**
unsigned long lastBuzzTime = 0;
bool buzzerState = false;

// **Blynk Virtual Pins**
#define VIRTUAL_PIN_TEMP V0
#define VIRTUAL_PIN_HUMI V1
#define VIRTUAL_PIN_STATUS V2

void setupFuzzy() {
    FuzzySet *dingin = new FuzzySet(0, 0, 20, 25);
    FuzzySet *normal = new FuzzySet(20, 25, 30, 35);
    FuzzySet *panas = new FuzzySet(30, 35, 50, 50);
    FuzzyInput *suhu = new FuzzyInput(1);
    suhu->addFuzzySet(dingin);
    suhu->addFuzzySet(normal);
    suhu->addFuzzySet(panas);
    fuzzy->addFuzzyInput(suhu);

    FuzzySet *kering = new FuzzySet(0, 0, 40, 50);
    FuzzySet *normalK = new FuzzySet(40, 50, 60, 70);
    FuzzySet *lembab = new FuzzySet(60, 70, 100, 100);
    FuzzyInput *kelembaban = new FuzzyInput(2);
    kelembaban->addFuzzySet(kering);
    kelembaban->addFuzzySet(normalK);
    kelembaban->addFuzzySet(lembab);
    fuzzy->addFuzzyInput(kelembaban);

    FuzzySet *nyaman = new FuzzySet(0, 0, 50, 60);
    FuzzySet *tidakNyaman = new FuzzySet(50, 60, 100, 100);
    FuzzyOutput *kondisi = new FuzzyOutput(3);
    kondisi->addFuzzySet(nyaman);
    kondisi->addFuzzySet(tidakNyaman);
    fuzzy->addFuzzyOutput(kondisi);

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
    Wire.begin(SDA_PIN, SCL_PIN);
    lcd.init();
    lcd.backlight();

    pinMode(RELAY_PIN, OUTPUT);
    pinMode(BUZZER_PIN, OUTPUT);
    digitalWrite(RELAY_PIN, HIGH);
    digitalWrite(BUZZER_PIN, LOW);

    dht.begin();
    setupFuzzy();

    // **Hubungkan ke Blynk**
    Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);
}

void loop() {
    Blynk.run();  // Jalankan Blynk

    h = dht.readHumidity();
    t = dht.readTemperature();

    if (isnan(h) || isnan(t)) {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Sensor Error!");
        Serial.println("Sensor Error!");
        delay(2000);
        return;
    }

    fuzzy->setInput(1, t);
    fuzzy->setInput(2, h);
    fuzzy->fuzzify();
    int kondisi = fuzzy->defuzzify(3);

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Suhu: ");
    lcd.print(t);
    lcd.print(" C");

    lcd.setCursor(0, 1);
    lcd.print("Status: ");

    if (kondisi < 50) {
        lcd.print("Nyaman");
        digitalWrite(RELAY_PIN, HIGH);
        digitalWrite(BUZZER_PIN, LOW);
        Blynk.virtualWrite(VIRTUAL_PIN_STATUS, "Nyaman");
    } else {
        lcd.print("TdkNymn");
        digitalWrite(RELAY_PIN, LOW);
        if (millis() - lastBuzzTime >= 1000) {
            lastBuzzTime = millis();
            buzzerState = !buzzerState;
            digitalWrite(BUZZER_PIN, buzzerState);
        }
        Blynk.virtualWrite(VIRTUAL_PIN_STATUS, "Tidak Nyaman");
    }

    Blynk.virtualWrite(VIRTUAL_PIN_TEMP, t);
    Blynk.virtualWrite(VIRTUAL_PIN_HUMI, h);
    Blynk.virtualWrite(V2, kondisi < 50 ? "Nyaman" : "Tidak Nyaman");
    Serial.print("Suhu: "); Serial.print(t);
    Serial.print(" C, Kelembaban: "); Serial.print(h);
    Serial.print(" %, Status: ");
    Serial.println(kondisi < 50 ? "Nyaman" : "Tidak Nyaman");

    delay(500);
}
