#include <stdio.h>
#include <stdlib.h>
#include "smpl.h"

#define recv     1
#define send     2
#define enter_rc 3
#define free_rc  4
#define reply    5

typedef struct {
	int id;
	int msgs_sent;
	int count;
	int local_timestamp;
	int timestamp1;
	int timestamp2;
	int requester1;
	int requester2;
	int pending[3];
	int reply1;
	int reply2;
}tnodo;
tnodo * nodo;

/*
	Função usada como SEED da função srand()
*/
unsigned long long rdtsc(){
    unsigned int lo,hi;
    __asm__ __volatile__ ("rdtsc" : "=a" (lo), "=d" (hi));
    return ((unsigned long long)hi << 32) | lo;
}

/*
	Função Finaliza Programa - Print
*/
void finaliza_execucao(){
	puts("==========================================================================");
	puts("=                       Finalização do Programa                          =");
	puts("=            Todos os processos executaram a Região Crítica              =");
	puts("==========================================================================");
}

/*
	Função Inicializa Programa - Print
*/
void inicio_execucao(char* argv[], int N){
	puts  ("==========================================================================");
	puts  ("=                          Programa Iniciado                             =");
	printf("=                        Executará com %d nodos:                          =\n", N);
	int j = 2, i;
	for (i = 0; i < N; ++i){
		printf("=                   Nodo %d enviará REQUEST no tempo %s                   =\n", i, argv[j]);
		j++;
		j++;
	}
	puts  ("==========================================================================");
}

main(int argc, char* argv[]){
	static int N, token, event, r, i, rand_time, recv_request_from_node, token_sent_request, timestamp;
	static char fa_name[3];

	if(argc != 5 && argc != 7){
		puts("Uso correto: tempo <nodo1> <tempo-nodo1> <nodo2> <tempo-nodo2> <nodo3> <tempo-nodo3>");
		puts("<nodo3> é opcional");
		exit(1);
	}

	N=(argc-1)/2;
	smpl(0, "Programa Tempo");
	reset();
	stream(1);
	srand(rdtsc());


	/*
		Inicializa cada nodo
	*/
	nodo=(tnodo*) malloc(sizeof(tnodo)*N);
	for(i=0; i<N; i++){
		memset(fa_name, '\0', N);
		sprintf(fa_name, "%d", i);
		nodo[i].id = facility(fa_name, 1);
		nodo[i].msgs_sent = 0;
		nodo[i].local_timestamp = 0;
		nodo[i].timestamp1 = 0;
		nodo[i].timestamp2 = 0;
		nodo[i].requester1 = -1;
		nodo[i].requester2 = -1;
		nodo[i].reply1 = -1;
		nodo[i].reply2 = -1;
	}

	/*
		Escalona REQUEST dos nodos passados como parâmetro ao programa
	*/
	int schd = 2;
	for(i=0; i<N; i++){
		schedule(send, atof(argv[schd]), i);
		schd = schd + 2;
	}

	inicio_execucao(argv, N);

	int c = 0;
	while(c < N){
		cause(&event, &token);
		switch(event){
			case recv:
				if(N == 2){ //Executa para N = 2
					if(status(nodo[token].id) == 0){
						recv_request_from_node = nodo[token].requester1;
						printf("Sou o nodo %d, recebi uma requisição do nodo %d no tempo %5.1lf\n", (token)%N, recv_request_from_node, time());
						if(nodo[token].local_timestamp == 0){
							printf("Sou o nodo %d, enviei um reply para nodo %d no tempo %5.1lf\n", (token)%N, recv_request_from_node, time());
							schedule(reply, (rand() % 5)+1, recv_request_from_node);
							nodo[recv_request_from_node].reply1 = token;
						} else if(nodo[token].local_timestamp < nodo[token].timestamp1){
							nodo[token].pending[recv_request_from_node] = 1;
							nodo[token].requester1 = -1;
							nodo[token].timestamp1 = 0;
						} else if(nodo[token].local_timestamp == nodo[token].timestamp1){
							if(token < recv_request_from_node){
								nodo[token].pending[recv_request_from_node] = 1;
								nodo[token].requester1 = -1;
								nodo[token].timestamp1 = 0;
							} else {
								printf("Sou o nodo %d, enviei um reply para nodo %d no tempo %5.1lf\n", (token)%N, recv_request_from_node, time());
								schedule(reply, (rand() % 5)+1, recv_request_from_node);
								nodo[recv_request_from_node].reply1 = token;
								nodo[token].requester1 = -1;
								nodo[token].timestamp1 = 0;
							}
						} else if(nodo[token].local_timestamp > nodo[token].timestamp1){
							printf("Sou o nodo %d, enviei um reply para nodo %d no tempo %5.1lf\n", (token)%N, recv_request_from_node, time());
							schedule(reply, (rand() % 5)+1, recv_request_from_node);
							nodo[recv_request_from_node].reply1 = token;
							nodo[token].requester1 = -1;
							nodo[token].timestamp1 = 0;
						}
					} else {
						recv_request_from_node = nodo[token].requester1;
						printf("Sou o nodo %d, recebi uma requisição do nodo %d no tempo %5.1lf\n", (token)%N, recv_request_from_node, time());
						nodo[token].pending[recv_request_from_node] = 1;
						nodo[token].requester1 = -1;
						nodo[token].timestamp1 = 0;
					}
				} else { //Executa para N = 3
					if(status(nodo[token].id) == 0){
						if(nodo[token].timestamp1 != 0){
							recv_request_from_node = nodo[token].requester1;
							timestamp = nodo[token].timestamp1;
						} else {
							recv_request_from_node = nodo[token].requester2;
							timestamp = nodo[token].timestamp2;
						}
						
						printf("Sou o nodo %d, recebi uma requisição do nodo %d no tempo %5.1lf\n", (token)%N, recv_request_from_node, time());
						if(nodo[token].local_timestamp == 0){
							printf("Sou o nodo %d, enviei um reply para nodo %d no tempo %5.1lf\n", (token)%N, recv_request_from_node, time());
							schedule(reply, (rand() % 5)+1, recv_request_from_node);
							if(nodo[recv_request_from_node].reply1 == -1)
								nodo[recv_request_from_node].reply1 = token;
							else
								nodo[recv_request_from_node].reply2 = token;
							if(recv_request_from_node == nodo[token].requester1){
								nodo[token].requester1 = -1;
								nodo[token].timestamp1 = 0;
							} else {
								nodo[token].requester2 = -1;
								nodo[token].timestamp2 = 0;
							}
						} else if(nodo[token].local_timestamp < timestamp){
							nodo[token].pending[recv_request_from_node] = 1;
							if(recv_request_from_node == nodo[token].requester1){
								nodo[token].requester1 = -1;
								nodo[token].timestamp1 = 0;
							} else {
								nodo[token].requester2 = -1;
								nodo[token].timestamp2 = 0;
							}
						} else if(nodo[token].local_timestamp == timestamp){
							if(token < recv_request_from_node){
								nodo[token].pending[recv_request_from_node] = 1;
								if(recv_request_from_node == nodo[token].requester1){
									nodo[token].requester1 = -1;
									nodo[token].timestamp1 = 0;
								} else {
									nodo[token].requester2 = -1;
									nodo[token].timestamp2 = 0;
								}
							} else {
								printf("Sou o nodo %d, enviei um reply para nodo %d no tempo %5.1lf\n", (token)%N, recv_request_from_node, time());
								schedule(reply, (rand() % 5)+1, recv_request_from_node);
								if(nodo[recv_request_from_node].reply1 == -1)
									nodo[recv_request_from_node].reply1 = token;
								else
									nodo[recv_request_from_node].reply2 = token;
								if(recv_request_from_node == nodo[token].requester1){
									nodo[token].requester1 = -1;
									nodo[token].timestamp1 = 0;
								} else {
									nodo[token].requester2 = -1;
									nodo[token].timestamp2 = 0;
								}
							}
						} else if(nodo[token].local_timestamp > timestamp){
							printf("Sou o nodo %d, enviei um reply para nodo %d no tempo %5.1lf\n", (token)%N, recv_request_from_node, time());
							schedule(reply, (rand() % 5)+1, recv_request_from_node);
							if(nodo[recv_request_from_node].reply1 == -1)
								nodo[recv_request_from_node].reply1 = token;
							else
								nodo[recv_request_from_node].reply2 = token;
							if(recv_request_from_node == nodo[token].requester1){
								nodo[token].requester1 = -1;
								nodo[token].timestamp1 = 0;
							} else {
								nodo[token].requester2 = -1;
								nodo[token].timestamp2 = 0;
							}
						}
					} else {
						if(nodo[token].timestamp1 != 0)
							recv_request_from_node = nodo[token].requester1;
						else
							recv_request_from_node = nodo[token].requester2;
						printf("Sou o nodo %d, recebi uma requisição do nodo %d no tempo %5.1lf\n", (token)%N, recv_request_from_node, time());
						nodo[token].pending[recv_request_from_node] = 1;
						if(recv_request_from_node == nodo[token].requester1){
							nodo[token].requester1 = -1;
							nodo[token].timestamp1 = 0;
						} else {
							nodo[token].requester2 = -1;
							nodo[token].timestamp2 = 0;
						}
					}
				}
				break;

			case send:
				if(N == 2){ //Executa para N = 2
					//Envio do request para o outro nodo
					rand_time = (rand() % 5)+1;
					token_sent_request = (token+1)%N;
					printf("Sou o nodo %d, enviei uma requisição para nodo %d no tempo %5.1lf\n", (token)%N, token_sent_request, time());
					schedule(recv, rand_time, token_sent_request);
					nodo[token_sent_request].timestamp1 = time();
					nodo[token_sent_request].requester1 = token;
					nodo[token].local_timestamp = time();
					nodo[token].msgs_sent++;
				} else { //Executa para N = 3
					//Envio do request para um dos dois nodos
					rand_time = (rand() % 5)+1;
					token_sent_request = (token+1)%N;
					printf("Sou o nodo %d, enviei uma requisição para nodo %d no tempo %5.1lf\n", (token)%N, token_sent_request, time());
					schedule(recv, rand_time, token_sent_request);
					if(nodo[token_sent_request].requester1 == -1){
						nodo[token_sent_request].timestamp1 = time();
						nodo[token_sent_request].requester1 = token;
					} else {
						nodo[token_sent_request].timestamp2 = time();
						nodo[token_sent_request].requester2 = token;
					}
					nodo[token].msgs_sent++;
					//Envio do request para o nodo restante
					rand_time = (rand() % 5)+1;
					token_sent_request = (token+2)%N;
					printf("Sou o nodo %d, enviei uma requisição para nodo %d no tempo %5.1lf\n", (token)%N, token_sent_request, time());
					schedule(recv, rand_time, token_sent_request);
					if(nodo[token_sent_request].requester1 == -1){
						nodo[token_sent_request].timestamp1 = time();
						nodo[token_sent_request].requester1 = token;
					} else {
						nodo[token_sent_request].timestamp2 = time();
						nodo[token_sent_request].requester2 = token;
					}
					nodo[token].local_timestamp = time();
					nodo[token].msgs_sent++;
				}
				break;

			case reply:
				if(nodo[token].reply1 != -1){
					printf("Sou o nodo %d, recebi Reply do nodo %d no tempo %5.1lf\n", (token)%N, nodo[token].reply1, time());
					nodo[token].reply1 = -1;
				} else if(nodo[token].reply2 != -1){
					printf("Sou o nodo %d, recebi Reply do nodo %d no tempo %5.1lf\n", (token)%N, nodo[token].reply2, time());
					nodo[token].reply2 = -1;
				}
				
				nodo[token].msgs_sent--;
				if(!nodo[token].msgs_sent){
					schedule(enter_rc, 0.0, token);
				}
				break;

			case enter_rc:
				r = request(nodo[token].id, token, 0);
				if(r != 0){
					puts("Não consegui entrar na região crítica...\n");
					exit(1);
				}
				printf("Sou o nodo %d, entrei na região crítica no tempo %5.1f\n", (token)%N, time());
				schedule(free_rc, (rand() % 5)+1, token);
				break;

			case free_rc:
				release(nodo[token].id, token);
				printf("Sou o nodo %d, sai da região crítica no tempo %5.1f\n", (token)%N, time());
				for (i = 0; i < N; ++i){
					if(nodo[token].pending[i] == 1){
						printf("Sou o nodo %d, enviei um reply para nodo %d no tempo %5.1lf\n", (token)%N, i, time());
						schedule(reply, (rand() % 5)+1, i);
						if(nodo[i].reply1 == -1)
							nodo[i].reply1 = token;
						else
							nodo[i].reply2 = token;
						if(i == nodo[token].requester1){
							nodo[token].requester1 = -1;
							nodo[token].timestamp1 = 0;
						} else {
							nodo[token].requester2 = -1;
							nodo[token].timestamp2 = 0;
						}
						nodo[token].pending[i] = 0;
					}
				}
				nodo[token].local_timestamp = 0;
				c++;
				break;
		}
	}
	finaliza_execucao();
}
/*!
 * \mainpage Trabalho 2 Sistemas Distribuidos (Algoritmo Ricart Agrawala Exclusão Mútua Distribuída)
 *
 * \section intro Introdução
 * <div align="justify"> O trabalho foi desenvolvido utilizando a linguagem C, e a biblioteca do SmPL, para o desenvolvimento do algoritmo de 
 *Exclusão Mútua Distribuída. Cada processo do sistema irá enviar um REQUEST a todos os demais para poder acessar a região crítica. Após várias 
 *trocas de mensagens com REPLIES de todos os outros processos, o processo requisitante irá entrar, executar e sair da região crítica.
 * </div>
 *
 * \section link Link Código Fonte
 * <a href="../mutexagrawala.c.txt" target="_blank">mutexagrawala.c.txt</a>
 *
 * \section sec1 Aspectos para Desenvolvimento
 * <div align="justify"> O desenvolvimento deste trabalho consistiu em, para cada processo do sistema, ao querer executar a região crítica, deve realizar alguns passos antes.
 * Inicialmente o processo requisitante deverá enviar uma mensagem de REQUEST a todos os demais processos do sistema. Esses processos ao receberem o REQUEST devem verificar
 * se não possuem uma requisição própria de maior prioridade que a requisição que receberam. Se possuirem requisição de maior prioridade, ou estiverem executando a região
 * crítica, deverão armazenar o REQUEST que receberam em um vetor de PENDENTES[], caso contrário enviam um REPLY ao processo requisitante. Ao receber o REPLY de TODOS os 
 * demais processos do sistema, o processo requisitante terá permissão de entrar na região crítica. Ao sair da região crítica, o processo percorre todo o vetor de PENDENTES[]
 * enviando um REPLY a todos os processos cujos IDs estejam com valor 1 no seu vetor de PENDENTES[].
 * </div>
 *
 * \section sec2 Modo de Execução do Programa
 * <div align="justify"> O programa pode ser executado para 2 ou 3 nodos.</div>
 *
 * \section sec3 LOG
 * <div align="justify"> Os logs possuem o seguinte formato: </div>
 * <ul>
 *  <li>Inicio Execução, apresenta os processos a executarem a Região Crítica assim como o tempo em que enviarão REQUEST aos demais processos</li>
 *  <li>"Corpo" da Execução, apresenta o processo que esta executando/escalonando o evento, assim como qual evento esta sendo realizado e por/para qual nodo esse evento foi escalonado </li>
 *  <li>Fim da Execução, finaliza o programa</li>
 * </ul>
 *
 * <div align="justify"> Abaixo seguem os LOGs e suas simulações demonstradas por imagens: </div>
 *
 * <div align="justify"> Legenda para as imagens </div>
 *
 * <img src="Events.png">
 *
 * <ul>
 *  <li><a href="log_n_o6_f2.txt" target="_blank">log N = 2 nodo1 = 0 tempoREQUEST = 10 nodo2 = 1 tempoREQUEST = 9</a>
 * 		<img src="LOGnodo0tempo10nodo1tempo9.png"><br>
 *  <li><a href="log_n_o12_f3.txt" target="_blank">log N = 2 nodo1 = 0 tempoREQUEST = 10 nodo2 = 1 tempoREQUEST = 20</a>
 * 		<img src="LOGnodo0tempo10nodo1tempo20.png"><br>
 *  <li><a href="log_s_o6_f2.txt" target="_blank">log N = 2 nodo1 = 0 tempoREQUEST = 20 nodo2 = 1 tempoREQUEST = 20</a>
 * 		<img src="LOGnodo0tempo20nodo1tempo20.png"><br>
 *  <li><a href="log_s_o12_f3.txt" target="_blank">log N = 2 nodo1 = 0 tempoREQUEST = 5 nodo2 = 1 tempoREQUEST = 5 nodo3 = 2 tempoRESQUEST = 5</a>
 * 		<img src="LOGn0t5n1t5n2t5.png"><br>
 *  <li><a href="log_s_o12_f3.txt" target="_blank">log N = 2 nodo1 = 0 tempoREQUEST = 10 nodo2 = 1 tempoREQUEST = 15 nodo3 = 2 tempoRESQUEST = 25</a>
 * 		<img src="LOGn0t10n1t15n2t25.png"><br>
 *  <li><a href="log_s_o12_f3.txt" target="_blank">log N = 2 nodo1 = 0 tempoREQUEST = 30 nodo2 = 1 tempoREQUEST = 5 nodo3 = 2 tempoRESQUEST = 13</a>
 * 		<img src="LOGn0t30n1t5n2t13.png">
 * </ul>
 *
 */
