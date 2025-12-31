#ifndef ECG_QRS_DETECTOR_H
#define ECG_QRS_DETECTOR_H

#include <Arduino.h>

class QRSDetector {
private:
  static const int RR_BUFFER_SIZE = 10;
  unsigned long rrIntervals[RR_BUFFER_SIZE]; // Intervalos RR en ms
  int rrIndex;
  unsigned long lastBeatTime;
  
public:
  QRSDetector();
  
  // Registrar un latido detectado
  void recordBeat(unsigned long currentTime);
  
  // Cálculo de BPM
  int calculateBPM();
  
  // Reset de estado
  void reset();
};

#endif // ECG_QRS_DETECTOR_H
