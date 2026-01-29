#include <stdio.h>
#include <stdlib.h>

int main(void)
{
    FILE *fp = fopen("little_bin_file", "rb");
    if (fp == NULL) 
    {
        perror("Failed to open file");
        return 1;
    }
    int integer_number;
    double double_number;
    char character;
    float float_number;

    if (fread(&integer_number, sizeof integer_number, 1, fp) != 1 ||
        fread(&double_number, sizeof double_number, 1, fp) != 1 ||
        fread(&character, sizeof character, 1, fp) != 1 ||
        fread(&float_number, sizeof float_number, 1, fp) != 1) {
        perror("Failed to read file");
        fclose(fp);
        return 1;
    }
    fclose(fp);

    printf("%d\n", integer_number);
    printf("%f\n", double_number);
    printf("%c\n", character);
    printf("%f", float_number);
    return 0;
}
