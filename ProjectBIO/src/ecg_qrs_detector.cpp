#include "ecg_qrs_detector.h"
#include <string.h>

QRSDetector::QRSDetector() 
  : rrIndex(0), lastBeatTime(0) {
  memset(rrIntervals, 0, sizeof(rrIntervals));
}

void QRSDetector::recordBeat(unsigned long currentTime) {
  // Calcular intervalo RR (tiempo entre latidos)
  if (lastBeatTime > 0) {
    unsigned long rrInterval = currentTime - lastBeatTime;
    
    // Guardar en buffer circular
    rrIntervals[rrIndex] = rrInterval;
    rrIndex = (rrIndex + 1) % RR_BUFFER_SIZE;
  }
  
  lastBeatTime = currentTime;
}

int QRSDetector::calculateBPM() {
  // Promediar intervalos RR válidos
  unsigned long sumRR = 0;
  int validCount = 0;
  
  for (int i = 0; i < RR_BUFFER_SIZE; i++) {
    if (rrIntervals[i] > 0) {
      sumRR += rrIntervals[i];
      validCount++;
    }
  }
  
  if (validCount == 0) return 0;
  
  unsigned long avgRR = sumRR / validCount;
  
  // BPM = 60000 / RR (en ms)
  int bpm = (int)(60000.0f / avgRR);
  
  // Rango válido de BPM: 40-200
  if (bpm < 40 || bpm > 200) return 0;
  
  return bpm;
}

void QRSDetector::reset() {
  memset(rrIntervals, 0, sizeof(rrIntervals));
  rrIndex = 0;
  lastBeatTime = 0;
}
