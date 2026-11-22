/*
 * Semáforos em C (POSIX) - SO / IFRN - Dia 4, Parte B - Grupo 8
 * 1) Produtor-consumidor com buffer de tamanho 2 (ordem correta dos down)
 * 2) Deadlock: down(mutex) antes de down(vazio) / down(cheio)
 *
 * Compilar: gcc -Wall -Wextra -pthread -o semaforos main.c
 */
#define _POSIX_C_SOURCE 200809L
#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <errno.h>
#include <unistd.h>

#define N 2

static int buffer[N];
static int in_pos = 0, out_pos = 0;
static sem_t mutex, vazio, cheio;

static void sleep_ms(long ms) {
    struct timespec ts = { ms / 1000, (ms % 1000) * 1000000L };
    nanosleep(&ts, NULL);
}

/* DOWN com limite de tempo: retorna 0 se conseguiu, -1 se estourou o tempo */
static int down_timeout(sem_t *s, int segundos) {
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    ts.tv_sec += segundos;
    while (sem_timedwait(s, &ts) == -1) {
        if (errno == EINTR) continue;
        return -1;
    }
    return 0;
}

static void estado(const char *quem) {
    int m, v, c;
    sem_getvalue(&mutex, &m);
    sem_getvalue(&vazio, &v);
    sem_getvalue(&cheio, &c);
    printf("    [%s] mutex=%d vazio=%d cheio=%d\n", quem, m, v, c);
    fflush(stdout);
}

/* ---------- 1) Produtor-consumidor correto ---------- */
#define ITENS 6

static void *produtor(void *arg) {
    (void)arg;
    for (int i = 1; i <= ITENS; i++) {
        sem_wait(&vazio);              /* down(vazio): espera espaço */
        sem_wait(&mutex);              /* down(mutex): entra na região crítica */
        buffer[in_pos] = i;
        printf("P: inseriu item %d na posicao %d\n", i, in_pos);
        in_pos = (in_pos + 1) % N;
        estado("P");
        sem_post(&mutex);              /* up(mutex) */
        sem_post(&cheio);              /* up(cheio): sinaliza item */
        sleep_ms(200);                 /* produtor mais rapido: buffer enche */
    }
    return NULL;
}

static void *consumidor(void *arg) {
    (void)arg;
    for (int i = 1; i <= ITENS; i++) {
        sem_wait(&cheio);              /* down(cheio): espera item */
        sem_wait(&mutex);
        int item = buffer[out_pos];
        printf("C: removeu item %d da posicao %d\n", item, out_pos);
        out_pos = (out_pos + 1) % N;
        estado("C");
        sem_post(&mutex);
        sem_post(&vazio);              /* up(vazio): libera espaço */
        sleep_ms(600);                 /* consumidor lento */
    }
    return NULL;
}

static void init_sems(int vz, int ch) {
    sem_init(&mutex, 0, 1);
    sem_init(&vazio, 0, vz);
    sem_init(&cheio, 0, ch);
}

static void destroy_sems(void) {
    sem_destroy(&mutex);
    sem_destroy(&vazio);
    sem_destroy(&cheio);
}

static void demo_correta(void) {
    pthread_t p, c;
    in_pos = out_pos = 0;
    init_sems(N, 0);
    printf("\n=== Produtor-consumidor (buffer=%d, %d itens) ===\n", N, ITENS);
    estado("INICIO");
    pthread_create(&p, NULL, produtor, NULL);
    pthread_create(&c, NULL, consumidor, NULL);
    pthread_join(p, NULL);
    pthread_join(c, NULL);
    printf("Fim: todas as threads terminaram sem travar.\n");
    destroy_sems();
}

/* ---------- 2) Deadlock (ordem incorreta) ---------- */
#define TIMEOUT 3
#define TIMEOUT_P 5   /* produtor espera mais, para o consumidor mostrar o bloqueio dele */

static void *produtor_errado(void *arg) {
    (void)arg;
    sem_wait(&mutex);                                  /* down(mutex) PRIMEIRO (erro) */
    printf("P: pegou o mutex.\n");
    printf("P: down(vazio) com vazio=0... vai esperar segurando o mutex.\n");
    if (down_timeout(&vazio, TIMEOUT_P) == -1)
        printf("P: BLOQUEADO ha %ds esperando 'vazio' (segura o mutex) -> DEADLOCK\n", TIMEOUT_P);
    else
        sem_post(&vazio);
    sem_post(&mutex);                                  /* libera so para encerrar a demo */
    return NULL;
}

static void *consumidor_errado(void *arg) {
    (void)arg;
    sleep_ms(500);                                     /* garante que P pegou o mutex antes */
    sem_wait(&cheio);                                  /* cheio=2 -> passa */
    printf("C: down(cheio) ok. Agora down(mutex)...\n");
    if (down_timeout(&mutex, TIMEOUT) == -1)
        printf("C: BLOQUEADO ha %ds esperando 'mutex' (P o segura) -> DEADLOCK\n", TIMEOUT);
    else
        sem_post(&mutex);
    sem_post(&cheio);                                  /* devolve para encerrar a demo */
    return NULL;
}

static void demo_deadlock(void) {
    pthread_t p, c;
    init_sems(0, N);                                   /* buffer cheio: vazio=0, cheio=2 */
    printf("\n=== Deadlock: down(mutex) antes de down(vazio) ===\n");
    printf("Cenario: buffer cheio.\n");
    estado("INICIO");
    pthread_create(&p, NULL, produtor_errado, NULL);
    pthread_create(&c, NULL, consumidor_errado, NULL);
    pthread_join(p, NULL);
    pthread_join(c, NULL);
    printf("\nEspera circular: P espera 'vazio' (que so C libera);\n"
           "C espera 'mutex' (que P segura). Nenhum avanca.\n"
           "Correcao: down(vazio)/down(cheio) ANTES de down(mutex).\n");
    printf("(sem_timedwait foi usado so para a demo nao travar de verdade)\n");
    destroy_sems();
}

int main(void) {
    int op;
    do {
        printf("\n===== SEMAFOROS EM C - SO IFRN - GRUPO 8 =====\n"
               "1) Produtor-consumidor (correto)\n"
               "2) Demonstracao de deadlock\n"
               "0) Sair\n> ");
        fflush(stdout);
        if (scanf("%d", &op) != 1) break;
        if (op == 1) demo_correta();
        else if (op == 2) demo_deadlock();
    } while (op != 0);
    return 0;
}
