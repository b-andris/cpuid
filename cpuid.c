#include <stdio.h>
#include <stdint.h>
#include <stdarg.h>
#include <stdlib.h>

char* intsToStr(char* str, unsigned int count, ...){
	va_list v;
	va_start(v, count);
	for (unsigned int i = 0; i < count; i++) ((int*)str)[i] = va_arg(v, int32_t);
	va_end(v);
	return str;
}

int main(int argc, char *argv[]) {
	int32_t eax, ebx, ecx, edx;

	__asm__("cpuid;" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(0));
	char* vendor = intsToStr(calloc(3 + 1, sizeof(int32_t)), 3, ebx, edx, ecx);
	
	char* name = calloc(3 * 4 + 1, sizeof(int32_t));
	for (int i = 0; i < 3; i++) {
		__asm__("cpuid;" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(i + 0x80000002));
		intsToStr(name + (16 * i), 4, eax, ebx, ecx, edx);
	}
	
	printf("%s (by %s)\n", name, vendor);
	free(name);
	free(vendor);
}