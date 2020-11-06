typedef struct node
{
    char value[81];
    struct node *next, *prev;
}list;

typedef struct nope
{
    list *first;
    list *last;
}queue;
