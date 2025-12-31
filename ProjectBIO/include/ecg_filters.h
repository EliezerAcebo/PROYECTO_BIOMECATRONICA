#ifndef ECG_FILTERS_H
#define ECG_FILTERS_H

#include <Arduino.h>

class ECGFilters {
private:
  // High-Pass Filter (elimina DC)
  static constexpr float HP_ALPHA = 0.995f; // Coef. para HPF
  float hpState_y;      // Estado y[i-1]
  float hpState_x;      // Estado x[i-1]
  
  // Low-Pass Filter (Media móvil)
  static constexpr int LP_WINDOW = 5; // Promediar últimos 5 valores
  float lpBuffer[5];
  int lpIndex;
  
public:
  ECGFilters();
  
  // Procesar señal con cascada HPF -> LPF
  float processSignal(float rawValue);
  
  // Reset de estado
  void reset();
};

#endif // ECG_FILTERS_H
