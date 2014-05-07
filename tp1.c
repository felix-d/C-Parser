/*
 * =====================================================================================
 *
 *       Filename:  tp1.c
 *
 *    Description:  TP1 pour le cours de Concepts des langages de programmation E2014
 *
 *        Version:  1.0
 *        Created:  06/05/2014 23:44:47
 *       Revision:  none
 *       Compiler:  gcc
 *
 *        Authors:  Champagne, Pascal & Descoteaux, Felix 
 *
 * =====================================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


/*-----------------------------------------------------------------------------
 *  DEFINITIONS
 *-----------------------------------------------------------------------------*/


#define TRUE 1
#define FALSE 0
#define CHIFFRES "0123456789"


//Return true if x is of type "operator"
#define OPERATEUR(x) x->type == OP 


//Return true if op1 has precedence on op2
#define A_PRECEDENCE(op1, op2) op1->precedence > op2->precedence


//Print error related expression
#define PREFIX_ERREUR printf("Erreur dans expression #%d : ", nb_expr)


//Memory allocation
#define ALLOC_TYPE(type, var, items, ligne) \
    var = (type *)malloc(sizeof(type) * items); \
if (!var) \
{ \
    printf("Probleme lors de l'allocation de memoire a la ligne %d\n", ligne); \
    return FALSE; \
}


#define CALLOC_TYPE(type, var, items, ligne) \
    ALLOC_TYPE(type, var, items, ligne); \
memset(var, 0, sizeof(type) * items)


//Free pointer X
#define FREE_PTR(x) \
    if (x) \
{ \
    free(x); \
    x = NULL; \
}


/*-----------------------------------------------------------------------------
 *  TYPES DEFINITIONS
 *-----------------------------------------------------------------------------*/


typedef int bool;


//Op's signatures for _op struct
bool addition(double gauche, double droite, double *resultat);
bool soustraction(double gauche, double droite, double *resultat);
bool multiplication(double gauche, double droite, double *resultat);
bool division(double gauche, double droite, double *resultat);


//Function pointer for operation
typedef bool (*t_op_funct)(double gauche, double droite, double *resultat);


//Struct for operators
typedef struct _op
{
    char symbole;
    char precedence;
    char dependance;
    t_op_funct funct;
} t_op; 


//Struct for string
typedef struct _str
{
    long alloc;
    long util;
    char *str;
} t_str;


//Struct for expressions
typedef struct _expr
{
    //Type is either 'operation' or 'number', hence the union
    enum 
    {
        OP = 1, 
        NOMBRE
    } type;
    union 
    {
        struct
        {
            char *str;
        } nombre;
        //Operation with left and right members
        struct
        {
            t_op *op;
            struct _expr *gauche, *droite;
        } op;
    } _;
    //Different string formats
    char *strScheme;
    char *strC;
    char *strPostScript;
    double valeur;
} t_expr;


typedef t_expr t_ASA;


//Struct for the stack
typedef struct
{
    int alloc; //top?
    int util;
    t_expr **termes;
} t_pile_termes;


/*-----------------------------------------------------------------------------
 *  INITIALIZATIONS
 *-----------------------------------------------------------------------------*/


//List of possible operations
//symbol, precedance, dependance, op type
t_op liste_op[] = 
{ 
    {'+', 0x00, FALSE, addition}, 
    {'-', 0x00, TRUE, soustraction}, 
    {'*', 0x01, FALSE, multiplication}, 
    {'/', 0x01, TRUE, division} 
};


//List of postscript operations
char *postscript_op[] = {"add", "sub", "mul", "div"};


static int nb_expr = 1;


/*-----------------------------------------------------------------------------
 *  ERROR DEBUGGING LEVEL
 *-----------------------------------------------------------------------------*/


//Pre processing for error debugging
//#define tp1_debug
#ifdef tp1_debug
  #define DEBUG_PRINT(x) \
       PREFIX_ERREUR; \
       printf(x)
#else
  #define DEBUG_PRINT(x)
#endif


/*-----------------------------------------------------------------------------
 *  FUNCTIONS DEFINITIONS
 *-----------------------------------------------------------------------------*/


//Operations functions definitions
bool addition(double gauche, double droite, double *resultat)
{
    *resultat = gauche + droite;
    return TRUE;
}


bool soustraction(double gauche, double droite, double *resultat)
{
    *resultat = gauche - droite;
    return TRUE;
}


bool multiplication(double gauche, double droite, double *resultat)
{
    *resultat = gauche * droite;
    return TRUE;
}


bool division(double gauche, double droite, double *resultat)
{
    if (!droite)
    { 
        PREFIX_ERREUR;
        printf("Division par zéro.\n");
        return FALSE;
    }

    *resultat = gauche / droite;
    return TRUE;
}


//Return type of operator
t_op *type_op(char c)
{
    long i, end = sizeof(liste_op) / sizeof(liste_op[0]);

    for(i = 0; i < end; i++)
        if (liste_op[i].symbole == c)
            return &liste_op[i];

    return NULL;
}


//String duplication?
char *_my_strdup(char *p, int len)
{
    char *ptr = NULL;

    ALLOC_TYPE(char, ptr, len + 1, __LINE__);
    memcpy(ptr, p, len);
    ptr[len] = 0;

    return ptr;
}

 /* :WARNING:07/05/2014 09:35:58:: DEPRECATED */
// File reading of the expression
bool lire_expr_postfix_DEPRECATED(t_str *s, FILE *infile)
{
    char c;
    s->util = 0;

    do
    {
        c = fgetc(infile);

        if (feof(infile))
            break;

        if (s->util == s->alloc)
        {
            t_str temp;
            temp.alloc = !s->alloc ? 128 : s->alloc << 1;
            /* Ce qui suit est est un simili realloc en utilisant malloc et free
               La raison d'être de ce simili realloc est que l'énoncé stipule qu'il
               faut utiliser la fonction malloc pour l'allocation de la mémoire. */
            ALLOC_TYPE(char, temp.str, temp.alloc, __LINE__);
            memcpy(temp.str, s->str, s->alloc * sizeof(char));
            FREE_PTR(s->str);
            s->alloc = temp.alloc;
            s->str = temp.str;
        }

        if (c == '\n') c = 0;

        //
        s->str[s->util++] = c;

        if (!c) break;
    }
    while(1);

    return TRUE;
}


//Push an expression on the stack
bool push_pile_termes(t_pile_termes *p, t_expr *noeud)
{
    if (p->util == p->alloc)
    {
        t_pile_termes temp;

        temp.alloc = !p->alloc ? 16 : p->alloc << 1;
        /* Ce qui suit est est un simili realloc en utilisant malloc et free
           La raison d'être de ce simili realloc est que l'énoncé stipule qu'il
           faut utiliser la fonction malloc pour l'allocation de la mémoire. */
        ALLOC_TYPE(t_expr *, temp.termes, temp.alloc, __LINE__);
        memcpy(temp.termes, p->termes, p->alloc * sizeof(t_expr **));
        FREE_PTR(p->termes);
        p->alloc = temp.alloc;
        p->termes = temp.termes;
    }

    p->termes[p->util] = noeud;
    p->util++;

    return TRUE;
}


//Pop the stack
bool pop_pile_termes(t_pile_termes *p, t_expr **noeud)
{
    if (!p->util)
        return FALSE;

    p->util--;
    *noeud = p->termes[p->util];

    return TRUE;
}


//Free expression
void liberer_expr(t_expr **p)
{
    if (!*p)
        return;

    if ((*p)->type == OP)
    {
        liberer_expr(&(*p)->_.op.gauche);
        liberer_expr(&(*p)->_.op.droite);

        FREE_PTR((*p)->strScheme);
        FREE_PTR((*p)->strC);
        FREE_PTR((*p)->strPostScript);
        FREE_PTR(*p);

        return;
    }

    if ((*p)->type == NOMBRE)
    {
        FREE_PTR((*p)->_.nombre.str);
        (*p)->strScheme = (*p)->strC = (*p)->strPostScript = NULL;
        FREE_PTR((*p));

        return;
    }

    PREFIX_ERREUR;
    printf("liberer_expr : Type de noeud inconnu.  BUG!\n");
    exit(2);
}


//Free stack
void liberer_pile_termes(t_pile_termes *p)
{
    t_expr *temp;

    while(pop_pile_termes(p, &temp))
        liberer_expr(&temp);
}


//Convert postfix to abstract syntax
bool convertir_postfix_en_ASA(t_str *s, t_pile_termes *pile_termes, t_ASA **ASA)
{
    char *p; //string postfix temporaire
    t_expr *pTemp; //expr postfix temporaire
    t_op *pOP;
    int len;
    bool succes = FALSE, syntaxe_invalide = FALSE;

    printf("%s",s->str);
    for(p = s->str, len = 1, pTemp = NULL; 
            *p; 
            p += len, len = 1, pTemp = NULL)
    {    
        if (*p == ' ')
            continue;

        if ((pOP = type_op(*p)))
        {
            CALLOC_TYPE(t_expr, pTemp, 1, __LINE__);

            pTemp->type = OP;
            pTemp->_.op.op = pOP;

            if (!pop_pile_termes(pile_termes, &pTemp->_.op.droite))
            {
                syntaxe_invalide = TRUE;
                DEBUG_PRINT("Terme de droite manquant.\n");
                goto wrapup;
            }

            if (!pop_pile_termes(pile_termes, &pTemp->_.op.gauche))
            {
                syntaxe_invalide = TRUE;
                DEBUG_PRINT("Terme de gauche manquant.\n");
                goto wrapup;
            }

            if (!push_pile_termes(pile_termes, pTemp))
                goto wrapup;

            continue;
        }

        if ((len = strspn(p, CHIFFRES)))
        {
            CALLOC_TYPE(t_expr, pTemp, 1, __LINE__);

            pTemp->type = NOMBRE;
            pTemp->_.nombre.str = _my_strdup(p, len);
            if (!pTemp->_.nombre.str)
                goto wrapup;

            if (!push_pile_termes(pile_termes, pTemp))
                goto wrapup;

            continue;
        }

        syntaxe_invalide = TRUE;
        DEBUG_PRINT("Caractère inconnu.\n");
        goto wrapup;
    }  

    if (pile_termes->util != 1)
    {
        syntaxe_invalide = TRUE;
        DEBUG_PRINT("Trop de termes à traiter pour le nombre d'opération.\n");
        goto wrapup;
    }

    pop_pile_termes(pile_termes, ASA);

    succes = TRUE;

wrapup:

    if (!succes)
    {
        liberer_pile_termes(pile_termes);

        *ASA = NULL;

        if (pTemp)
            liberer_expr(&pTemp);

        if (syntaxe_invalide)
            printf("ERREUR DE SYNTAXE!\n\n");
    }

    return succes;
}


//Add brackets to an expression
void ajouter_parentheses(t_expr *parent, t_expr *gauche, bool *parenthese_gauche, t_expr *droite, bool *parenthese_droite)
{
    *parenthese_gauche = FALSE;
    *parenthese_droite = FALSE;

    if (OPERATEUR(gauche))
        *parenthese_gauche = A_PRECEDENCE(parent->_.op.op, gauche->_.op.op);

    if (OPERATEUR(droite))
    {
        *parenthese_droite = A_PRECEDENCE(parent->_.op.op, droite->_.op.op);

        if (!*parenthese_droite)
        {
            *parenthese_droite = parent->_.op.op->precedence == droite->_.op.op->precedence && 
                parent->_.op.op->dependance;

        }
    }
}


//Generate an expression
bool generer_expressions(t_expr *p)
{
    if (p->type == OP)
    {
        size_t len;
        bool parenthese_gauche, parenthese_droite;

        if (!generer_expressions(p->_.op.gauche))
            return FALSE;

        if (!generer_expressions(p->_.op.droite))
            return FALSE;

        // "(op gauche droite)" -->   + 5 -> "(+  )" + 1 -> "null terminator"
        len = strlen(p->_.op.gauche->strScheme) + strlen(p->_.op.droite->strScheme) + 5 + 1;
        ALLOC_TYPE(char, p->strScheme, len, __LINE__);
        sprintf(p->strScheme, "(%c %s %s)", p->_.op.op->symbole, p->_.op.gauche->strScheme, p->_.op.droite->strScheme);

        // "gauche droite op" -->   + 5 -> "  xxx" + 1 -> "null terminator"
        len = strlen(p->_.op.gauche->strPostScript) + strlen(p->_.op.droite->strPostScript) + 5 + 1;
        ALLOC_TYPE(char, p->strPostScript, len, __LINE__);
        sprintf(p->strPostScript, 
                "%s %s %s", 
                p->_.op.gauche->strPostScript, 
                p->_.op.droite->strPostScript, 
                postscript_op[p->_.op.op - liste_op]
               );

        // p->strC
        ajouter_parentheses(p, p->_.op.gauche, &parenthese_gauche, p->_.op.droite, &parenthese_droite);

        // "gaucheopdroite" -->   + 1 -> "x" + 1 -> "null terminator"
        len = strlen(p->_.op.gauche->strC) + strlen(p->_.op.droite->strC) + 1 + 1;
        len += parenthese_gauche ? 2 : 0;
        len += parenthese_droite ? 2 : 0;
        ALLOC_TYPE(char, p->strC, len, __LINE__);
        sprintf(p->strC, 
                "%s%s%s%c%s%s%s",
                parenthese_gauche ? "(" : "",
                p->_.op.gauche->strC, 
                parenthese_gauche ? ")" : "",
                p->_.op.op->symbole,
                parenthese_droite ? "(" : "",
                p->_.op.droite->strC, 
                parenthese_droite ? ")" : ""
               );

        //printf("%f %c %f\n", p->_.op.gauche->valeur, p->_.op.op->symbole, p->_.op.droite->valeur);
        if (!p->_.op.op->funct(p->_.op.gauche->valeur, p->_.op.droite->valeur, &p->valeur))
            return FALSE;

        return TRUE;
    }

    if (p->type == NOMBRE)
    {
        p->strScheme = p->strC = p->strPostScript = p->_.nombre.str;
        p->valeur = strtod(p->_.nombre.str, NULL);
        // check errno ???
        return TRUE;
    }

    PREFIX_ERREUR;
    printf("generer_expressions : Type de noeud inconnu.  BUG!\n");
    exit(2);
}


//Print an expression
void imprimer_expressions(t_str *s, t_expr *p)
{
    printf("    Scheme: %s\n", p->strScheme);
    printf("         C: %s\n", p->strC);
    printf("Postscript: %s\n", p->strPostScript);
    printf("    Valeur: %g\n\n", p->valeur);
}


/*-----------------------------------------------------------------------------
 *  MAIN
 *-----------------------------------------------------------------------------*/


int main(int argc, char **argv)
{
    int status = 2;
    bool input_errors = TRUE;
    t_str s = {0, 0, NULL}; //alloc,util,*char
    t_pile_termes pile_termes = {0, 0, NULL}; //alloc,util,**t_expr
    
    //Declaration of 3 pointers for base expression members
    t_expr *expr1;
    t_expr *expr2;
    t_expr *expr3;

    //Input
    char input[1000];

    //Declaration of abstract syntax tree
    t_ASA *pASA = NULL;

    //La boucle va boucler tant quil y a des expressions en input
    while(1){
        //Output
        printf("EXPRESSION? ");

        //Get input
        fgets(input, sizeof input, stdin);
        s.str = input;

       
        //On convertie l'expression en postfix
        if (!convertir_postfix_en_ASA(&s, &pile_termes, &pASA))
            continue;

        //On genere les differentes expressions
        if (!generer_expressions(pASA))    
            continue;

        //On imprime les differentes expressions
        imprimer_expressions(&s, pASA);

        
        //FREE MEMORY*****
        
        //On libere le pointeur a larbre de syntaxe pour lexpression courante 
        liberer_expr(&pASA);

        //On libere le stack
        if (pile_termes.alloc)
        {
            FREE_PTR(pile_termes.termes);
            pile_termes.util = pile_termes.alloc = 0;
        }

        //On libere la string
        if (s.alloc)
        {
            FREE_PTR(s.str);
            s.alloc = s.util = 0;
        }
    }
   
    return 1;
}
