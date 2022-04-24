#ifndef UDP_TIMESTAMP_H
#define UDP_TIMESTAMP_H

#include <ctime>
#include <cstdint>

/* current time in milliseconds since the start of the program */
uint64_t get_current_timestamp();
uint64_t timestamp_ms();
uint64_t timestamp_ms(const timespec &ts);

#endif //UDP_TIMESTAMP_H