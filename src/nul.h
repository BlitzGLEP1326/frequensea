// Utility

#ifndef NUL_H
#define NUL_H

#include <stdint.h>

// Buffer

typedef enum {
    NUL_BUFFER_U8 = 1,
    NUL_BUFFER_F64
} nul_buffer_type;

typedef union nul_buffer_data {
    uint8_t *u8;
    double *f64;
} nul_buffer_data;

typedef struct {
    nul_buffer_type type;
    int length;
    int channels;
    int size_bytes;
    nul_buffer_data data;
} nul_buffer;

nul_buffer *nul_buffer_new_u8(int length, int channels, const uint8_t *data);
nul_buffer *nul_buffer_new_f64(int length, int channels, const double *data);
nul_buffer *nul_buffer_copy(nul_buffer *buffer);
nul_buffer *nul_buffer_reduce(nul_buffer *buffer, double percentage);
void nul_buffer_set_data(nul_buffer *dst, nul_buffer *src);
void nul_buffer_append(nul_buffer *dst, nul_buffer *src);
uint8_t nul_buffer_get_u8(nul_buffer *buffer, int offset);
double nul_buffer_get_f64(nul_buffer *buffer, int offset);
void nul_buffer_set_u8(nul_buffer *buffer, int offset, uint8_t value);
void nul_buffer_set_f64(nul_buffer *buffer, int offset, double value);
nul_buffer *nul_buffer_convert(nul_buffer *buffer, nul_buffer_type new_type);
void nul_buffer_save(nul_buffer *buffer, const char *fname);
void nul_buffer_free(nul_buffer *buffer);

#endif // NUL_H
