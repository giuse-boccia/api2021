/********************************************************************************************************************************************
*                                                                                                                                           *
*           Progetto di API 2020/21             *           Giuseppe Boccia             *                   10716235                        *
*                                                                                                                                           *
*********************************************************************************************************************************************
*                                                                                                                                           *
*   Di seguito una breve write-up del progetto:                                                                                             *
*                                                                                                                                           *
*   INPUT e MEMORIZZAZIONE                                                                                                                  *
*   Ho implementato l'input con la funzione getc_unlocked(): vengono lette le singole cifre aggiornando una variabile temporanea.           *
*   Quando viene trovata una virgola o il carattere new-line il valore temporaneo viene memorizzato nella struttura definitiva.             *
*   Come specificato dai professori, ho supposto che l'input fornito al programma sia sempre corretto.                                      *
*   I pesi degli archi vengono memorizzati in una matrice di uint32_t in quanto sono interi positivi strettamente minori di 2^32            *
*   I punteggi dei grafi e le distanze provvisorie dei singoli vertici sono memorizzati in uint64_t in quanto potrebbero                    *
*   essere valori maggiori di 2^32 - 1, ma comunque strettamente minori di 2^64.                                                            *
*                                                                                                                                           *
*   PUNTEGGIO                                                                                                                               *
*   Ho implementato l'algoritmo di Djikstra per calcolare il punteggio dei singoli grafi. La mia implementazione utilizza un array          *
*   come coda di priorità, per questo extract_min ha complessità O(n). Usando un min-heap questa complessità potrebbe                       *
*   diminuire, tuttavia il guadagno sarebbe apprezzabile solamente con un grafo "abbastanza sparso" (Cormen pag. 555).                      *
*   Sempre secondo il Cormen l'implementazione con heap di Fibonacci è la più efficente, ma non è stata necessaria ai fini del progetto.    *
*                                                                                                                                           *
*   CLASSIFICA                                                                                                                              *
*   La classifica è stata implementata con una lista doppiamente concatenata ordinata per score decrescente con al più k elementi.          *
*   All'inserimento del k+1 esimo elemento si elimina quello con il punteggio più alto, cioè il primo.                                      *
*   L'inserimento avviene cercando in parallelo sia dalla testa che dalla coda il punto in cui inserire il nuovo nodo, questo               *
*   perché negli "stress test" dell'implementazione di TopK i grafi vengono forniti con score crescente o decrescente, quindi la            *
*   maggior parte degli inserimenti avviene o in testa o in coda.                                                                           *
*   Nel caso pessimo, la complessità dell'inserimento è O(n), mentre si avrebbe complessità O(log n) implementando                          *
*   la classifica con max-heap oppure un BST autobilanciante, per esempio un albero rosso-nero.                                             *
*   L'implementazione con rb-tree non è stata necessaria ai fini del progetto.                                                              *                                                                                                              *
*                                                                                                                                           *
*********************************************************************************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#define BASE 10
#define INPUT_MAXLEN 15         // Massima lunghezza del comando in input (es: AggiungiGrafo, TopK)

// Elemento lista doppiamente concatenata (per topk)
typedef struct top_list_s {
    int idx;                        // Indice del grafo
    uint64_t score;                 // Punteggio del grafo
    struct top_list_s *next;       // Puntatore all'elemento successivo
    struct top_list_s *precedente; // Puntatore all'elemento precedente
}top_list_t;

// Restituisce l'indice dell'elemento appartenente a Q con distanza minore (dist[i]=0 --> i è irraggiungibile oppure i=0)
int extract_min(int dim, uint64_t dist[dim], bool Qlist[dim]) {
    int i, ris, min_dist;
    ris = -1;
    min_dist = 0;

    for (i = 1; i < dim && !min_dist; i++) {
        if (Qlist[i] && dist[i]) {
            ris = i;
            min_dist = dist[i];
        }
    }

    while (i < dim) {
        if (Qlist[i] && dist[i] && dist[i] < min_dist) {
            ris = i;
            min_dist = dist[i];
        }
        i++;
    }
    if (ris > 0) {
        Qlist[ris] = false;
    }
    return ris;     // restituisce -1 se non ci sono nodi raggiungibili
}

// Riceve la matrice di adiacenza di un grafo e restituisce il punteggio
uint64_t valuta_grafo(int dim, uint32_t pesi[][dim], uint64_t dist[dim], bool Qlist[dim]) {
    int i, curr;
    uint64_t score;
    int Qdim;       // Dimensione della coda di priorità

    // Inizializzazione
    for (i = 0; i < dim; i++) {
        dist[i] = 0;
        Qlist[i] = true;
    }
    Qdim = dim;

    // Prima passata di Djikstra (fuori ciclo)
    for (i = 1; i < dim; i++) {
        if (pesi[0][i]) {
            dist[i] = pesi[0][i];
        }
    }
    Qlist[0] = false;
    Qdim--;

    // Restanti iterazioni di Djikstra
    curr = extract_min(dim, dist, Qlist);
    while (Qdim > 0 && curr > 0) {
        Qdim--;
        for (i = 1; i < dim; i++) {
            if (pesi[curr][i] && Qlist[i] && (!dist[i] || dist[i] > dist[curr] + pesi[curr][i])) {
                dist[i] = dist[curr] + pesi[curr][i];
            }
        }
        curr = extract_min(dim, dist, Qlist);
    }

    // Calcolo score
    score = 0;
    for (i = 1; i < dim; i++) {
        score += dist[i];
    }

    return score;
}

// Acquisisce la matrice di adiacenza di un nuovo grafo, sovrascrivendo la precedente
void nuovo_grafo(int dim, uint32_t grafo[][dim]) {
    int i, j, carattere;
    uint32_t numero;

    for (i = 0; i < dim; i++) {
        for (j = 0; j < dim; j++) {
            numero = 0;
            carattere = getc_unlocked(stdin);
            while (carattere != (int)',' && carattere != (int)'\n') {
                numero = (numero * BASE) + carattere - (int)'0';
                carattere = getc_unlocked(stdin);
            }
            grafo[i][j] = numero;
        }
    }
    return;
}

// Visualizza gli indici dei migliori k grafi fino ad ora (score più basso)
void visualizza_topk(top_list_t *h) {
    top_list_t *p;

    for (p = h; p != NULL; p = p->next) {
        if (p->next == NULL) {
            printf("%d", p->idx);
        } else {
            printf("%d ", p->idx);
        }
    }
    printf("\n");
    return;
}

/* Inserisce grafo in ordine decrescente per punteggio
*   Ci sono 4 casi possibili:
*       1. La lista è vuota (list_dim = 0)
*       2. La lista è parzialmente vuota (list_dim < k)
*       3. La lista è piena e l'elemento va inserito (list_dim == k && score < top_score)
*       4. La lista è piena e l'elemento non va inserito (list_dim == k && score >= top_score)
*/
top_list_t *inserisci_grafo(top_list_t *h, int idx, uint32_t score, int k) {
    top_list_t *prec, *succ, *new, *tmp, *p, *q;
    static top_list_t *last = NULL;
    static int list_dim = 0;                    // dimensione della lista
    uint32_t top_score;
    bool stop, forward;

    // Caso 0 (k = 0, non so neanche se sia possibile che succeda)
    if (k == 0) {
        return h;
    }

    // Caso 1 (solo primo inserimento)
    if (list_dim == 0) {
        new = malloc(sizeof(top_list_t));
        if (new == NULL) {
            printf("Errore MALLOC in inserisci_grafo()\n");
            return h;
        }
        list_dim++;
        h = new;
        new->idx = idx;
        new->score = score;
        new->next = NULL;
        new->precedente = NULL;
        last = h;

        return h;
    }

    top_score = h->score;

    // Caso 4
    if (list_dim == k && score >= top_score) {
        return h;
    }

    // Casi 2 e 3
    new = malloc(sizeof(top_list_t));
    if (new == NULL) {
        printf("Errore MALLOC in inserisci_grafo()\n");
        return h;
    }
    new->idx = idx;
    new->score = score;
    new->next = NULL;
    new->precedente = NULL;

    succ = NULL;
    prec = NULL;
    p = h;      // scorre la lista dalla testa
    q = last;   // scorre la lista dalla coda

    stop = false;
    forward = true;

    while (!stop) {
        if (forward) {
            if (p->score <= score) {
                stop = true;
                prec = p->precedente;
                succ = p;
            } else {
                p = p->next;
                forward = false;
            }
        } else {
            if (q->score > score) {
                stop = true;
                prec = q;
                succ = q->next;
            } else {
                q = q->precedente;
                forward = true;
            }
        }
    }

    if (prec && succ) {       // Inserimento nè in testa nè in coda
        prec->next = new;
        succ->precedente = new;
        new->next = succ;
        new->precedente = prec;
    } else if (succ) {         // Inserimento in testa (new è la nuova testa)
        new->next = h;
        h->precedente = new;
        h = new;
    } else {                  // inserimento in coda (new è il nuovo last)
        new->precedente = prec;
        prec->next = new;
        last = new;
    }

    // Caso 3 (se la lista era piena rimuove il primo elemento (quello con lo score più alto))
    if (list_dim == k) {
        tmp = h;
        h = h->next;
        free(tmp);
    } else {    // Caso 2
        list_dim++;
    }

    return h;
}

// Legge la prossima riga di input (eventualmente rimuove il carattere "new-line")
void leggi_riga(char *riga, int max_len) {

    if (fgets(riga, max_len, stdin)) {
        riga[strcspn(riga, "\n")] = '\0';     // se ho letto qualcosa tolgo il '\n'
    }

    return;
}

int main(int argc, char *argv[]) {
    int d = 0;
    int k = 0;
    int graph_idx = 0;
    uint64_t score;
    top_list_t *head = NULL;
    char comando[INPUT_MAXLEN + 1];

    leggi_riga(comando, INPUT_MAXLEN);      // La prima riga dell'input dovrebbe contere d e k separati da uno spazio

    d = atoi(strtok(comando, " "));
    k = atoi(strtok(NULL, " "));

    // Le seguenti strutture servono a Djikstra ma vengono dichiarate qui per essere allocate solo una volta
    uint32_t grafo[d][d];   // Matrice di adiacenza del grafo
    uint64_t dist[d];       // Array delle distanze dei vertici
    bool Qlist[d];          // Appartenenza del vertice alla coda di priorità

    leggi_riga(comando, INPUT_MAXLEN);

    while (!feof(stdin)) {

        if (!strcmp(comando, "AggiungiGrafo")) {
            nuovo_grafo(d, grafo);
            score = valuta_grafo(d, grafo, dist, Qlist);
            head = inserisci_grafo(head, graph_idx, score, k);
            graph_idx++;

        } else if (!strcmp(comando, "TopK")) {
            visualizza_topk(head);

        } else {
            printf("Comando non valido\n");
        }

        leggi_riga(comando, INPUT_MAXLEN);

    }
    return 0;
}