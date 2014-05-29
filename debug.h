/* Fichier: "debug.h", Time-stamp: <2005-04-03 20:22:19 feeley> */

/*
 * Ce fichier d'entete permet de detecter automatiquement certaines
 * erreurs de gestion memoire.  Les appels aux fonctions de librairie
 * "malloc", "calloc" et "free" sont interceptes pour verifier que
 * tous les blocs de memoire qui sont alloues (avec malloc et calloc)
 * ont ete recuperes (avec free) a la fin de l'execution de "main".
 * Le fichier "debug.h" doit etre inclu au tout debut du fichier C qui
 * l'utilise.
 */

#ifndef _DEBUG_H_
#define _DEBUG_H_

#include <stdio.h>
#include <stdlib.h>

typedef struct _t_resource
{
  int ligne;
  void *res;
  struct _t_resource *next;
} t_resource;

t_resource *g_res = NULL;

int MAIN (int argc, char **argv);

int nb_blocs;

int main (int argc, char **argv)
{
  int result;

  nb_blocs = 0;

  result = MAIN (argc, argv);

  //if (nb_blocs != 0)
    printf ("%d BLOCS NON DESALLOUES\n", nb_blocs);
  if (nb_blocs != 0)
  {
    printf("Dumping leaks:\n");
    t_resource *temp;
    for(temp = g_res; 
        temp; 
        temp = temp->next)
    {
      printf("  fuite trouvé pour l'allocation faite à la ligne %d.\n", temp->ligne);
    }
  }

  return result;
}

void *MALLOC (int n, int ligne)
{
  char *p;
  t_resource *temp;

  if (n < 0)
    {
      fprintf (stderr,
               "PARAMETRE NEGATIF PASSE A MALLOC A LA LIGNE %d\n",
	       ligne);
      exit (1);
    }
  
  p = (char*) malloc (n+sizeof (int));
  if (p == NULL)
    return NULL;

  temp = (t_resource *)malloc(sizeof(t_resource));
  if (!temp)
  {
    printf("Erreur malloc t_resource.\n");
    return NULL;
  }

  nb_blocs++;

  *(int*)p = -1-n;

  temp->ligne = ligne;
  temp->res   = (void*)(p+sizeof (int));
  if (!g_res)
  {
    temp->next = NULL;
  }
  else
  {
    temp->next = g_res;
  }
  g_res = temp;

  return g_res->res;
}

void *CALLOC (int n, int m, int ligne)
{
  char *p;

  if (n < 0 || m < 0)
    {
      fprintf (stderr,
	       "PARAMETRE NEGATIF PASSE A CALLOC A LA LIGNE %d\n",
               ligne);
      exit (1);
    }

  p = (char*) calloc (n*m+sizeof (int),1);
  if (p == NULL)
    return NULL;

  t_resource *temp = (t_resource *)malloc(sizeof(t_resource));
  if (!temp)
  {
    printf("Erreur malloc t_resource.\n");
    return NULL;
  }

  nb_blocs++;

  *(int*)p = -1-n*m;

  temp->ligne = ligne;
  temp->res   = (void*)(p+sizeof (int));
  if (!g_res)
  {
    temp->next = NULL;
  }
  else
  {
    temp->next = g_res;
  }
  g_res = temp;

  return g_res->res;

}

void FREE (void *ptr, int ligne)
{
  char *p;
  int n;

  if (ptr == NULL)
    {
      fprintf (stderr, "POINTEUR NUL PASSE A FREE A LA LIGNE %d\n", ligne);
      exit (1);
    }

  p = (char*)ptr - sizeof (int);

  n = -1-*(int*)p;

  if (n < 0)
    {
      fprintf (stderr,
	       "DONNEE DEJA DESALLOUEE PASSEE A FREE A LA LIGNE %d\n",
               ligne);
      exit (1);
    }

  t_resource *temp, *last = NULL;
  for(temp = g_res; 
      temp; 
      temp = temp->next)
  {
    if (temp->res == ptr)
    {
      if (!last)
        g_res = temp->next;
      else
        last->next = temp->next;
    
      *(int*)p = 0;
      free(p);
      free(temp);
      break;
    }
    last = temp;
  }

  nb_blocs--;


/*
  free (p);
*/
}

void *REALLOC(void *ptr, int size, int ligne)
{
  char *p;
  int n;

  if (ptr == NULL)
    {
      fprintf (stderr, "POINTEUR NUL PASSE A FREE A LA LIGNE %d\n", ligne);
      exit (1);
    }

  p = (char*)ptr - sizeof (int);

  n = -1-*(int*)p;

  if (n < 0)
    {
      fprintf (stderr,
	       "DONNEE DEJA DESALLOUEE PASSEE A FREE A LA LIGNE %d\n",
               ligne);
      exit (1);
    }

  t_resource *temp, *last = NULL;
  for(temp = g_res; 
      temp; 
      temp = temp->next) {
    if (temp->res == ptr) {
      p = realloc(p, size);
      if (p == NULL)
        return NULL;

      *(int*)p = -1-n;

      temp->ligne = ligne;
      temp->res   = (void*)(p+sizeof (int));

      return temp->res;  
    }
  }

  return NULL;

  
}

#define main MAIN
#define malloc(n) MALLOC(n,__LINE__)
#define calloc(n,m) CALLOC(n,m,__LINE__)
#define realloc(p,n) REALLOC(p,n,__LINE__)
#define free(p) FREE(p,__LINE__)

#endif

