#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

typedef

char* reverse(char* string);

int main(int argc, char* argv[]) {
	char* string = "lots of strings";
	printf("%s\n", string);
	char* reversed = reverse(string);
	printf("%s\n", reversed);
	
	free(reversed);
	return EXIT_SUCCESS;
}

char* reverse(char* string) {
	char* reversed = malloc(sizeof(char) * strlen(string) + 1);
	printf("%s\n", string);
	int c = 0;
	while (c < strlen(string)) {
		//printf("%c to %c\n", reversed[(strlen(string) - c) - 1], reversed[c]);
		reversed[c] = reversed[(strlen(string) - c) - 1];
		c++;
	}
	return reversed;
}