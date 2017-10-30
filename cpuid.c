#include <stdio.h>
#include <stdint.h>
#include <stdarg.h>
#include <stdlib.h>
#include <assert.h>
#include <stdbool.h>

typedef struct {
	int32_t eax, ebx, ecx, edx;
} cpu_dat_s;

static void* val(void* p) {
	if (!p) {
		fputs("No more memory. Aborting.\n", stderr);
		exit(EXIT_FAILURE);
	}
	return p;
}

static char* intsToStr(char* str, unsigned int count, ...){
	va_list v;
	va_start(v, count);
	for (unsigned int i = 0; i < count; i++) ((int*)str)[i] = va_arg(v, int32_t);
	va_end(v);
	return str;
}

static cpu_dat_s cpuid(uint32_t fnc) {
	cpu_dat_s ret;
	__asm__("cpuid;" : "=a"(ret.eax), "=b"(ret.ebx), "=c"(ret.ecx), "=d"(ret.edx) : "a"(fnc));
	return ret;
}

static cpu_dat_s cpuid_ind(uint32_t fnc, uint32_t ind) {
	cpu_dat_s ret;
	__asm__("cpuid;" : "=a"(ret.eax), "=b"(ret.ebx), "=c"(ret.ecx), "=d"(ret.edx) : "a"(fnc), "c"(ind));
	return ret;
}

static uint32_t bits(uint32_t data, uint8_t start, uint8_t end) {
	assert(start < 32);
	assert(end < 32);
	if (start > end) {
		uint8_t tmp = end;
		end = start;
		start = tmp;
	}
	data >>= start;
	uint32_t mask = (uint32_t)-1 >> (31 + start - end);
	return data & mask;
}

static uint32_t bit(uint32_t data, uint8_t bit) {
	return bits(data, bit, bit);
}

static void print_model(FILE* f, bool header) {
	assert(f);
	cpu_dat_s res = cpuid(0);
	char* vendor = intsToStr(calloc(3 + 1, sizeof(int32_t)), 3, res.ebx, res.edx, res.ecx);
	
	char* name = calloc(3 * 4 + 1, sizeof(int32_t));
	for (int i = 0; i < 3; i++) {
		res = cpuid(i + 0x80000002);
		intsToStr(name + (16 * i), 4, res.eax, res.ebx, res.ecx, res.edx);
	}
	
	if (header) fputs("[Model]\n", f);
	fprintf(f, "%s (by %s)\n\n", name, vendor);
	free(name);
	free(vendor);
}

static void print_cache(FILE* f, bool header) {
	assert(f);
	if (header) fputs("[Cache]\n", f);
	cpu_dat_s res = cpuid(0);
	if (res.eax < 4) {
		fputs("Not supported by CPU\n\n", f);
		return;
	}
	uint32_t ind = 0;
	fputs("Levl\tType\tInit\tLine\tPart\tWays\tSets\t   Size\t  INVD\tIncl\tIndx\n", f);
	fputs("------------------------------------------------------------------------------------\n", f);
	while (1) {
		res = cpuid_ind(4, ind++);
		uint32_t type = bits(res.eax, 0, 4);
		if (!type) break;
		uint32_t level = bits(res.eax, 5, 7);
		uint32_t init = bit(res.eax, 8);
		uint32_t assoc = bit(res.eax, 9);
		char* type_desc;
		switch (type) {
			case 1: type_desc = "data"; break;
			case 2: type_desc = "inst"; break;
			case 3: type_desc = "unif"; break;
			default:type_desc = "unkn"; break;
		}
		uint32_t line = bits(res.ebx, 0, 11) + 1;
		uint32_t partitions = bits(res.ebx, 12, 21) + 1;
		uint32_t ways = bits(res.ebx, 22, 31) + 1;
		uint32_t sets = res.ecx + 1;
		uint32_t size = line * partitions * ways * sets;
		char size_class = ' ';
		while (!(size % 1024) && size) {
			size /= 1024;
			switch (size_class) {
				case ' ': size_class = 'k'; break;
				case 'k': size_class = 'M'; break;
				case 'M': size_class = 'G'; break;
				case 'G': size_class = 'T'; break;
				default : size_class = '?'; break;
			}
		}
		
		uint32_t invalidate = bit(res.edx, 0);
		uint32_t inclusive = bit(res.edx, 1);
		uint32_t indexing = bit(res.edx, 2);
		
		fprintf(f, "L%d\t%s\t%s\t%2d B\t%4d\t%4d\t%4d\t%3d %ciB\t%s\t%s\t%s\n", level, type_desc, init? "self" : "  SW", line, partitions, ways, sets, size, size_class, invalidate? "    no" : "   yes", inclusive? " yes" : "  no", indexing? "cmpl" : "drct");
	}
	fputs("\n", f);
}

static void print_help(FILE* f, bool header) {
	assert(f);
	if (header) fputs("[Help]\n", f);
	fputs("Usage:\tcpuid -[options] [output file]\n\n", f);
	fputs("Option\tHeader\t\tMeaning\n", f);
	fputs("-------------------------------------------------------------\n", f);
	fputs("c:\t[Cache]\t\tcache information \n", f);
	fputs("h:\t[Help]\t\tthis help message\n", f);
	fputs("H:\t\t\ttoggle headers on/off (on by default)\n", f);
	fputs("m:\t[Model]\t\tCPU model\n", f);
	fputc('\n', f);
}

int main(int argc, char* argv[]) {
	char* filename = NULL;
	size_t buf_size = 5;
	size_t buf_count = 0;
	char* options = val(malloc(buf_size));
	
	for (char** args = argv + 1; *args; args++) {
		if (**args == '-') {
			for (char* s = (*args) + 1; *s; s++) {
				if (++buf_count > buf_size) {
					buf_size *= 2;
					options = val(realloc(options, buf_size));
				}
				options[buf_count - 1] = *s;
			}
		} else if (!filename) {
			filename = *args;
		} else {
			fputs("Multiple output files not supported.\nSee -h option for help.\n", stderr);
			exit(EXIT_FAILURE);
		}
	}
	options[buf_count] = '\0';
	
	if (!buf_count) {
		fputs("No options specified.\nSee -h option for help.\n", stderr);
		exit(EXIT_FAILURE);
	}
	FILE* file = filename? fopen(filename, "wx") : stdout;
	if (!file) {
		fputs("Output file could not be created. Aborting.\n", stderr);
		exit(EXIT_FAILURE);
	}
	
	bool header = true;
	
	for (char* s = options; *s; s++) {
		switch (*s) {
			case 'c': print_cache(file, header); break;
			case 'h': print_help(file, header); break;
			case 'H': header = !header; break;
			case 'm': print_model(file, header); break;
		}
	}
}