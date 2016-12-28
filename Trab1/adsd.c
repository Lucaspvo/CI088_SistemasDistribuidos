/*
 * adsd.c
 *
 *  Created on: Apr 08, 2016
 *      Author: Alessandro Elias, GRR20110589
 */
/* --------------------------------------------------------------------------------------
   Programa que implementa o Tipo Abstrato de Dados Lista em Vetor
   Objetivo: ilustrar como fazer um log significativo
   Restricoes: o programa assuma que o usuario entrou com os valores corretos, inteiros.

   Autor: Prof. Elias P. Duarte Jr.
   Disciplina: Algoritmos II
   Data da ultima atualizacao: 2013/2
----------------------------------------------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "smpl.h"
#define _GNU_SOURCE
#include <getopt.h>


#define NUM_SIMULATIONS_SYNTHESIS		30

typedef int bool;

#ifndef TRUE
#define TRUE		1
#endif

#ifndef FALSE
#define FALSE		0
#endif

#define TEST		1
#define FAULT		2
#define REPAIR		3

typedef enum _status {
	st_unknown	 = -1,
	st_faultless = 0,
	st_fault	 = 1
} STATUS;

typedef enum _exec_mode {
	m_normal = 0,
	m_synthesis
} MODE;

typedef struct {
    int 	id;
    STATUS 	st;
    STATUS 	*tested_up;
    int 	nnodes_diagnosed;
} tnode;

typedef struct {
    tnode 	*node;
    int		nnodes;
    int 	nnodes_fault;
    MODE 	mode;
    float 	cycle_actual_time;
    int 	latency;
    int 	ntests;
} tdiagnostic;

static inline void print_header(tdiagnostic *);
void print_tested_up(tdiagnostic*);
void print_diagnose_synthesis(tdiagnostic*);
void print_diagnose(tdiagnostic*);
bool Is_EndOfDiagnose(tdiagnostic*);
static inline bool is_end_of_cycle(tdiagnostic*);
tdiagnostic *init_diagnose(int, int, int);
void reset_tested_up(tdiagnostic*);
void init_randon(int);
int get_random_node(void);
static inline void reset_random(int);
void init_randon(int);
void schedule_events(tdiagnostic*);
static void print_help(char *argv[]);
static void parse_cmdline(int, char*[], MODE*, int*, int*, int*);
void destroy_diagnose(tdiagnostic*);

double g_sum_latency = 0, g_sum_test = 0;
double g_sd_sum_latency = 0, g_sd_sum_test = 0;
int g_last_node;
int *g_rnd_vector;



//#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
//#pragma GCC diagnostic ignored "-Wimplicit-function-declaration"
//#pragma GCC diagnostic ignored "-Wimplicit-int"
/*___________________________________________________________________________*/
int main(int argc, char *argv[])
{
    static char *str_code[2] = {"Sem falha", "Falho"};
    static int 	token, event, r, i, k, nxt_node;
    int num_simul, num_nodes, num_nodes_fault;
    tdiagnostic *d;
	MODE 		exec_mode;

    if( argc < 5 ) {
    	print_help(argv);
        exit(EXIT_FAILURE);
    }
    parse_cmdline(argc, argv, &exec_mode, &num_simul, &num_nodes, &num_nodes_fault);

    srand ( time(NULL) );

    for( k=0; k < num_simul; k++) {
		smpl(0, "Programa Tempo");
		reset();
		stream(1);
		// Incializada diagnostico
		d = init_diagnose(num_nodes, num_nodes_fault, exec_mode);
		init_randon(d->nnodes);

		print_header(d);
		schedule_events(d);
		cause(&event, &token);
		while( !Is_EndOfDiagnose(d) ) {
			switch(event) {
			case TEST:
				if( status(d->node[token].id) != 0 )	// 1 - facility is busy
					break;
				printf("Sou o nodo %d, vou testar no tempo %5.1f\n", token, time());
				reset_random(d->nnodes);
				//node[token].tested_up[token] = st_faultless;
				if( d->node[token].st == st_faultless ) {
					nxt_node = token;
					while(1) {
						//nxt_node = (nxt_node + 1) % d->nnodos;
						nxt_node = get_random_node();
						//printf("nxt_node = %d\n", nxt_node); fflush(stdout);
						if( nxt_node == token ) {
							if( d->node[token].nnodes_diagnosed == d->nnodes )
								break;
							continue;
						}

						printf("\tO nodo %d testou o nodo %d como %s\n",\
								token, nxt_node, str_code[d->node[nxt_node].st]);
						++d->node[token].nnodes_diagnosed;
						++d->ntests;
						d->node[token].tested_up[nxt_node] = d->node[nxt_node].st;
						if (d->node[nxt_node].st == st_faultless )
							break;
					}
					d->node[token].tested_up[nxt_node] = st_faultless;
				}
				/*
				 * Tester gets information about the diagnostic, except about those nodes that
				 * have been tested at the same interval of tests.
				 */
				for( i=0; i < d->nnodes; i++ ) {
					if( d->node[token].tested_up[i] == st_unknown &&\
						d->node[nxt_node].tested_up[i] != st_unknown ) {
						d->node[token].tested_up[i] = d->node[nxt_node].tested_up[i];
						++d->ntests;
					}
				}
				schedule(TEST, 30.0, token);
			break;

			case FAULT:
				r = request(d->node[token].id, token, 0);
				d->node[token].st = st_fault;
				if( r != 0 ) {
					puts("Não consegui falhar o nodo...\n");
					exit(EXIT_FAILURE);
				}
				printf("Sou o nodo %d, falhei no tempo %5.1f\n", token, time());
			break;

			case REPAIR:
				release(d->node[token].id, token);
				d->node[token].st = st_faultless;
				printf("Sou o nodo %d, recuperei no tempo %5.1f\n", token, time());
				schedule(TEST, 30.0, token);
			break;
			}

			cause(&event, &token);
		}

		if( exec_mode == m_synthesis ) {
			g_sum_latency 	+= d->latency;
			g_sum_test		+= d->ntests;
			g_sd_sum_latency+= d->latency * d->latency;
			g_sd_sum_test 	+= d->ntests * d->ntests;
		}

		print_diagnose(d);

	}

	if( exec_mode == m_synthesis )
		print_diagnose_synthesis(d);

	return EXIT_SUCCESS;
}

void destroy_diagnose(tdiagnostic *d)
{
	free(d->node);
}

static void parse_cmdline(int argc, char *argv[], MODE *exec_mode, int *num_simul, int *nodes, int *nodes_fault)
{
	int opt;
	struct option long_opts[] = {
			{"normal", 0, NULL, 'n'},
			{"sintese", 0, NULL, 's'},
			{"nodes", 1, NULL, 'o'},
			{"fault", 1, NULL, 'f'},
			{0, 0, 0, 0}
	};

	*exec_mode = -1;
	*nodes = -1;
	*nodes_fault = -1;
	*num_simul = -1;
	while( (opt = getopt_long(argc, argv, ":nso:f:", long_opts, NULL)) != -1 ) {
		switch( opt ) {
		case 'n':
			*exec_mode = m_normal;
	    	*num_simul = 1;
		break;

		case 's':
			*exec_mode = m_synthesis;
	    	*num_simul = NUM_SIMULATIONS_SYNTHESIS;
		break;

		case 'o':
			*nodes = atoi(optarg);
		break;

		case 'f':
			*nodes_fault = atoi(optarg);
		break;

		case ':':
			print_help(argv);
			exit(EXIT_FAILURE);
		break;

		case '?':
			fprintf(stderr, "Opcao desconhecida '%c'\n", optopt);
			exit(EXIT_FAILURE);
		break;
		}
	}
}

static void print_help(char *argv[])
{
    fprintf(stderr, "Uso correto: %s\n", (char*)(strrchr(argv[0], '/')+1));
    fprintf(stderr, "\t[-n | --normal | -s | --sintese]\n");
    fprintf(stderr, "\t-o | --nodes <numero de nodos>\n");
    fprintf(stderr, "\t-f | --fault <numero de nodos falhos>\n");
}

static inline void print_header(tdiagnostic *d)
{
	static const char *str_mode[] = { "Modo normal", "Modo sintese" };
	puts("=============================================================================");
	puts("Inicio da execucao: programa que implementa ADSD.");
	puts("Prof. Elias P. Duarte Jr.  -  Disciplina Sistema Distribuido.");
	printf("%s\n", str_mode[d->mode]);
	printf("INICIO DIAGNOSTICO: TEMPO = %5.1f\n", d->cycle_actual_time);
	puts("=============================================================================");
	fflush(stdout);

}

void print_tested_up(tdiagnostic *d)
{
    int i, j;

    for( j=0; j < d->nnodes; j++) {
        if( d->node[j].st == st_faultless ) {
            printf("TESTED_UP[%d] = [ ", j);
            for( i=0; i < d->nnodes; i++ ) {
            	switch( d->node[j].tested_up[i] ) {
					case st_unknown: 	printf("* "); break;
					case st_faultless:	printf("S "); break;
					case st_fault: 		printf("F "); break;
            	}
            }
            puts("]");
        }
    }
    fflush(stdout);
}

void print_diagnose(tdiagnostic *d) {
	puts("=============================================================================");
    puts("TERMINO DIAGNOSTICO");
	puts("=============================================================================");
    printf("Latencia diagnostico = %d\n", d->latency);
    printf("Testes realizados = %d\n", d->ntests);
    printf("\n\n");
}

void print_diagnose_synthesis(tdiagnostic *d) {
    double average_latency 	= g_sum_latency / NUM_SIMULATIONS_SYNTHESIS;
    double average_tests 	= g_sum_test / NUM_SIMULATIONS_SYNTHESIS;
    double sd_latency		= sqrt( (g_sd_sum_latency / NUM_SIMULATIONS_SYNTHESIS) -\
    								(average_latency * average_latency) );
    double sd_tests 		= sqrt( (g_sd_sum_test / NUM_SIMULATIONS_SYNTHESIS) -\
    								(average_tests * average_tests) );
	puts("=============================================================================");
    puts("TERMINO 30 SIMULACOES");
	puts("=============================================================================");

    printf("Media Latencia diagnostico = %f\n", average_latency);
    printf("Desv padrao Latencia diagnostico = %f\n", sd_latency);
    printf("Media testes diagnostico = %f\n", average_tests);
    printf("Dev padrao testes diagnostico = %f\n", sd_tests);
    fflush(stdout);
}

static inline bool is_end_of_cycle(tdiagnostic *d)
{
	if( d->cycle_actual_time < time() ) {
		d->cycle_actual_time = time();
		return TRUE;
	}

	return FALSE;
}

bool Is_EndOfDiagnose(tdiagnostic *d)
{
    int i;

    if( is_end_of_cycle(d) ) {
        for( i=0; i < d->nnodes; i++){
            if( d->node[i].st == st_faultless && d->node[i].nnodes_diagnosed < d->nnodes ) {
                ++d->latency;
                print_tested_up(d);
            	puts("=============================================================================");
                printf("INICIO RODADA: TEMPO = %5.1f\n", d->cycle_actual_time);
            	puts("=============================================================================");

                return FALSE;
            }
        }

        print_tested_up(d);
        return TRUE;
    }

    return FALSE;
}

tdiagnostic *init_diagnose(int nnodes, int nnodes_fault, int exec_mode )
{
    int i, j;
    static char fa_name[5];

    tdiagnostic *d = (tdiagnostic*)malloc(sizeof(tdiagnostic));
    tnode *node = (tnode*)malloc(sizeof(tnode) * nnodes);

    d->nnodes = nnodes;
    d->nnodes_fault = nnodes_fault;
    d->mode = exec_mode;
    d->latency = 0;
    d->ntests = 0;
    d->cycle_actual_time = time();

    for( i=0; i < nnodes; i++ ) {
        memset(fa_name, '\0', 5);
        sprintf(fa_name, "%d", i);
        node[i].id = facility(fa_name, 1); 		//resource
        node[i].st = st_faultless;
        node[i].nnodes_diagnosed = 1;
        node[i].tested_up = (STATUS *)malloc(sizeof(STATUS) * nnodes);
        for( j=0; j < nnodes;j++ )
            node[i].tested_up[j] = st_unknown;
        node[i].tested_up[i] = st_faultless;
    }
    d->node = node;

    return d;
}

void reset_tested_up(tdiagnostic *d)
{
    int i, j;

    for(i=0; i < d->nnodes; i++ ) {
        d->node[i].st= st_faultless;
        d->node[i].nnodes_diagnosed = 1;
        for (j = 0; j < d->nnodes; j++){
            d->node[i].tested_up[j] = st_unknown;
        }
        d->node[i].tested_up[i] = st_faultless;
    }
}

void init_randon(int size)
{
    int i;

    g_rnd_vector = (int*)malloc(sizeof(int) * size);
    for( i=0; i < size; i++ )
    	g_rnd_vector[i] = i;
}

static inline void reset_random(int size)
{ g_last_node = size - 1; }

int get_random_node(void)
{
    int r, aux;

    r = rand() % (g_last_node + 1);
    //printf("%d\n", r);
    aux = g_rnd_vector[r];
    g_rnd_vector[r] = g_rnd_vector[g_last_node];
    g_rnd_vector[g_last_node] = aux;
    --g_last_node;

    return aux;
}

void schedule_events(tdiagnostic *d)
{
    int i, r;

    for( i=0; i < d->nnodes; i++ )
        schedule(TEST, 30.0, i);

    reset_random(d->nnodes);

    // falha num_nodos_falho nodos
    for(i=0; i < d->nnodes_fault; i++ ) {
        r = get_random_node();
        schedule(FAULT, 0, r);
        //printf("rand = %d\n", r);
    }

    // schedule(fault, 0, 2);
    // schedule(fault, 0, 3);
    // schedule(fault, 0, 4);
}

//#pragma GCC diagnostic pop
