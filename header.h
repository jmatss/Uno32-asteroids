extern const uint8_t const font[128 * 8];
extern const uint8_t const player[8];
extern const uint8_t const asteroid[8];
extern const uint8_t const bullet[2];
extern const uint8_t const siffror[4 * 10];
extern const uint8_t const pekare[5];
extern const uint8_t const nedre_pekare[8];
extern const uint8_t const ovre_pekare[8];

extern uint8_t display_buffer[4][128];

void send_highscore(uint8_t difficulty, uint8_t* name, uint16_t score, int placering);
uint16_t char_to_memory_char(uint8_t* name);
void memory_char_to_char(uint16_t name_memory_char, uint8_t* name);
