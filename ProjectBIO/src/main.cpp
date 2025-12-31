#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <soc/soc.h>
#include <soc/rtc_cntl_reg.h>
#include "ecg_provider.h"

// ========== CREDENCIALES ==========
const char* WIFI_SSID = "Redmi Note 11";
const char* WIFI_PASSWORD = "7e6p83buxkvaq8r";
const char* UBIDOTS_TOKEN = "BBUS-hwXuYbb4wOXkxAxu4NhcnOaxnx4rHR";
const char* UBIDOTS_URL = "http://industrial.api.ubidots.com/api/v1.6/devices/esp32-ecg/";

// ========== CONFIGURACION ==========
const unsigned long MIDE_DURATION = 12000;   
const unsigned long ENVIA_DURATION = 4000;   

// UMBRAL: 100 
int Threshold = 100;           
const unsigned long DEBOUNCE = 300; 

// ========== VARIABLES ==========
ECGProvider ecg;
enum State { MIDE, ENVIA };
State currentState = MIDE;
unsigned long stateStartTime = 0;
unsigned long lastBeatTime = 0;
unsigned long lastPrintTime = 0;

// Variables visuales
int plotRaw = 2048;
int plotFilter = 0;
int plotBPM = 0; 

// Promedio
char payload[100];
int bpmSum = 0;
int bpmCount = 0;
int lastBpmSent = 0; 

bool wifiInitialized = false;
bool alreadySent = false;

// ========== ENVIAR A UBIDOTS ==========
void enviarUbidots(int bpm) {
  if (WiFi.status() != WL_CONNECTED) return;
  
  HTTPClient http;
  http.begin(UBIDOTS_URL);
  http.addHeader("Content-Type", "application/json");
  http.addHeader("X-Auth-Token", UBIDOTS_TOKEN);
  
  sprintf(payload, "{\"bpm\": %d}", bpm);
  Serial.print("[Ubidots] ENVIANDO: "); Serial.println(payload);
  
  int httpCode = http.POST(payload);
  Serial.print("[Ubidots] Respuesta: "); Serial.println(httpCode);
  http.end();
}

void setup() {
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); 
  Serial.begin(115200);
  delay(500);
  
  Serial.println("\n=== SISTEMA ECG: REINICIO LIMPIO ===");
  ecg.begin();
  
  WiFi.mode(WIFI_OFF);
  currentState = MIDE;
  stateStartTime = millis();
}

void loop() {
  unsigned long now = millis();
  unsigned long elapsed = now - stateStartTime;
  
  // ==========================================
  // ESTADO 1: MIDE
  // ==========================================
  if (currentState == MIDE) {
    
    if (ecg.hasNewData()) {
      ecg.processNewData();
      
      int currentRaw = ecg.getRawADC();
      int currentFilter = ecg.getFilteredValue();
      
      // --- DETECCION DIRECTA ---
      if (currentFilter < -Threshold) { 
        unsigned long timeSinceLast = now - lastBeatTime;
        
        if (timeSinceLast > DEBOUNCE) {
          int instantBPM = 60000 / timeSinceLast;
          
          if (instantBPM >= 40 && instantBPM <= 140) {
            lastBeatTime = now;
            plotBPM = instantBPM; // Actualiza visual
            bpmSum += instantBPM;
            bpmCount++;
            Serial.print("!!! LATIDO !!! BPM: "); Serial.println(instantBPM);
          }
        }
      }

      if (!ecg.isLeadsOff()) {
        plotRaw = currentRaw;
        plotFilter = currentFilter;
      }
    }
    
    // --- TELEPLOT ---
    if (now - lastPrintTime >= 40) {
      lastPrintTime = now;
      Serial.print(">ECG_Raw:"); Serial.println(plotRaw);
      Serial.print(">ECG_Filter:"); Serial.println(plotFilter);
      Serial.print(">BPM:"); Serial.println(plotBPM);
    }
    
    // --- CAMBIO DE TURNO (MIDE -> ENVIA) ---
    if (elapsed >= MIDE_DURATION) {
      int promedio = 0;
      
      if (bpmCount > 0) {
        promedio = bpmSum / bpmCount;
        lastBpmSent = promedio; 
      } else {
        promedio = lastBpmSent;
        if (promedio == 0) promedio = plotBPM;
        Serial.println("[ALERTA] Sin latidos nuevos.");
      }
      
      if (promedio < 40 && promedio > 0) promedio = 60; 
      lastBpmSent = promedio;

      Serial.print("--- FIN CICLO. Promedio: "); Serial.println(promedio);
      
      currentState = ENVIA;
      stateStartTime = now;
      wifiInitialized = false;
      alreadySent = false;
    }
  }
  
  // ==========================================
  // ESTADO 2: ENVIA
  // ==========================================
  else if (currentState == ENVIA) {
    if (!wifiInitialized) {
      wifiInitialized = true;
      WiFi.mode(WIFI_STA);
      WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
      Serial.println("[WiFi] Conectando...");
    }
    
    if (!alreadySent) {
      if (WiFi.status() == WL_CONNECTED) {
        enviarUbidots(lastBpmSent);
        alreadySent = true;
      } else if (elapsed > 3500) {
        Serial.println("[WiFi] Timeout.");
        alreadySent = true;
      }
    }
    
    // --- VOLVER A MEDIR (AQUÍ ESTÁ LA SOLUCIÓN) ---
    if (elapsed >= ENVIA_DURATION) {
      Serial.println("[WiFi] Apagando...");
      WiFi.disconnect(true);
      WiFi.mode(WIFI_OFF);
      
      // === RESETEO TOTAL DE VARIABLES ===
      // Esto elimina el "67" pegado. Empezamos de cero.
      plotBPM = 0;       
      bpmSum = 0; 
      bpmCount = 0;
      
      // Truco: Reseteamos el reloj de latidos para que el primero entre fácil
      lastBeatTime = millis(); 
      
      currentState = MIDE;
      stateStartTime = now;
      Serial.println("--- VOLVIENDO A MEDIR (Reset) ---");
    }
  }
  yield();
}