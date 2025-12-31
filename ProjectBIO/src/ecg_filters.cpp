#include "ecg_filters.h"
#include <string.h>

ECGFilters::ECGFilters() 
  : hpState_y(0), hpState_x(0), lpIndex(0) {
  memset(lpBuffer, 0, sizeof(lpBuffer));
}

float ECGFilters::processSignal(float rawValue) {
  // ===== ETAPA 1: FILTRO PASA-ALTOS (elimina offset DC) =====
  // Fórmula: y[i] = 0.995 * y[i-1] + x[i] - x[i-1]
  float hpOutput = HP_ALPHA * hpState_y + (rawValue - hpState_x);
  
  // Actualizar estados
  hpState_y = hpOutput;
  hpState_x = rawValue;
  
  // ===== ETAPA 2: FILTRO PASA-BAJOS (Media Móvil) =====
  // Guardar valor en buffer circular
  lpBuffer[lpIndex] = hpOutput;
  lpIndex = (lpIndex + 1) % LP_WINDOW;
  
  // Promediar últimos 5 valores
  float sum = 0;
  for (int i = 0; i < LP_WINDOW; i++) {
    sum += lpBuffer[i];
  }
  float lpOutput = sum / LP_WINDOW;
  
  return lpOutput; // Señal filtrada, centrada en 0
}

void ECGFilters::reset() {
  hpState_y = 0;
  hpState_x = 0;
  lpIndex = 0;
  memset(lpBuffer, 0, sizeof(lpBuffer));
}
