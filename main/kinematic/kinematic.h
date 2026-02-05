#ifndef KINEMATIC_H
#define KINEMATIC_H
/**
 * @brief Actualiza el movimiento del robot basándose en inputs del mando.
 * @param potencia Valor normalizado de aceleración (0.0 a 1.0)
 * @param direccion_stick Valor del stick (-1.0 a 1.0)
 */

 void actualizar_movimiento (float potencia, float direccion_stick);

#endif // KINEMATIC_H


