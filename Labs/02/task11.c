#include <stdio.h>
#include <stdlib.h>

int main()
{
    int a;
    int count = 0;
    int capacity = 10;
    int *arr = malloc(capacity*sizeof(int));
    if (!arr) return 1;                         /* check if malloc failed */
    while (scanf("%d", &a) == 1)
    {
        if(count == capacity)
        {
            capacity+=10;
            int *tmp = realloc(arr, capacity*sizeof(int));
            if(!tmp)                            /* check if realloc failed */
            {
                free(arr);
                return 1;
            }
            arr = tmp;
        }
        arr[count] = a;
        count++;
    }
    
    int sum = 0;
    for (int i = 0; i < count; i++)
    {
        printf("\nNumber in position %d is %d ", i, arr[i]);
        sum +=arr[i];
    }
    printf("\n Sum: %d", sum);

    free(arr);
    return 0;
}