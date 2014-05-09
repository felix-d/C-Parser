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

#define tp1_debug
//#define stack_debug
#define TRUE 1
#define FALSE 0
#define CHIFFRES "0123456789"


//Return true if x is of type "operator"
#define OPERATEUR(x) x->type == OP 


//Return true if op1 has precedence on op2
#define A_PRECEDENCE(op1, op2) op1->precedence > op2->precedence

//Memory allocation
#define ALLOC_TYPE(type, var, items, ligne) \
    var = (type *)malloc(sizeof(type) * items); \
if (!var) \
{ \
    debug_line = ligne;\
    DEBUG_PRINT(MEMORY_ERROR);\
    return FALSE; \
}

//Malloc + set la memoire a zero
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
        t_str *nombre;
        //Operation with left and right members
        struct
        {
            t_op *op;
            struct _expr *gauche, *droite;
        } op;
    } _;
    //Different string formats
    t_str *strScheme;
    t_str *strC;
    t_str *strPostScript;
    double valeur;
} t_expr;

typedef enum {
    UNKNOWN_NODE_TYPE,
    MISSING_RIGHT_TERM,
    MISSING_LEFT_TERM,
    UNKNOWN_CHARACTER,
    TOO_MANY_TERMS,
    SYNTAX_ERROR,
    MEMORY_ERROR,
    DIVIDE_BY_ZERO
} t_error; 


typedef t_expr t_ASA;


//Struct for the stack
typedef struct
{
    int alloc; //number of elements
    int util; //position actuelle dans le stack
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
static int debug_line = 0;


/*-----------------------------------------------------------------------------
 *  ERROR DEBUGGING LEVEL
 *-----------------------------------------------------------------------------*/


//Pre processing for error debugging
#ifdef tp1_debug
#define DEBUG_PRINT(x) \
    ERROR(x); 
#else
#define DEBUG_PRINT(e)
#endif
#ifdef stack_debug
#define DEBUG_STACK(s) \
    print_stack(s);
#else
#define DEBUG_STACK(s)
#endif


/*-----------------------------------------------------------------------------
 *  FUNCTIONS DEFINITIONS
 *-----------------------------------------------------------------------------*/


/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  void ERROR(t_error e)
 *  Description:  Error output
 * =====================================================================================
 */
void ERROR(t_error e){
    switch(e){
        case UNKNOWN_NODE_TYPE:
            printf("Type de noeud inconnu!\n");
            break;
        case MISSING_RIGHT_TERM:
            printf("Terme de droite manquant!\n");
            break;
        case MISSING_LEFT_TERM:
            printf("Terme de gauche manquant!\n");
            break;

        case UNKNOWN_CHARACTER:
            printf("Caractere inconnu!\n");
            break;

        case TOO_MANY_TERMS:
            printf("Trop de termes!\n");
            break;

        case SYNTAX_ERROR:
            printf("Erreur de syntaxe!\n");
            break;

        case MEMORY_ERROR:
            printf("Probleme lors de l'allocation de memoire a la ligne %d\n", debug_line ); 
            break;

        case DIVIDE_BY_ZERO:
            printf("Divison par zero!\n");
            break;

        default:
            printf("Erreur inconnue!\n");
            break;
    }
}


/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  void print_t_expr(t_expr* ex)
 *  Description:  Print t_expr
 * =====================================================================================
 */
void print_atomic_t_expr(t_expr* ex)
{

    if(ex->type==2)
        printf(" %c ", *ex->_.nombre->str);
    else if(ex->type==1)
        printf(" %c ", ex->_.op.op->symbole);

}


/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  void print_stack(t_pile_termes* p)
 *  Description:  Prints stack p content.
 * =====================================================================================
 */
void print_stack(t_pile_termes* p)
{
    printf("Stack: ");

    if(p->util==0)printf(" empty "); 
    for(int i=0;i<p->util;i++){ 
        print_atomic_t_expr(p->termes[i]);
    }
    printf("\n");
}


/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  bool addition(double gauche, double droite, double *resultat)
 *  Description:  Sum of gauche and droite
 * =====================================================================================
 */
bool addition(double gauche, double droite, double *resultat)
{
    *resultat = gauche + droite;
    return TRUE;
}


/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  bool soustraction(double gauche, double droite, double *resultat)
 *  Description:  Difference of gauche and droite
 * =====================================================================================
 */
bool soustraction(double gauche, double droite, double *resultat)
{
    *resultat = gauche - droite;
    return TRUE;
}


/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  bool multiplication(double gauche, double droite, double *resultat)
 *  Description:  Product of gauche and droite
 * =====================================================================================
 */
bool multiplication(double gauche, double droite, double *resultat)
{
    *resultat = gauche * droite;
    return TRUE;
}


/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  bool division(double gauche, double droite, double *resultat)
 *  Description:  Quotient of gauche and droite
 * =====================================================================================
 */
bool division(double gauche, double droite, double *resultat)
{
    if (!droite)
    { 
        DEBUG_PRINT(DIVIDE_BY_ZERO);
        return FALSE;
    }

    *resultat = gauche / droite;
    return TRUE;
}



/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  t_op *type_op(char c)
 *  Description:  Return the operator type of a character
 * =====================================================================================
 */
t_op *type_op(char c)
{
    long i, end = sizeof(liste_op) / sizeof(liste_op[0]);

    for(i = 0; i < end; i++)
        if (liste_op[i].symbole == c)
            return &liste_op[i];

    return NULL;
}





/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  bool newStringHeap(char *s, long len, t_str **str, long line)
 *  Description:  Creates a full struct t_str with a trailing char buffer for (t_str *)->str
 *                Copies null terminated string if s is not NULL
 * =====================================================================================
 */
bool newString(char *s, long len, t_str **str, long line)
{
  t_str *pTemp;

  if (!str)
  {
    printf("**str est NULL défaut venant de la ligne %d\n", __LINE__ ? !line : (int)line);
    return FALSE;
  }

  ALLOC_TYPE(t_str, pTemp, sizeof(pTemp) + len + 1, __LINE__ ? !line : (int)line);

  pTemp->str = (char *)(pTemp + 1);  // premier byte au delà de la struct t_str

  if (s)
  {
    memcpy(pTemp->str, s, len);
    pTemp->str[len] = 0;
    pTemp->util = len;
  }
  else
  {
    pTemp->util = 0;
  }

  pTemp->alloc = len + 1;

  *str = pTemp;

  return TRUE;
}

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  void delString(t_str **str)
 *  Description:  Frees the String and sets the pointer to NULL
 * =====================================================================================
 */
void delString(t_str **str)
{
  if (!str)
    return;

  FREE_PTR(*str);
}





/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  bool lire_expr_postfix(t_str *s, FILE *infile)
 *  Description:  Read expression from string  
 * =====================================================================================
 */
bool lire_expr_postfix(t_str *s, FILE *stream)
{
    char c;
    s->util = 0;

    do
    {

        c = fgetc(stream);
        if (feof(stream)) 
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

        s->str[s->util++] = c;

        if (!c) break;
    }
    while(1);

    return TRUE;
}


/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  bool push_pile_termes(t_pile_termes *p, t_expr *noeud)
 *  Description:  Push an expression on the stack, after increasing its size by 16
 * =====================================================================================
 */
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
    DEBUG_STACK(p);
    return TRUE;
}


/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  bool pop_pile_termes(t_pile_termes *p, t_expr **noeud)
 *  Description:  Pop an expression from the stack
 * =====================================================================================
 */
bool pop_pile_termes(t_pile_termes *p, t_expr **noeud)
{
    if (!p->util) //si la pile est vide
        return FALSE;
    p->termes[p->util] = NULL;
    p->util--; //decrement current position on stack
    *noeud = p->termes[p->util]; //on retourne le noeud

    DEBUG_STACK(p);

    return TRUE;
}


/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  void liberer_expr(t_expr **p)
 *  Description:  Free recursively memory of p
 * =====================================================================================
 */
void liberer_expr(t_expr **p)
{
    //Le pointeur est null
    if (!*p)
        return;

    //Le pointeur est une t_expr de type OP
    if ((*p)->type == OP)
    {
        //Appel recursif pour liberer le terme gauche et droit
        liberer_expr(&(*p)->_.op.gauche);
        liberer_expr(&(*p)->_.op.droite);

        //Liberation des pointeurs
        delString(&(*p)->strScheme);
        delString(&(*p)->strC);
        delString(&(*p)->strPostScript);
        FREE_PTR(*p);

        return;
    }

    if ((*p)->type == NOMBRE)
    {
        delString(&((*p)->_.nombre));
        (*p)->strScheme = (*p)->strC = (*p)->strPostScript = NULL;
        FREE_PTR((*p));

        return;
    }

    DEBUG_PRINT(UNKNOWN_NODE_TYPE);
    exit(2);
}



/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  void liberer_pile_termes(t_pile_termes *p)
 *  Description:  Free stack 
 * =====================================================================================
 */
void liberer_pile_termes(t_pile_termes *p)
{
    t_expr *temp;

    while(pop_pile_termes(p, &temp))
        liberer_expr(&temp);
}


/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  bool convertir_postfix_en_ASA(t_str *s, t_pile_termes *pile_termes, t_ASA **ASA)
 *  Description:  Convert postfix expression to abstract syntax tree using a stack
 * =====================================================================================
 */
bool convertir_postfix_en_ASA(t_str *s, t_pile_termes *pile_termes, t_ASA **ASA)
{
    char *p; 
    t_expr *pTemp;
    t_op *pOP;
    int len;
    bool succes = FALSE, syntaxe_invalide = FALSE;

    //BOUCLE QUI PARCOURT LA STRING
    for(p = s->str, len = 1, pTemp = NULL; 
            *p; 
            p += len, len = 1, pTemp = NULL)
    {    
        //SI LE CARACTERE EST UN ESPACE VIDE
        if (*p == ' '){ 
            continue;
        }

        //SI CEST UN OPERATEUR
        else if ((pOP = type_op(*p))) 
        {
            CALLOC_TYPE(t_expr, pTemp, 1, __LINE__); 

            pTemp->type = OP; 
            pTemp->_.op.op = pOP;

            if (!pop_pile_termes(pile_termes, &pTemp->_.op.droite))
            {
                syntaxe_invalide = TRUE;
                DEBUG_PRINT(MISSING_RIGHT_TERM);
                break;
            }

            else if (!pop_pile_termes(pile_termes, &pTemp->_.op.gauche))
            {
                syntaxe_invalide = TRUE;
                DEBUG_PRINT(MISSING_LEFT_TERM);
                break;
            }

            else if (!push_pile_termes(pile_termes, pTemp)) 
                break;

            continue;
        }

        //SI CEST UN CHIFFRE
        else if ((len = strspn(p, CHIFFRES)))
        {
            CALLOC_TYPE(t_expr, pTemp, 1, __LINE__);

            pTemp->type = NOMBRE;
            if (!newString(p, len, &pTemp->_.nombre, __LINE__))
                break;

            else if (!push_pile_termes(pile_termes, pTemp))//
                break;

            continue;
        }

        else
        {
            syntaxe_invalide = TRUE;
            DEBUG_PRINT(UNKNOWN_CHARACTER);
            break;
        }
    }  

    if(pile_termes->util == 1)
    {

        pop_pile_termes(pile_termes, ASA);
        succes = TRUE;
    }
    else
    {
        syntaxe_invalide = TRUE;
        if(pile_termes->util>1)DEBUG_PRINT(TOO_MANY_TERMS);
    }

    if (!succes)
    {
        liberer_pile_termes(pile_termes);
        *ASA = NULL;

        if (pTemp)
            liberer_expr(&pTemp);

        if (syntaxe_invalide)
            DEBUG_PRINT(SYNTAX_ERROR);
    }

    return succes;
}



/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  void ajouter_parentheses(t_expr *parent, t_expr *gauche, bool *parenthese_gauche, t_expr *droite, bool *parenthese_droite)
 *  Description:  Ajout de parentheses a l'expression
 * =====================================================================================
 */
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


/* 
 * ===  FUNCTION  ======================================================================
 *         Name: bool generer_expressions(t_expr *p)
 *  Description: Generate expressions in C, PostScript, Scheme and get value
 * =====================================================================================
 */
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

        // SCHEME
        // "(op gauche droite)" -->   + 5 -> "(+  )" + 1 -> "null terminator"
        len = p->_.op.gauche->strScheme->util + p->_.op.droite->strScheme->util + 5 + 1;
        if (!newString(NULL, len, &p->strScheme, __LINE__))
          return FALSE;
        sprintf(p->strScheme->str, "(%c %s %s)", p->_.op.op->symbole, p->_.op.gauche->strScheme->str, p->_.op.droite->strScheme->str);


        //POSTSCRIPT
        // "gauche droite op" -->   + 5 -> "  xxx" + 1 -> "null terminator"
        len = p->_.op.gauche->strPostScript->util + p->_.op.droite->strPostScript->util + 5 + 1;
        if (!newString(NULL, len, &p->strPostScript, __LINE__))
          return FALSE;
        sprintf(p->strPostScript->str, 
                "%s %s %s", 
                p->_.op.gauche->strPostScript->str, 
                p->_.op.droite->strPostScript->str, 
                postscript_op[p->_.op.op - liste_op]
               );

        //C
        // p->strC
        ajouter_parentheses(p, p->_.op.gauche, &parenthese_gauche, p->_.op.droite, &parenthese_droite);

        // "gaucheopdroite" -->   + 1 -> "x" + 1 -> "null terminator"
        len = p->_.op.gauche->strC->util + p->_.op.droite->strC->util + 1 + 1;

        if (!newString(NULL, len, &p->strC, __LINE__))
          return FALSE;

        char *parenthese_gauche_str_gauche;
        char *parenthese_gauche_str_droite;
        if (parenthese_gauche)
        {
            len += 2;
            parenthese_gauche_str_gauche = "(";
            parenthese_gauche_str_droite = ")";
        }
        else
        {
            parenthese_gauche_str_gauche = "";
            parenthese_gauche_str_droite = "";
        }

        char *parenthese_droite_str_gauche;
        char *parenthese_droite_str_droite;
        if (parenthese_droite)
        {
          len += 2;
          parenthese_droite_str_gauche = "(";
          parenthese_droite_str_droite = ")";
        }
        else
        {
          parenthese_droite_str_gauche = "";
          parenthese_droite_str_droite = "";
        }
        sprintf(p->strC->str, 
                "%s%s%s%c%s%s%s",
                parenthese_gauche_str_gauche,
                p->_.op.gauche->strC->str, 
                parenthese_gauche_str_droite,
                p->_.op.op->symbole,
                parenthese_droite_str_gauche,
                p->_.op.droite->strC->str, 
                parenthese_droite_str_droite
               );

        //printf("%f %c %f\n", p->_.op.gauche->valeur, p->_.op.op->symbole, p->_.op.droite->valeur);
        if (!p->_.op.op->funct(p->_.op.gauche->valeur, p->_.op.droite->valeur, &p->valeur))
            return FALSE;

        return TRUE;
    }

    if (p->type == NOMBRE)
    {
        p->strScheme = p->strC = p->strPostScript = p->_.nombre;
        p->valeur = strtod(p->_.nombre->str, NULL);
        // check errno ???
        return TRUE;
    }

    DEBUG_PRINT(UNKNOWN_NODE_TYPE);
    exit(2);
}


//Print an expression
void imprimer_expressions(t_str *s, t_expr *p)
{
    printf("    Scheme: %s\n", p->strScheme->str);
    printf("         C: %s\n", p->strC->str);
    printf("Postscript: %s\n", p->strPostScript->str);
    printf("    Valeur: %g\n\n", p->valeur);
}


/*-----------------------------------------------------------------------------
 *  MAIN
 *-----------------------------------------------------------------------------*/


int main(int argc, char **argv)
{
    int status = 2;
    bool input_errors = TRUE;
    t_str *s = NULL;
    t_pile_termes pile_termes = {0, 0, NULL}; 

    //Declaration of 3 pointers for base expression members
    t_expr *expr1;
    t_expr *expr2;
    t_expr *expr3;

    //Declaration of abstract syntax tree
    t_ASA *pASA = NULL;

    newString(NULL, 0x400, &s, __LINE__);  // start with 1Kb

    while(1){

        printf("Expression? ");
        liberer_expr(&pASA);

        if (!lire_expr_postfix(s, stdin))
            continue; 

        if (feof(stdin)) {  // ctrl-d sets EOF
          printf("\n");
          break;
        }

        if (!*s->str || 
                (strspn(s->str, " ") == s->util - 1))
            continue;

        if (!strcmp(s->str, "^D"))
            break;

        if (!convertir_postfix_en_ASA(s, &pile_termes, &pASA))
            continue;

        if (!generer_expressions(pASA))    
            continue;

        imprimer_expressions(s, pASA);
        nb_expr++;
    }  

    if (pile_termes.alloc)
    {
        FREE_PTR(pile_termes.termes);
        pile_termes.util = pile_termes.alloc = 0;
    }

    delString(&s);

    return 1;}
