/* --- OFFSETS DE BYTES --- */
#define XBOX_BYTE_LX            0   // Stick Izquierdo X (2 bytes: 0 y 1)
#define XBOX_BYTE_LY            2   // Stick Izquierdo Y (2 bytes: 2 y 3)
#define XBOX_BYTE_RX            4   // Stick Derecho X (2 bytes: 4 y 5)
#define XBOX_BYTE_RY            6   // Stick Derecho Y (2 bytes: 6 y 7)
#define XBOX_BYTE_LT            8   // Gatillo Izquierdo (2 bytes: 8 y 9)
#define XBOX_BYTE_RT            10  // Gatillo Derecho (2 bytes: 10 y 11)
#define XBOX_BYTE_DPAD          12  // Cruceta (1: N, 3: E, 5: S, 7: O. valores numericos)
#define XBOX_BYTE_BUTTONS_1     13  // Grupo A, B, X, Y, LB, RB
#define XBOX_BYTE_BUTTONS_2     14  // Grupo Menu, View, Xbox
#define XBOX_BYTE_SHARE         15  // Botón Share

/* --- MÁSCARAS DE BITS (BITMASKS) --- */
// Byte 13
#define XBOX_MASK_A             0x01 // 00000001
#define XBOX_MASK_B             0x02 // 00000010
#define XBOX_MASK_X             0x08 // 00001000
#define XBOX_MASK_Y             0x10 // 00010000
#define XBOX_MASK_LB            0x40 // 01000000
#define XBOX_MASK_RB            0x80 // 10000000

// Byte 14
#define XBOX_MASK_VIEW          0x04 // Select
#define XBOX_MASK_MENU          0x08 // Start
#define XBOX_MASK_HOME          0x10 // Botón central Xbox

// Byte 15
#define XBOX_MASK_SHARE         0x01 // Botón Share