#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>

#define TYPESIZE 64
#define GENS 100

#define PADDING 2 //Edge padding for now 

#define ARRAY_HEIGHT 20
#define ARRAY_WIDTH  75
#define WORDS  ((ARRAY_WIDTH + TYPESIZE - 1) / TYPESIZE)
#define STRIDE (WORDS + PADDING) //grille avec padding


typedef struct {
    uint64_t un;
    uint64_t deux;
    uint64_t quatre;
} v;



void additionner_voisins(uint64_t p, v *c){

    uint64_t r1 = 0;
    uint64_t r2 = 0;

    r1 = c->un & p;
    c->un = c->un ^ p;

    r2 = c->deux & r1;
    c->deux = c->deux ^ r1;

    c->quatre |= r2;

}

uint64_t comparer8voisins(uint64_t *p){

    v c = {0}; //compteur

    // << va vers les poids fort >> va vers les poids faibles

    uint64_t top = *(p + STRIDE);
    uint64_t bottom = *(p - STRIDE);

    uint64_t top_left = *(p-1 + STRIDE)>>63 | top << 1;
    uint64_t top_right = top >> 1 | *(p+1 + STRIDE)<<63;

    uint64_t bottom_left = *(p-1 - STRIDE)>>63 | bottom << 1;
    uint64_t bottom_right = bottom >> 1 | *(p+1 - STRIDE)<<63;    

    uint64_t left = *(p-1)>>63 | *p<<1; // ABCD << 1 | WYXZ >> 3 =  0ABC | Z000 = ZABC
    uint64_t right = *p>>1 | *(p+1)<<63;

    additionner_voisins(top, &c);
    additionner_voisins(bottom, &c);

    additionner_voisins(top_left, &c);
    additionner_voisins(top_right, &c);

    additionner_voisins(bottom_left, &c);
    additionner_voisins(bottom_right, &c);

    additionner_voisins(left, &c);
    additionner_voisins(right, &c);

    uint64_t next = c.deux & ~c.quatre & (c.un | *p); //si deux = 1 et que 4 = 0 on est à 2 ou 3 voisins 
                                                      //si un = 1 alors on est à 3 --> cellule né ou survit
                                                      //si un = 0 alors on est à 2, si *p (le bit sur lequel on est) = 1 alors on survit (règles des 2 voisins)
                                                      
    return next;
}

int main()
{

    //affichage
    printf("\033[2J");          // une fois au début : efface l'écran

    static uint64_t xLocal = 0; //:D
    uint64_t *p = calloc((ARRAY_HEIGHT + PADDING) * STRIDE, sizeof(uint64_t));
    uint64_t *q = calloc((ARRAY_HEIGHT + PADDING) * STRIDE, sizeof(uint64_t));

    // coordonnées y * word + x pour atterir dans la case (x,y) (ici on utilise stride car il y'a un padding mais c'est la meme)
    p[1 * STRIDE + 1] = 0x2;   
    p[2 * STRIDE + 1] = 0x4;   
    p[3 * STRIDE + 1] = 0x7;   

    //claude help
    uint64_t mask = (ARRAY_WIDTH % TYPESIZE) ? (1ULL << (ARRAY_WIDTH % TYPESIZE)) - 1 : ~0ULL; // on récup le nombre de bit libre sur la dernière colonne

    for(int g = 0; g < GENS; g++){

        printf("\033[H");       // remet le curseur en haut à gauche


        for(uint64_t y = 1; y <= ARRAY_HEIGHT; y++){ //je suis parti sur un traitement 2D de notre tableau 1D au final avec 2 boucles for plutot que une grosse
            for(uint64_t x = 1; x <= WORDS; x++){ //on commence à 1 pour le padding edge 

                for(uint64_t j = 0; j < TYPESIZE; j ++){

                    if(xLocal < ARRAY_WIDTH) //on print les bits uniquement et pas les octets
                    {
                        p[y * STRIDE + x]>>j & 1 ? printf("*") : printf(".");
                        xLocal++;
                    }
                }
            }

            printf("\n");
            xLocal = 0;
        }
        fflush(stdout);

        for (int y = 1; y <= ARRAY_HEIGHT; y++) {
            for (int x = 1; x <= WORDS; x++){
                q[y * STRIDE + x] = comparer8voisins(&p[y * STRIDE + x]); //on recopie le mot dans le deuxieme array
            }
            q[y * STRIDE + WORDS] &= mask;//on applique le mask sur la derniere colonne (on force le reste à 0)
        }

        uint64_t *tmp = p; p = q; q = tmp; //we need 2 arrays so we don't mix 2 generations 
    }

 


    free(p);
    free(q);
    return 0;
}
