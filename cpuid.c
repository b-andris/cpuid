#include <stdio.h>
#include <stdint.h>
#include <stdarg.h>
#include <stdlib.h>

typedef struct cpuid_values_t {
	int32_t eax, ebx, ecx, edx;
} cpuid_values;

cpuid_values cpuid(int function) {
	cpuid_values values;
	__asm__("mov %0, %%eax;"
		"cpuid;"
		:"=a"(values.eax),"=b"(values.ebx),"=c"(values.ecx),"=d"(values.edx):"g"(function));
	return values;
}

static inline char* intsToStr(char* str, unsigned int count, ...){
	va_list v;
	va_start(v,count);
	for (unsigned int i = 0; i < count; i++) {
		((int*)str)[i] = va_arg(v,int32_t);
	}
	va_end(v);
	return str;
}

int main(int argc, char *argv[]) {
	cpuid_values values;

	values = cpuid(0);
	char* vendor = intsToStr(calloc(3 + 1, 4), 3, values.ebx, values.edx, values.ecx);
	
	char* name = calloc(3 * 4 + 1, 4);
	for (int i = 0; i < 3; i++) {
		values = cpuid(i + 0x80000002);
		intsToStr(name + (16 * i), 4, values.eax, values.ebx, values.ecx, values.edx);
	}
	
	printf("%s (by %s)\n", name, vendor);
	free(name);
	free(vendor);
}