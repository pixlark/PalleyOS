#include <kstdlib.h>
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>


// Compares the c strings str2 and str2
// Continues comparison while the characters in comparison are equal,
// or a null byte is encountered
// returns 0 on equal, 
// <0 if the non-matching char has a lower value in str1 than str2
// >0 if the non-matching char has a higher value in str1 than str2
size_t kstrcmp (const char* str1, const char* str2) {
	while(*str1 != '\0' && *str2 != '\0'){
		int diff = *str1 - *str2;
		if(diff != 0) return diff;
		str1++;
		str2++;
	}
	return 0;
}


// Compares the first num bytes of the block of memory pointed to by ptr1
// to that of ptr2. Returns zero if equal, or a non-zero value representing
// which is greater.
// **Note: unlike strcmp, this function does not stop comparing after finding
// 		   a null character
size_t kmemcmp (const void* ptr1, const void* ptr2, size_t num) {

	while(num) {
		int diff = *((uint8_t*)ptr1) - *((uint8_t*)ptr2);		
		if(diff != 0) return diff;
		ptr1 += 1;
		ptr2 += 1;
		num--;
	}
	return 0;
}

// Copies chars from src to destination until a null character is hit
// returns the number of chars written
size_t kstrcpy (char* dest, const char* src) {
    size_t num_written = 0;
	while(*src != '\0' && num_written < sizeof(size_t)){
        *dest = *src;
		dest++;
		src++;
	}
	return num_written;
}

// Copies num chars from src to dest
// returns the number of chars written
size_t kmemcpy (void* dest, const void* src, size_t num) {
    size_t num_written = 0;
	while(num_written < num){
        *((uint8_t*)dest) = *((uint8_t*)src);
		dest++;
		src++;
	}
	return num_written;
}
