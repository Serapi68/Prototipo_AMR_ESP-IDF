#ifndef AUTO_SENSORES_H
#define AUTO_SENSORES_H

/**
 * @brief Inicializa el módulo de evasión de obstáculos.
 * Se llama una vez al arrancar el sistema.
 */
void auto_sensores_init(void);

/**
 * @brief Ejecuta un ciclo de la máquina de estados de evasión de obstáculos.
 * Esta función debe ser llamada periódicamente.
 */
void auto_sensores_run(void);

#endif // AUTO_SENSORES_H