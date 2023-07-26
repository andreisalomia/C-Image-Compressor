#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <limits.h>

#define ULL unsigned long long

typedef struct Pixel
{
    unsigned char R;
    unsigned char G;
    unsigned char B;
} pixel;

typedef struct Node
{
    pixel P;
    int size;
    int leaf; // leaf retine daca nodul dat este frunza sau nu
    struct Node *stanga_sus;
    struct Node *dreapta_sus;
    struct Node *dreapta_jos;
    struct Node *stanga_jos;
} node;

typedef struct QuadTree
{
    node *head;
    int levels;
    int leaves;
    int high_leaf;
} quadtree;

void read_file(FILE *f, pixel **matrix, int size)
{
    int i, j;
    for (i = 0; i < size; i++)
    {
        for (j = 0; j < size; j++)
        {
            fread(&matrix[i][j], sizeof(pixel), 1, f);
        }
    }
}

int median(int x, int y, int size, pixel **matrix, int factor, ULL *red, ULL *green, ULL *blue)
{
    /*Functia care calculeaza factorul de similaritate pentru o portiune din
    matrice. Initial culorile RGB sunt 0, asupra lor fiind aplicate sumele date.
    Culorile RGB raman cu culoarea modificata pentru cazul in care blocul devine
    o singura culoare*/
    int i = 0, j = 0;
    unsigned long long mean = 0;
    *red = 0;
    *green = 0;
    *blue = 0;
    for (i = x; i < x + size; i++)
    {
        for (j = y; j < y + size; j++)
        {
            *red += matrix[i][j].R;
            *green += matrix[i][j].G;
            *blue += matrix[i][j].B;
        }
    }
    *red = *red / (size * size);
    *green = *green / (size * size);
    *blue = *blue / (size * size);
    *red = floor(*red);
    *green = floor(*green);
    *blue = floor(*blue);
    for (i = x; i < x + size; i++)
    {
        for (j = y; j < y + size; j++)
        {
            mean += (*red - matrix[i][j].R) * (*red - matrix[i][j].R) +
                    (*green - matrix[i][j].G) * (*green - matrix[i][j].G) +
                    (*blue - matrix[i][j].B) * (*blue - matrix[i][j].B);
        }
    }
    mean = mean / (3 * size * size);
    mean = floor(mean);

    if (mean <= factor)
        return 1;
    else
        return 0;
}

node *create_tree(pixel **matrix, ULL *red, ULL *green, ULL *blue, int size, int factor, int i, int j)
{
    node *head = malloc(sizeof(node));
    if ((median(i, j, size, matrix, factor, red, green, blue) == 1 || size == 1))
    {
        /*Daca scorul de similaritate este satisfacut sau marimea blocului este de 1,
        atunci nodul respectiv devine o frunza a arborelui.*/
        head->P.R = *red;
        head->P.G = *green;
        head->P.B = *blue;
        head->leaf = 1;
        head->dreapta_jos = NULL;
        head->dreapta_sus = NULL;
        head->stanga_jos = NULL;
        head->stanga_sus = NULL;
    }
    else
    {
        /*Daca zona trebuie impartita in alte 4 culori, se apeleaza recursiv
        functia de crearea arborelui pentru fiecare dintre cele 4 directii,
        tinand cont de pozitionarea in matrice*/
        head->leaf = 0;
        head->stanga_sus = create_tree(matrix, red, green, blue, size / 2, factor, i, j);
        head->dreapta_sus = create_tree(matrix, red, green, blue, size / 2, factor, i, j + size / 2);
        head->dreapta_jos = create_tree(matrix, red, green, blue, size / 2, factor, i + size / 2, j + size / 2);
        head->stanga_jos = create_tree(matrix, red, green, blue, size / 2, factor, i + size / 2, j);
    }

    return head;
}

int which_is_max(int a, int b, int c, int d)
{
    /*O functie ajutatoare care afla maximul dintre 4 valori*/
    if (a >= b)
        b = a;
    else
        a = b;

    if (c >= d)
        d = c;
    else
        c = d;

    if (a >= c)
        return a;
    else
        return c;
}

int find_height(node *head)
{
    /*O functie recursiva care parcurge arborele in cele 4 directii
    si incrementeaza valorile cand se trece pe un nivel nou.*/
    int ss, ds, dj, sj;
    if (head == NULL)
        return 0;
    ss = find_height(head->stanga_sus);
    ds = find_height(head->dreapta_sus);
    dj = find_height(head->dreapta_jos);
    sj = find_height(head->stanga_jos);

    return which_is_max(ss, ds, dj, sj) + 1;
}

void levelXnodes(node *head, int level, int given_level, int *highest_leaf, char *command, FILE *f_out)
{
    /*Functie care parcurge nivelurile arborelui si scrie in fisier
    nodurile nivel cu nivel. In acelasi timp calculeaza si pe ce nivel
    se afla cea mai inalta frunza, care va ajuta la aflarea celui
    mai mare bloc care ramane nedivizat.*/
    unsigned char is = 1, isnt = 0;
    if (head == NULL)
        return;

    if (level == given_level)
    {
        if (head->leaf == 1)
        {
            if (level < *highest_leaf)
                *highest_leaf = level;
            if (strcmp(command, "-c2") == 0)
            {
                fwrite(&is, sizeof(is), 1, f_out);
                fwrite(&head->P.R, sizeof(unsigned char), 1, f_out);
                fwrite(&head->P.G, sizeof(unsigned char), 1, f_out);
                fwrite(&head->P.B, sizeof(unsigned char), 1, f_out);
            }
            return;
        }
        else if (head->leaf == 0)
        {
            if (strcmp(command, "-c2") == 0)
                fwrite(&isnt, sizeof(isnt), 1, f_out);
            return;
        }
    }

    levelXnodes(head->stanga_sus, level + 1, given_level, highest_leaf, command, f_out);
    levelXnodes(head->dreapta_sus, level + 1, given_level, highest_leaf, command, f_out);
    levelXnodes(head->dreapta_jos, level + 1, given_level, highest_leaf, command, f_out);
    levelXnodes(head->stanga_jos, level + 1, given_level, highest_leaf, command, f_out);
}

void go_through_tree(node *head, int height, int *highest_leaf, char *command, FILE *f_out)
{
    int i;
    for (i = 0; i < height; i++)
    {
        levelXnodes(head, 0, i, highest_leaf, command, f_out);
    }
}

int how_many_leaves(node *head)
{
    /*Functie recursiva care numara toate frunzele arborelui*/
    if (head == NULL)
        return 0;
    if (head->dreapta_jos == NULL && head->dreapta_sus == NULL && head->stanga_jos == NULL && head->stanga_sus == NULL)
    {
        return 1;
    }
    else
    {
        return how_many_leaves(head->dreapta_jos) +
               how_many_leaves(head->dreapta_sus) +
               how_many_leaves(head->stanga_jos) +
               how_many_leaves(head->stanga_sus);
    }
}

void free_quadtree(node *head)
{
    /*Este parcurs arborele si eliberat fiecare nod.*/
    if (head == NULL)
    {
        return;
    }

    free_quadtree(head->stanga_sus);
    free_quadtree(head->dreapta_sus);
    free_quadtree(head->dreapta_jos);
    free_quadtree(head->stanga_jos);

    free(head);
}

void free_matrix(pixel **matrix, int n)
{
    int i;
    for (i = 0; i < n; i++)
    {
        free(matrix[i]);
    }
    free(matrix);
}

void create_dec_tree(node *head, int curr_level, int reading_level, FILE *f_in, int end, int height)
{
    /*Functie care citeste nodurile din fisierul binar si creeaza arborele
    pe baza tipului de nod citit. Daca nodul nu este frunza se face apelarea
    recursiva, in celalalt caz se citesc culorile din fisierul binar.*/
    int read=0;
    int size = height, aux_level = curr_level - 1;
    while (aux_level)
    {
        size = size / 2;
        aux_level--;
    }
    if (curr_level > reading_level || end == 1)
    {
        return;
    }

    if (head != NULL)
    {
        if (curr_level == reading_level)
        {
            read = fread(&head->leaf, sizeof(unsigned char), 1, f_in);
            if (head->leaf == 1)
            {
                read = fread(&head->P.R, sizeof(unsigned char), 1, f_in);
                read = fread(&head->P.G, sizeof(unsigned char), 1, f_in);
                read = fread(&head->P.B, sizeof(unsigned char), 1, f_in);
            }
            if (head->leaf == 0)
            {
                if (read == 0)
                    end = 1;
                head->stanga_sus = malloc(sizeof(node));
                head->dreapta_sus = malloc(sizeof(node));
                head->stanga_jos = malloc(sizeof(node));
                head->dreapta_jos = malloc(sizeof(node));

                head->stanga_sus->stanga_sus = NULL;
                head->stanga_sus->dreapta_sus = NULL;
                head->stanga_sus->stanga_jos = NULL;
                head->stanga_sus->dreapta_jos = NULL;

                head->dreapta_sus->stanga_sus = NULL;
                head->dreapta_sus->dreapta_sus = NULL;
                head->dreapta_sus->stanga_jos = NULL;
                head->dreapta_sus->dreapta_jos = NULL;

                head->stanga_jos->stanga_sus = NULL;
                head->stanga_jos->dreapta_sus = NULL;
                head->stanga_jos->stanga_jos = NULL;
                head->stanga_jos->dreapta_jos = NULL;

                head->dreapta_jos->stanga_sus = NULL;
                head->dreapta_jos->dreapta_sus = NULL;
                head->dreapta_jos->stanga_jos = NULL;
                head->dreapta_jos->dreapta_jos = NULL;
            }
        }
        if (head->leaf == 0)
        {
            create_dec_tree(head->stanga_sus, curr_level + 1, reading_level, f_in, end, height);
            create_dec_tree(head->stanga_sus, curr_level + 1, reading_level, f_in, end, height);
            create_dec_tree(head->stanga_sus, curr_level + 1, reading_level, f_in, end, height);
            create_dec_tree(head->stanga_sus, curr_level + 1, reading_level, f_in, end, height);
        }
    }
}

void fill_matrix(int n, pixel **matrix, node *head, int x, int y, int size)
{
    /*Functie care pe baza arborelui este umpluta recursiv matricea de culorile,
    adica imaginea decomprimata. Argumentele functiei sunt date astfel incat sa
    corespunda celor 4 zone din matrice.*/
    int i, j;
    if (head != NULL)
    {
        {
        if (head->leaf == 1)
        {
            printf("sulea");
            for (i = x; i < x + size; i++)
            {
                for (j = y; j < y + size; j++)
                {
                    matrix[i][j].R = head->P.R;
                    matrix[i][j].G = head->P.G;
                    matrix[i][j].B = head->P.B;
                    printf(" {%d %d %d} ", head->P.R, head->P.G, head->P.R);
                }
            }
        }
    }
    if(head->leaf==0)
    {
        fill_matrix(n, matrix, head->stanga_sus, x, y, size / 2);
        fill_matrix(n, matrix, head->dreapta_sus, x, y + size / 2, size / 2);
        fill_matrix(n, matrix, head->dreapta_jos, x + size / 2, y + size / 2, size / 2);
        fill_matrix(n, matrix, head->stanga_jos, x + size / 2, y, size / 2);
    }
    }
}


void print_matrix(pixel **matrix, int rows, int cols, FILE *f_out)
{
    /*Afisarea matricei de culori, adica a imaginii decomprimate.*/
    int i, j;
    for (i = 0; i < rows; i++)
    {
        for (j = 0; j < cols; j++)
        {
            fwrite(&matrix[i][j].R, sizeof(unsigned char), 1, f_out);
            fwrite(&matrix[i][j].G, sizeof(unsigned char), 1, f_out);
            fwrite(&matrix[i][j].B, sizeof(unsigned char), 1, f_out);
        }
    }
}