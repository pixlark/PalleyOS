#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <tio.h>

static void render_integer(char* result, uint32_t* result_i, int32_t to_render) {
    if (to_render < 0) {
        result[*result_i] = '-';
        *result_i += 1;
        to_render *= -1;
    }
    
    int32_t iter = to_render;
    uint8_t buffer[32];
    size_t i = 0;
    while (true) {
        buffer[i++] = iter % 10;
        iter /= 10;
        if (iter == 0) {
            break;
        }
    }
    for (int32_t j = i - 1; j >= 0; j--) {
        result[*result_i] = buffer[j] + '0';
        *result_i += 1;
    }
}

static void render_unsigned_integer(char* result, uint32_t* result_i, uint32_t to_render) {
    uint32_t iter = to_render;
    uint8_t buffer[32];
    size_t i = 0;
    while (true) {
        buffer[i++] = iter % 10;
        iter /= 10;
        if (iter == 0) {
            break;
        }
    }
    for (int32_t j = i - 1; j >= 0; j--) {
        result[*result_i] = buffer[j] + '0';
        *result_i += 1;
    }
}

static void render_hexadecimal(char* result, uint32_t* result_i, uint32_t to_render) {
    uint32_t iter = to_render;
    uint8_t buffer[32];
    size_t i = 0;
    while (true) {
        buffer[i++] = iter % 16;
        iter /= 16;
        if (iter == 0) {
            break;
        }
    }
    char hex_digits[] = {
        '0', '1', '2', '3',
        '4', '5', '6', '7',
        '8', '9', 'a', 'b',
        'c', 'd', 'e', 'f'
    };
    for (int32_t j = i - 1; j >= 0; j--) {
        result[*result_i] = hex_digits[buffer[j]];
        *result_i += 1;
    }
}

static void render_binary(char* result, uint32_t* result_i, uint32_t to_render) {
    uint32_t iter = to_render;
    uint8_t buffer[32];
    size_t i = 0;
    while (true) {
        buffer[i++] = iter % 2;
        iter /= 2;
        if (iter == 0) {
            break;
        }
    }
    for (int32_t j = i - 1; j >= 0; j--) {
        result[*result_i] = '0' + buffer[j];
        *result_i += 1;
    }    
}

static void render_string(char* result, uint32_t* result_i, char* to_render) {
    while (*to_render != '\0') {
        result[*result_i] = *to_render;
        *result_i += 1;
        to_render += 1;
    }
}

void kvsprintf(char* result, const char* format, va_list args) {
    uint32_t result_i = 0;
    uint32_t format_i = 0;
    
    while (format[format_i] != '\0') {
        if (format[format_i] == '%') {
            format_i++;
            switch (format[format_i++]) {
            case 'd': {
                int32_t arg = va_arg(args, int32_t);
                render_integer(result, &result_i, arg);
            } break;
            case 'u': {
                uint32_t arg = va_arg(args, uint32_t);
                render_unsigned_integer(result, &result_i, arg);
            } break;
            case 'x': {
                uint32_t arg = va_arg(args, uint32_t);
                render_hexadecimal(result, &result_i, arg);
            } break;
            case 'b': {
                uint32_t arg = va_arg(args, uint32_t);
                render_binary(result, &result_i, arg);
            } break;
            case 's': {
                char* arg = va_arg(args, char*);
                render_string(result, &result_i, arg);
            } break;
            case 'c': {
                int arg = va_arg(args, int);
                result[result_i++] = arg;
            } break;
            default: {
                return;
            } break;
            }
        } else {
            result[result_i] = format[format_i];
            result_i++;
            format_i++;
        }
    }

    result[result_i] = '\0';
}

void ksprintf(char* result, const char* format, ...) {
    va_list args;
    va_start(args, format);

    kvsprintf(result, format, args);
    
    va_end(args);
}

void kvprintf(const char* format, va_list args) {
    char result[512];
    kvsprintf(result, format, args);
    term_write(result);
}

void kprintf(const char* format, ...) {
    va_list args;
    va_start(args, format);

    kvprintf(format, args);
    
    va_end(args);
}

