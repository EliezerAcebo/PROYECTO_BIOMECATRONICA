#ifndef ECG_PROVIDER_H
#define ECG_PROVIDER_H

#include <Arduino.h>
#include "ecg_filters.h"
#include "ecg_qrs_detector.h"

// Configuración de Hardware
#define ECG_ADC_PIN 36        // VP (ADC1_CH0)
#define LEADS_OFF_PLUS 23     // LO+
#define LEADS_OFF_MINUS 22    // LO-
#define SAMPLING_RATE 250     // Hz

// Forward declaration
class ECGProvider;
static void timerISR();

class ECGProvider {
private:
  static const int BUFFER_SIZE = 250;
  uint16_t adcBuffer[BUFFER_SIZE];
  volatile int bufferIndex;
  volatile bool bufferReady;
  
  // Datos compartidos ISR <-> Loop (VOLATILE)
  volatile uint16_t rawADCValue;
  volatile bool newDataAvailable;
  
  // Componentes de procesamiento (NO en ISR)
  ECGFilters* filters;
  QRSDetector* qrsDetector;
  
  float filteredValue;
  int currentBPM;
  bool qrsDetected;
  
  // Timer handle
  hw_timer_t* samplingTimer;
  
  // Métodos privados
  void initTimer();
  void stopTimer();
  void processDataInLoop(); // Procesamiento fuera de ISR
  
  // Permitir acceso a timerISR
  friend void timerISR();
  
public:
  ECGProvider();
  ~ECGProvider();
  
  void begin();
  void end();
  
  // Lectura de datos
  float getFilteredValue() const { return filteredValue; }
  uint16_t getRawADC() const { return rawADCValue; }
  bool isQRSDetected() const { return qrsDetected; }
  int getBPM() const { return currentBPM; }
  bool isLeadsOff() const;
  
  // Procesamiento desde loop
  bool hasNewData() const { return newDataAvailable; }
  void processNewData(); // Llamar desde loop()
  
  // Estado del buffer
  bool isBufferReady() const { return bufferReady; }
  void resetBuffer() { bufferReady = false; }
  uint16_t* getBuffer() { return adcBuffer; }
  int getBufferSize() const { return BUFFER_SIZE; }
};

#endif // ECG_PROVIDER_H
