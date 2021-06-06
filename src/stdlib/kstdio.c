#include <stdarg.h>
#include <stdint.h>

static void render_integer(char* result, size_t* result_i, int32_t to_render) {
    if (to_render < 0) {
        result[*result_i] = '-';
        *result_i += 1;
        to_render *= -1;
    }
    
    size_t digits = 1;
    { // Count # of digits
        size_t iter = to_render;
        while (1) {
            if (iter < 10) {
                break;
            }
            iter /= 10;
            digits++;
        }
    }
    { // Render digits
        size_t place = 1;
        for (size_t i = 0; i < digits - 1; i++) {
            place *= 10;
        }
        for (size_t i = 0; i < digits; i++) {
            size_t digit = to_render;
            digit %= (10 * place);
            digit /= place;
            
            result[*result_i] = digit + '0';
            *result_i += 1;

            place /= 10;
        }
    }
}

static void render_unsigned_integer(char* result, size_t* result_i, uint32_t to_render) {
    size_t digits = 1;
    { // Count # of digits
        size_t iter = to_render;
        while (1) {
            if (iter < 10) {
                break;
            }
            iter /= 10;
            digits++;
        }
    }
    { // Render digits
        size_t place = 1;
        for (size_t i = 0; i < digits - 1; i++) {
            place *= 10;
        }
        for (size_t i = 0; i < digits; i++) {
            size_t digit = to_render;
            digit %= (10 * place);
            digit /= place;
            
            result[*result_i] = digit + '0';
            *result_i += 1;

            place /= 10;
        }
    }    
}

static void render_string(char* result, size_t* result_i, char* to_render) {
    while (*to_render != '\0') {
        result[*result_i] = *to_render;
        *result_i += 1;
        to_render += 1;
    }
}

void kvsprintf(char* result, const char* format, va_list args) {
    size_t result_i = 0;
    size_t format_i = 0;
    
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
    char result[1024];
    kvsprintf(result, format, args);
    printf("%s", result);
}

void kprintf(const char* format, ...) {
    va_list args;
    va_start(args, format);

    kvprintf(format, args);
    
    va_end(args);
}
