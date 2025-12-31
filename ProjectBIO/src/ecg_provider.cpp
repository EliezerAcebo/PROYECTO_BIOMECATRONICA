#include "ecg_provider.h"

// Variable global para acceso desde ISR
static ECGProvider* g_ecgProvider = nullptr;

// ISR DEL TIMER - ULTRA MINIMA (sin filtros, sin Serial, sin floats complejos)
static void timerISR() {
  if (!g_ecgProvider) return;
  
  // SOLO lectura del ADC
  g_ecgProvider->rawADCValue = analogRead(ECG_ADC_PIN);
  
  // SOLO establecer bandera
  g_ecgProvider->newDataAvailable = true;
}

// Variables estáticas para filtrado (persistent entre llamadas)
static float baseline = 0;
static bool firstRun = true;
static unsigned long lastBeatTime = 0;
static int bpm = 0;
static const int BEAT_THRESHOLD = -200;
static const int BEAT_COOLDOWN = 400; // ms (Max ~150 BPM)

ECGProvider::ECGProvider() 
  : bufferIndex(0), bufferReady(false), 
    rawADCValue(0), newDataAvailable(false),
    filteredValue(0), currentBPM(0), qrsDetected(false), 
    samplingTimer(nullptr), filters(nullptr), qrsDetector(nullptr) {
  g_ecgProvider = this;
}

ECGProvider::~ECGProvider() {
  end();
}

void ECGProvider::begin() {
  // Configurar pines
  pinMode(ECG_ADC_PIN, INPUT);
  pinMode(LEADS_OFF_PLUS, INPUT);
  pinMode(LEADS_OFF_MINUS, INPUT);
  
  // Configurar ADC
  analogReadResolution(12);
  analogSetAttenuation(ADC_11db);
  
  // AHORA instanciar filtros y detector (seguro, fuera de ISR)
  if (!filters) {
    filters = new ECGFilters();
  }
  if (!qrsDetector) {
    qrsDetector = new QRSDetector();
  }
  
  // Inicializar variables
  baseline = 0;
  firstRun = true;
  lastBeatTime = 0;
  bpm = 0;
  
  // Iniciar timer
  initTimer();
}

void ECGProvider::end() {
  stopTimer();
  if (filters) {
    delete filters;
    filters = nullptr;
  }
  if (qrsDetector) {
    delete qrsDetector;
    qrsDetector = nullptr;
  }
}

void ECGProvider::initTimer() {
  // Timer 0, prescaler 80, 1/250Hz = 4000 microsegundos
  samplingTimer = timerBegin(0, 80, true);
  timerAttachInterrupt(samplingTimer, &timerISR, true);
  timerAlarmWrite(samplingTimer, (1000000 / SAMPLING_RATE), true);
  timerAlarmEnable(samplingTimer);
}

void ECGProvider::stopTimer() {
  if (samplingTimer) {
    timerAlarmDisable(samplingTimer);
    timerDetachInterrupt(samplingTimer);
    timerEnd(samplingTimer);
    samplingTimer = nullptr;
  }
}

void ECGProvider::processNewData() {
  if (!newDataAvailable) return;
  
  int raw = rawADCValue;
  newDataAvailable = false;
  
  if (firstRun) {
    baseline = raw;
    firstRun = false;
  }
  baseline = 0.97f * baseline + 0.03f * raw;
  
  filteredValue = raw - baseline;
  
  qrsDetected = false;
  unsigned long currentTime = millis();
  
  if (filteredValue < BEAT_THRESHOLD && (currentTime - lastBeatTime > BEAT_COOLDOWN)) {
    unsigned long delta = currentTime - lastBeatTime;
    lastBeatTime = currentTime;
    
    int instantBPM = (int)(60000.0f / delta);
    if (instantBPM > 40 && instantBPM < 160) {
      bpm = instantBPM;
      qrsDetected = true;
      currentBPM = bpm;
    }
  }
  
  uint16_t scaledFiltered = (uint16_t)((filteredValue + 500.0f) * (4096.0f / 1000.0f));
  scaledFiltered = constrain(scaledFiltered, 0, 4095);
  
  adcBuffer[bufferIndex] = scaledFiltered;
  bufferIndex++;
  if (bufferIndex >= BUFFER_SIZE) {
    bufferIndex = 0;
    bufferReady = true;
  }
}

bool ECGProvider::isLeadsOff() const {
  return (digitalRead(LEADS_OFF_PLUS) == HIGH) || (digitalRead(LEADS_OFF_MINUS) == HIGH);
}
