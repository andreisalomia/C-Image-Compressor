/*SALOMIA Andrei-Gabriel 314CC*/
#include "func.h"

int main(int argc, char *argv[])
{
    FILE *f_in = fopen(argv[argc - 2], "r");
    FILE *f_out = fopen(argv[argc - 1], "w");

    ULL red = 0, green = 0, blue = 0;
    int factor, i, j;
    char command[20];
    /*Atribuirea valorii factorului daca nu se cere decomprimarea*/
    if (argc > 4)
    {
        factor = atoi(argv[argc - 3]);
        strcpy(command, argv[argc - 4]);
    }
    else
    {
        strcpy(command, argv[argc - 3]);
    }

    if (strcmp(command, "-c2") == 0 || strcmp(command, "-c1") == 0)
    {
        char tip_ppm[4];
        int width, height, max_colour;
        /* Citirea informatiilor specifice fisierelor ppm*/
        fscanf(f_in, "%s", tip_ppm);
        fscanf(f_in, "%d", &width);
        fscanf(f_in, "%d", &height);
        fscanf(f_in, "%d", &max_colour);
        unsigned int n = height;
        /*Pozitionarea la inceputul partii binare din fisier*/
        fseek(f_in, 1, SEEK_CUR);

        pixel **matrix = (pixel **)malloc(n * sizeof(pixel *));
        for (i = 0; i < n; i++)
        {
            matrix[i] = (pixel *)malloc(n * sizeof(pixel));
        }
        read_file(f_in, matrix, n);

        quadtree q;
        q.head = NULL;
        q.high_leaf = INT_MAX;
        q.head = create_tree(matrix, &red, &green, &blue, n, factor, 0, 0);
        q.levels = find_height(q.head);
        q.leaves = how_many_leaves(q.head);
        if (strcmp(command, "-c1") == 0)
        {
            go_through_tree(q.head, q.levels, &q.high_leaf, command, f_out);
            /*Zona cea mai mare ramasa nedivizata este 2 la puterea celei mai
            "inalte" frunze din arbore.*/
            q.high_leaf = n / pow(2, q.high_leaf);
            fprintf(f_out, "%d\n%d\n%d\n", q.levels, q.leaves, q.high_leaf);
        }
        else if (strcmp(command, "-c2") == 0)
        {
            fwrite(&n, sizeof(n), 1, f_out);
            go_through_tree(q.head, q.levels, &q.high_leaf, command, f_out);
        }

        free_matrix(matrix, n);
        free_quadtree(q.head);
    }

    if (strcmp(command, "-d") == 0)
    {
        quadtree q;
        unsigned int img_size;
        fread(&img_size, sizeof(unsigned int), 1, f_in);
        int n_copy = img_size;
        int max_level = 1;
        while (n_copy != 0)
        {
            n_copy = n_copy / 2;
            max_level++;
        }
        q.head = malloc(sizeof(node));
        q.head = NULL;
        for (int l = 1; l <= max_level; l++)
        {
            create_dec_tree(q.head, 1, l, f_in, 0, img_size);
        }
        pixel **matrix = (pixel **)malloc(img_size * sizeof(pixel *));
        for (i = 0; i <= img_size; i++)
        {
            matrix[i] = (pixel *)malloc(img_size * sizeof(pixel));
        }
        printf("%d\n", img_size);
        fill_matrix(img_size, matrix, q.head, 1, 1, img_size);
        /*Afisarea informatiilor specifice fisierelor ppm.*/
        fprintf(f_out, "P6\n");
        fprintf(f_out, "%u %u\n", img_size, img_size);
        fprintf(f_out, "%u\n", 255);
        /*Afisarea imaginii in format binar.*/
        print_matrix(matrix, img_size, img_size, f_out);
        free_matrix(matrix, img_size);
        free_quadtree(q.head);
    }

    fclose(f_in);
    fclose(f_out);
}