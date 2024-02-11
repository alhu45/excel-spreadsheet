#include "model.h"
#include "interface.h"
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>

//creating enums
typedef enum {
    num,
    txt,
    blank,
    formula
} cellType;

//structure of the linked list for the dependent cell(s)
typedef struct depCell {
    ROW row;
    COL col;
    struct depCell *next;
} depCell;

//head of the linked list is NULL
//depCell *head = NULL;

//structure of the array
typedef struct {
    cellType type;
    //union used to store different types
    union {
        double number;
        char *text;
        char *equation;
    } content;
    //the linked list of dependent cells if the cell is dependent
    depCell *dependencies;
} cellData;

//making the global array
cellData spreadsheet [10][7];

void model_init() {
    //loop through the spreadsheet to initialize the array with the structure
    for (int row = 0; row < 10; row++) {
        for (int col = 0; col < 7; col++) {
            //initializing all the arrays as NULL
            spreadsheet[row][col].type = blank;
            spreadsheet[row][col].content.text = NULL;
        }
    }
}

//creating a function to add a node to the dependent cell
void linkedlistDepCell(ROW row, COL col, int depRow, int depCol) {
    //allocating memory to make a new cell
    depCell *newNode = (depCell*)malloc(sizeof(depCell));

    //setting values of the head node to the dependent cell row and col
    newNode -> row = depRow;
    newNode -> col = depCol;

    //points to NULL for next node
    newNode -> next = NULL;

    //locating the last node in the linked list
    depCell *currentNode = spreadsheet[row][col].dependencies;


    //if a linked list does not exist, create one
    if (currentNode == NULL) {
        //if there is no current node
        //the linked list is empty set the newNode as the head of the list
        spreadsheet[row][col].dependencies = newNode;
    } else {
        //pointer pointing to head
        depCell *ptr = currentNode;

        while (ptr->next != NULL) {
            ptr = ptr->next;
        }
        spreadsheet[row][col].dependencies = newNode;
        ptr->next = newNode;
    }
}

//creating a function to update the cells that are dependent
void updatingCells(ROW row, COL col) {
    depCell *currentNode = spreadsheet[row][col].dependencies;

    //traversing through the linked list
    while(currentNode != NULL) {
        //calculates the value of the dependent cell corresponding to the change
        double newCellVal = calFormula(spreadsheet[currentNode->row][currentNode->col].content.equation, row, col);

        char temp[32];
        snprintf(temp, sizeof(temp), "%lf", newCellVal);
        update_cell_display(currentNode->row, currentNode->col, temp);

        //moves to the next dependent cell within the linked list
        currentNode = currentNode -> next;
    }
}

void set_cell_value(ROW row, COL col, char *text) {
    //if there is an existing content in cell, free the cells memory and add the new data into cell
    if(spreadsheet[row][col].type == txt && spreadsheet[row][col].content.text != NULL){
        free(spreadsheet[row][col].content.text);
    }
    char *ptr;
    long number = strtol(text, &ptr, 10);

    //handles formula
    if (text[0] == '=') {
        spreadsheet[row][col].type = formula;
        //duplicating string into the equation, ignores the '="
        spreadsheet[row][col].content.equation = strdup(text + 1);

        //if the cell is dependent on other cells, add the cells to a linked list
        linkedlistDepCell(row, col, row, col);

        double sum = calFormula(spreadsheet[row][col].content.equation, row, col);
        //creating temp array to store the value of
        char temp[32];
        snprintf(temp, sizeof(temp), "%lf", sum);
        update_cell_display(row, col, temp);

        //update dependent cell if required
        updatingCells(row, col);
    } else {
        //converting the string to an int
        if (*ptr == '\0') {
            //if the conversion was successful, store the integer in the cell
            spreadsheet[row][col].type = num;

            //check if it is dependent
            if (*ptr != '\0') {
                linkedlistDepCell(row, col, row, col);
                updatingCells(row, col);
            }

            spreadsheet[row][col].content.number = (double)number;
            update_cell_display(row, col, text);
        } else {
            //if the conversion failed, store the text in the cell
            spreadsheet[row][col].type = txt;
            spreadsheet[row][col].content.text = strdup(text);
            update_cell_display(row, col, text);
        }
    }

    // This just displays the text without saving it in any data structure. You will need to change this.
    //update_cell_display(row, col, text);
    free(text);
}

//function to calculate the cells
double calFormula (char *formulae, ROW row, COL col) {
    //splits the values when there is a "+"
    char *split;
    //sum value to add the values of the cells or numbers
    double sum = 0;
    //
    char *form;

    char* formulaFinal = strdup(formulae);
    //transversing through the formula to split the cells
    //the function strtok is a function used to break strings into smaller tokens when it sees a "+"
    while((split = strtok_r(formulaFinal, "+", &formulaFinal))) {
        //if the first index is a letter
        if(isalpha(split[0])) {
            //subtracts the number value by 1 because the array starts at [0]
            int newRow = split[1] - '1';
            //converts the letter into the corresponding column
            int newCol = toupper(split[0]) - 'A';

            //
            if(newRow >= 10 || newRow <0 || newCol >= 7 || newCol < 0) {
                return 0;
            }

            //adds dependent cells to the linked list
            linkedlistDepCell(newRow, newCol,row, col);


            //if the new cell is a number=
            if(spreadsheet[newRow][newCol].type == num) {
                //adding the sum(s) together
                sum += spreadsheet[newRow][newCol].content.number;
            } else {
                printf("Not Finished");
                return sum;
            }
        } else {
            //if the first index is not a letter, it is a number
            //adds the sum of the numbers
            sum += strtof(split, &form); //what is form for?
        }
    }

    return sum;
}

void clear_cell(ROW row, COL col) {

    //if there is an existing content in cell, free the cells memory and add the new data into cell
    if(spreadsheet[row][col].type == txt && spreadsheet[row][col].content.text != NULL){
        free(spreadsheet[row][col].content.text);
    }

    spreadsheet[row][col].type = blank;
    spreadsheet[row][col].content.text = NULL;

    // This just clears the display without updating any data structure. You will need to change this.
    update_cell_display(row, col, "");
}

char *get_textual_value(ROW row, COL col) {
    //temporary pointer to hold the textual value
    char *show;

    //if data type is number, display number
    if (spreadsheet[row][col].type == num) {
        show = malloc(15 * sizeof(char));
        snprintf(show, 20, "%f", spreadsheet[row][col].content.number);

    //if data type is text, display text
    } else if (spreadsheet[row][col].type == txt ) {
        show = malloc(strlen(spreadsheet[row][col].content.text) + 1);
        strcpy(show, spreadsheet[row][col].content.text);

    //if it was equation, show the sum of the products
    } else if (spreadsheet[row][col].type == formula){
        show = malloc(strlen(spreadsheet[row][col].content.equation) + 1);
        snprintf(show, strlen(spreadsheet[row][col].content.equation) + 3, "=%s", spreadsheet[row][col].content.equation);
    } else {
        show = malloc(1);
        show[0] = '\0';
    }
    return show;
}

