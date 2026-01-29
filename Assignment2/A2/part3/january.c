#include <stdio.h>
#include <stdlib.h>

// Day structure
typedef struct node
{
    int day;
    double min;
    double max;
    struct node *next;
} Day;

// Printing the list 
void print_list(Day *head)
{
    Day *current_day = head;
    printf("day\tmin\t\tmax\n");
    while (current_day != NULL)
    {
        printf("%d\t%.6f\t%.6f\n", current_day->day, current_day->min, current_day->max);
        current_day = current_day->next;
    }
}

// Adding a specific day to the list
void insert(Day **head, int day, double min, double max)
{
    if (day < 1 || day > 31) return;

    Day *previous = NULL;
    Day *current_day  = *head;

    // Finding the insertion point (sorted by day)
    while (current_day != NULL && current_day->day < day) {
        previous = current_day;
        current_day = current_day->next;
    }

    // Update if the day already exists
    if (current_day != NULL && current_day->day == day) {
        current_day->min = min;
        current_day->max = max;
        return;
    }

    // Insert a new day between previous and current day
    Day *new_day = malloc(sizeof *new_day);
    if (new_day == NULL) return;  

    new_day->day = day;
    new_day->min = min;
    new_day->max = max;
    new_day->next = current_day;

    if (previous == NULL) {
        // insert at beginning
        *head = new_day;
    } else {
        previous->next = new_day;
    }
}

// Remove a specific day
void remove_day(Day **head, int day)
{
    if (head == NULL || *head == NULL) return;

    Day *current_day = *head;
    Day *previous = NULL;

    while (current_day != NULL && current_day->day != day) {
        previous = current_day;
        current_day = current_day->next;
    }

    if (current_day == NULL) return; // not found

    if (previous == NULL) {
        // deleting head
        *head = current_day->next;
    } else {
        previous->next = current_day->next;
    }

    free(current_day);
}
// Free allocated memory of the list
void free_list(Day *head)
{
    Day *current_day = head;
    Day *next;

    while (current_day != NULL) {
        next = current_day->next;
        free(current_day);        
        current_day = next;
    }
}

int main(void)
{
    Day *head = NULL;
    char command;
    int day;
    double min, max;
    while(1)
    {
        printf("Enter command: ");
        if(scanf(" %c", &command) != 1)
        {
            printf("Invalid input\n");
            continue;
        }
        switch (command)
        {
        case 'A':
            if (scanf("%d %lf %lf", &day, &min, &max) != 3) 
            {
                printf("Invalid input\n");
                break;
            }
            insert(&head, day, min, max);
            break;
        case 'D':
            if (scanf("%d", &day) != 1) 
            {
                printf("Invalid input\n");
                break;
            }
            remove_day(&head, day);
            break;
        case 'P':
            print_list(head);
            break;
        case 'Q':
            free_list(head);
            return 0;    
        default:
            printf("Invalid command\n");
            break;
        }
    }
    return 0;
}
