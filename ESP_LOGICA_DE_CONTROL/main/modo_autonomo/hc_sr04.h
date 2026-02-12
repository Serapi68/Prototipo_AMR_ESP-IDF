#ifndef HC_SR04_H
#define HC_SR04_H

//Inicia los pines del sensor HC-SR04
void hc_sr04_init(void);

/**
 * @brief Realiza una medición de distancia.
 * @return La distancia medida en centímetros. Devuelve un valor < 0 si hay un error o timeout.
 */
float hc_sr04_get_distance_cm(void);

#endif 
